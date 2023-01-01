#ifndef LLVM_LUXON_LUXONCAS_H
#define LLVM_LUXON_LUXONCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas::luxon {

using DigestTy = std::array<uint8_t, 65>;
using DigestRef = ArrayRef<uint8_t>;

class LuxonCAS;

class ObjectID {
  uint64_t Opaque;
  LuxonCAS *CAS;

  ObjectID(uint64_t OpaqueRef, LuxonCAS &CAS) : Opaque(OpaqueRef), CAS(&CAS) {}
  friend class LuxonCAS;

public:
  static ObjectID fromOpaqueRef(uint64_t OpaqueRef, LuxonCAS &CAS) {
    return ObjectID(OpaqueRef, CAS);
  }

  uint64_t getOpaqueRef() const { return Opaque; }

  DigestRef getDigest() const;

  void print(raw_ostream &OS) const;
  std::string getAsString() const;

  friend bool operator==(const ObjectID &LHS, const ObjectID &RHS) {
    return LHS.Opaque == RHS.Opaque && LHS.CAS == RHS.CAS;
  }
  friend bool operator!=(const ObjectID &LHS, const ObjectID &RHS) {
    return !(LHS == RHS);
  }
};

class LoadedObject {
  uint64_t Opaque;
  LuxonCAS *CAS;

  LoadedObject(uint64_t OpaqueRef, LuxonCAS &CAS)
      : Opaque(OpaqueRef), CAS(&CAS) {}

public:
  static LoadedObject fromOpaqueRef(uint64_t OpaqueRef, LuxonCAS &CAS) {
    return LoadedObject(OpaqueRef, CAS);
  }

  uint64_t getOpaqueRef() const { return Opaque; }

  StringRef getData() const;

  size_t getNumReferences() const;
  ObjectID getReference(size_t I);
  void forEachReference(function_ref<void(ObjectID, size_t Index)> Callback);

  friend bool operator==(const LoadedObject &LHS, const LoadedObject &RHS) {
    return LHS.Opaque == RHS.Opaque && LHS.CAS == RHS.CAS;
  }
  friend bool operator!=(const LoadedObject &LHS, const LoadedObject &RHS) {
    return !(LHS == RHS);
  }
};

class LuxonCAS {
public:
  static Expected<std::unique_ptr<LuxonCAS>> create(const Twine &Path);
  ~LuxonCAS();

  static void printID(DigestRef ID, raw_ostream &OS);
  static Expected<DigestTy> parseID(StringRef ID);

  static DigestTy digestObject(StringRef Data, ArrayRef<DigestRef> Refs);

  Expected<ObjectID> getID(DigestRef Digest);
  ObjectID getIDFromDigests(StringRef Data, ArrayRef<DigestRef> Refs);
  ObjectID getID(StringRef Data, ArrayRef<ObjectID> Refs);

  bool containsObject(const ObjectID &ID);

  Expected<Optional<LoadedObject>> load(const ObjectID &ID);

  Expected<ObjectID> store(StringRef Data, ArrayRef<ObjectID> Refs);
  Error storeForKnownID(const ObjectID &KnownID, StringRef Data,
                        ArrayRef<ObjectID> Refs);

private:
  friend class LoadedObject;
  friend class ObjectID;

  void *Impl; // a \p LuxonStore.
};

} // namespace llvm::cas::luxon

#endif
