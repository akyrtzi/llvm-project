#ifndef LLVM_LUXON_LUXONBASE_H
#define LLVM_LUXON_LUXONBASE_H

#include "llvm/CAS/CASID.h"
#include "llvm/Support/Error.h"

namespace llvm::cas {
class ObjectRef;
class ObjectStore;

class LuxonCASContext : public CASContext {
  void printIDImpl(raw_ostream &OS, const CASID &ID) const final;

public:
  using HashType = std::array<uint8_t, 65>;
  static Expected<HashType> rawParseID(StringRef Reference);
  Expected<CASID> parseID(StringRef Reference) const;

  static void printID(raw_ostream &OS, ArrayRef<uint8_t> ID);

  static HashType hashObject(const ObjectStore &CAS, ArrayRef<ObjectRef> Refs,
                             ArrayRef<char> Data);
  static HashType hashObjectWithHashes(const ObjectStore &CAS,
                                       ArrayRef<ArrayRef<uint8_t>> Hashes,
                                       ArrayRef<char> Data);

  static StringRef getHashName() { return "BLAKE2b"; }
  StringRef getHashSchemaIdentifier() const final {
    static const std::string ID =
        ("llvm.cas.luxon.v1[" + getHashName() + "]").str();
    return ID;
  }

  static const LuxonCASContext &getDefaultContext();

  LuxonCASContext() = default;
};

} // namespace llvm::cas

#endif
