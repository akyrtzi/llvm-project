#ifndef LLVM_LUXON_LUXONCAS_H
#define LLVM_LUXON_LUXONCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas::luxon {

using DigestTy = std::array<uint8_t, 65>;
using DigestRef = ArrayRef<uint8_t>;

class LuxonCAS;

class ObjectID {
  uint64_t Opaque;

  ObjectID(uint64_t OpaqueRef) : Opaque(OpaqueRef) {}

public:
  static ObjectID fromOpaqueRef(uint64_t OpaqueRef) {
    return ObjectID(OpaqueRef);
  }

  uint64_t getOpaqueRef() const { return Opaque; }

  DigestRef getDigest(const LuxonCAS &CAS) const;

  void print(raw_ostream &OS, const LuxonCAS &CAS) const;
  std::string getAsString(const LuxonCAS &CAS) const;

  friend bool operator==(const ObjectID &LHS, const ObjectID &RHS) {
    return LHS.Opaque == RHS.Opaque;
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

  class refs_iterator
      : public iterator_facade_base<refs_iterator,
                                    std::random_access_iterator_tag, ObjectID> {
    uint64_t Opaque;
    LuxonCAS *CAS;

  public:
    refs_iterator(uint64_t Opaque, LuxonCAS *CAS) : Opaque(Opaque), CAS(CAS) {}

    uint64_t getOpaqueValue() const { return Opaque; }

    bool operator==(const refs_iterator &R) const { return Opaque == R.Opaque; }

    ObjectID operator*() const;

    ptrdiff_t operator-(const refs_iterator &RHS) const;
    refs_iterator &operator+=(ptrdiff_t N);
  };

  iterator_range<refs_iterator> refs_range() const;

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
