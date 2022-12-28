#ifndef LLVM_LUXON_LUXONCAS_H
#define LLVM_LUXON_LUXONCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ObjectProxy;
class ObjectRef;
class ObjectStore;

Expected<std::unique_ptr<ObjectStore>> createLuxonCAS(const Twine &Path);

uint64_t luxon_ObjectStore_getInternalRef(ObjectStore &CAS, ObjectRef Ref);
ObjectRef luxon_ObjectStore_getObjectRefFromInternal(ObjectStore &CAS,
                                                     uint64_t);

ArrayRef<uint8_t> luxon_ObjectStore_getHash(ObjectStore &CAS, ObjectRef Ref);
Expected<ObjectRef> luxon_ObjectStore_createRefFromHash(ObjectStore &CAS,
                                                        ArrayRef<uint8_t> Hash);

std::array<uint8_t, 65> luxon_ObjectStore_hashDigest(ObjectStore &CAS,
                                                     ArrayRef<ObjectRef> Refs,
                                                     ArrayRef<char> Data);
ObjectRef luxon_ObjectStore_refDigest(ObjectStore &CAS,
                                      ArrayRef<ObjectRef> Refs,
                                      ArrayRef<char> Data);

bool luxon_ObjectStore_containsRef(ObjectStore &CAS, ObjectRef Ref);
Expected<Optional<ObjectProxy>> luxon_ObjectStore_load(ObjectStore &CAS,
                                                       ObjectRef Ref);
Error luxon_ObjectStore_storeForRef(ObjectStore &CAS, ObjectRef Ref,
                                    ArrayRef<ObjectRef> Refs,
                                    ArrayRef<char> Data);

} // namespace llvm::cas

#endif
