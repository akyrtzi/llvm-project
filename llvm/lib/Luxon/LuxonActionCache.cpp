#include "llvm/Luxon/LuxonActionCache.h"
#include "LuxonBase.h"
#include "llvm/CAS/ActionCache.h"
#include "llvm/CAS/OnDiskHashMappedTrie.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::cas;

namespace {

using HashType = LuxonCASContext::HashType;

template <size_t Size> class CacheEntry {
public:
  CacheEntry() = default;
  CacheEntry(ArrayRef<uint8_t> Hash) { llvm::copy(Hash, Value.data()); }
  CacheEntry(const CacheEntry &Entry) { llvm::copy(Entry.Value, Value.data()); }
  ArrayRef<uint8_t> getValue() const { return Value; }

private:
  std::array<uint8_t, Size> Value;
};

class LuxonActionCache final : public ActionCache {
public:
  Error putImpl(ArrayRef<uint8_t> ActionKey, const CASID &Result) final;
  Expected<Optional<CASID>> getImpl(ArrayRef<uint8_t> ActionKey) const final;

  static Expected<std::unique_ptr<LuxonActionCache>> create(StringRef Path);

private:
  static StringRef getHashName() { return "BLAKE2b"; }
  static StringRef getActionCacheTableName() {
    static const std::string Name =
        ("llvm.actioncache[" + getHashName() + "->" + getHashName() + "]")
            .str();
    return Name;
  }
  static constexpr StringLiteral ActionCacheFile = "actions";
  static constexpr StringLiteral FilePrefix = "v1.";

  LuxonActionCache(StringRef RootPath, OnDiskHashMappedTrie ActionCache);

  std::string Path;
  OnDiskHashMappedTrie Cache;
  using DataT = CacheEntry<sizeof(HashType)>;
};
} // end namespace

static std::string hashToString(ArrayRef<uint8_t> Hash) {
  SmallString<64> Str;
  toHex(Hash, /*LowerCase=*/true, Str);
  return Str.str().str();
}

static Error createResultCachePoisonedError(StringRef Key,
                                            const CASContext &Context,
                                            CASID Output,
                                            ArrayRef<uint8_t> ExistingOutput) {
  std::string Existing =
      CASID::create(&Context, toStringRef(ExistingOutput)).toString();
  return createStringError(std::make_error_code(std::errc::invalid_argument),
                           "cache poisoned for '" + Key + "' (new='" +
                               Output.toString() + "' vs. existing '" +
                               Existing + "')");
}

constexpr StringLiteral LuxonActionCache::ActionCacheFile;
constexpr StringLiteral LuxonActionCache::FilePrefix;

LuxonActionCache::LuxonActionCache(StringRef Path, OnDiskHashMappedTrie Cache)
    : ActionCache(LuxonCASContext::getDefaultContext()), Path(Path.str()),
      Cache(std::move(Cache)) {}

Expected<std::unique_ptr<LuxonActionCache>>
LuxonActionCache::create(StringRef AbsPath) {
  if (std::error_code EC = sys::fs::create_directories(AbsPath))
    return createFileError(AbsPath, EC);

  SmallString<256> CachePath(AbsPath);
  sys::path::append(CachePath, FilePrefix + ActionCacheFile);
  constexpr uint64_t MB = 1024ull * 1024ull;
  constexpr uint64_t GB = 1024ull * 1024ull * 1024ull;

  Optional<OnDiskHashMappedTrie> ActionCache;
  if (Error E = OnDiskHashMappedTrie::create(
                    CachePath, getActionCacheTableName(), sizeof(HashType) * 8,
                    /*DataSize=*/sizeof(DataT), /*MaxFileSize=*/GB,
                    /*MinFileSize=*/MB)
                    .moveInto(ActionCache))
    return std::move(E);

  return std::unique_ptr<LuxonActionCache>(
      new LuxonActionCache(AbsPath, std::move(*ActionCache)));
}

Expected<Optional<CASID>>
LuxonActionCache::getImpl(ArrayRef<uint8_t> Key) const {
  // Check the result cache.
  OnDiskHashMappedTrie::const_pointer ActionP = Cache.find(Key);
  if (!ActionP)
    return std::nullopt;

  const DataT *Output = reinterpret_cast<const DataT *>(ActionP->Data.data());
  return CASID::create(&getContext(), toStringRef(Output->getValue()));
}

Error LuxonActionCache::putImpl(ArrayRef<uint8_t> Key, const CASID &Result) {
  DataT Expected(Result.getHash());
  OnDiskHashMappedTrie::pointer ActionP = Cache.insertLazy(
      Key, [&](FileOffset TentativeOffset,
               OnDiskHashMappedTrie::ValueProxy TentativeValue) {
        assert(TentativeValue.Data.size() == sizeof(DataT));
        assert(isAddrAligned(Align::Of<DataT>(), TentativeValue.Data.data()));
        new (TentativeValue.Data.data()) DataT{Expected};
      });
  const DataT *Observed = reinterpret_cast<const DataT *>(ActionP->Data.data());

  if (Expected.getValue() == Observed->getValue())
    return Error::success();

  return createResultCachePoisonedError(hashToString(Key), getContext(), Result,
                                        Observed->getValue());
}

Expected<std::unique_ptr<ActionCache>>
cas::createLuxonActionCache(StringRef Path) {
  return LuxonActionCache::create(Path);
}

Expected<std::unique_ptr<ActionCache>>
cas::createPluginActionCache(StringRef LibraryPath,
                             ArrayRef<std::string> PluginArgs) {
  for (StringRef Opt : PluginArgs) {
    auto [Name, Value] = Opt.split('=');
    if (Name == "cas-path")
      return createLuxonActionCache(Value);
  }
  return createStringError(inconvertibleErrorCode(),
                           "plugin action-cache: missing 'cas-path' option");
}

Expected<std::unique_ptr<ActionCache>>
cas::createPluginActionCacheFromPathAndOptions(StringRef PathAndOptions) {
  auto [Path, URLOpts] = PathAndOptions.split('?');

  SmallVector<StringRef, 10> Opts;
  URLOpts.split(Opts, '&');
  SmallVector<std::string, 10> OptsStr;
  for (StringRef Opt : Opts)
    OptsStr.push_back(Opt.str());

  return createPluginActionCache(Path, OptsStr);
}
