#include "llvm/Luxon/LuxonActionCache.h"
#include "LuxonBase.h"
#include "llvm/CAS/ActionCache.h"
#include "llvm/CAS/OnDiskHashMappedTrie.h"
#include "llvm/Luxon/LuxonCAS.h"
#include "llvm/Support/Alignment.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::cas;
using namespace llvm::cas::luxon;

namespace {

using HashType = LuxonCASContext::HashType;

class CacheEntry {
public:
  CacheEntry() = default;
  CacheEntry(ObjectID Value) : Value(Value.getOpaqueRef()) {}
  ObjectID getValue() const { return ObjectID::fromOpaqueRef(Value); }

private:
  uint64_t Value;
};

class LuxonActionCache final : public ActionCache {
public:
  Error putImpl(ArrayRef<uint8_t> ActionKey, const CASID &Result) final;
  Expected<Optional<CASID>> getImpl(ArrayRef<uint8_t> ActionKey) const final;

  static Expected<std::unique_ptr<LuxonActionCache>> create(StringRef Path);

  Error putLuxonImpl(ArrayRef<uint8_t> ActionKey, ObjectID Value);
  Expected<Optional<ObjectID>> getLuxonImpl(ArrayRef<uint8_t> ActionKey) const;

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
  using DataT = CacheEntry;
};
} // end namespace

static std::string hashToString(ArrayRef<uint8_t> Hash) {
  SmallString<64> Str;
  toHex(Hash, /*LowerCase=*/true, Str);
  return Str.str().str();
}

static Error createResultCachePoisonedError(StringRef Key,
                                            const CASContext &Context,
                                            ObjectID Output,
                                            ObjectID ExistingOutput) {
  return createStringError(
      std::make_error_code(std::errc::invalid_argument),
      "cache poisoned for '" + Key + "' (new='" + Twine(Output.getOpaqueRef()) +
          "' vs. existing '" + Twine(ExistingOutput.getOpaqueRef()) + "')");
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
  report_fatal_error("LuxonActionCache::getImpl: not implemented");
}

Error LuxonActionCache::putImpl(ArrayRef<uint8_t> Key, const CASID &Result) {
  report_fatal_error("LuxonActionCache::putImpl: not implemented");
}

Expected<Optional<ObjectID>>
LuxonActionCache::getLuxonImpl(ArrayRef<uint8_t> Key) const {
  // Check the result cache.
  OnDiskHashMappedTrie::const_pointer ActionP = Cache.find(Key);
  if (!ActionP)
    return std::nullopt;

  const DataT *Output = reinterpret_cast<const DataT *>(ActionP->Data.data());
  return Output->getValue();
}

Error LuxonActionCache::putLuxonImpl(ArrayRef<uint8_t> Key, ObjectID Result) {
  DataT Expected(Result);
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

Expected<std::optional<ObjectID>> LuxonCAS::cacheGet(DigestRef Key) {
  LuxonActionCache &Cache = *static_cast<LuxonActionCache *>(CacheImpl);
  return Cache.getLuxonImpl(Key);
}

Error LuxonCAS::cachePut(DigestRef Key, ObjectID Value) {
  LuxonActionCache &Cache = *static_cast<LuxonActionCache *>(CacheImpl);
  return Cache.putLuxonImpl(Key, Value);
}

Expected<bool>
LuxonCAS::cacheGetMap(DigestRef Key,
                      function_ref<void(StringRef, ObjectID)> Callback) {
  Expected<std::optional<ObjectID>> Result = cacheGet(Key);
  if (!Result)
    return Result.takeError();
  if (!*Result)
    return false;

  Expected<Optional<LoadedObject>> Obj = load(**Result);
  if (!Obj)
    return Obj.takeError();

  StringRef RemainingNamesBuf = (*Obj)->getData();
  (*Obj)->forEachReference(
      [&RemainingNamesBuf, &Callback](ObjectID ID, size_t) {
        StringRef Name = RemainingNamesBuf.data();
        assert(!Name.empty());
        Callback(Name, ID);
        assert(Name.size() < RemainingNamesBuf.size());
        assert(RemainingNamesBuf[Name.size()] == 0);
        RemainingNamesBuf = RemainingNamesBuf.substr(Name.size() + 1);
      });
  return true;
}

Error LuxonCAS::cachePutMap(DigestRef Key,
                            ArrayRef<std::pair<StringRef, ObjectID>> Values) {
  SmallString<128> NamesBuf;
  SmallVector<ObjectID, 6> Refs;
  Refs.reserve(Values.size());
  for (const auto &Value : Values) {
    assert(!Value.first.empty());
    NamesBuf += Value.first;
    NamesBuf.push_back(0);
    Refs.push_back(Value.second);
  }

  Expected<ObjectID> ID = store(NamesBuf.str(), Refs);
  if (!ID)
    return ID.takeError();

  return cachePut(Key, *ID);
}
