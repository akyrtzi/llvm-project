#ifndef LLVM_LUXON_LUXONOBJECTSTORE_H
#define LLVM_LUXON_LUXONOBJECTSTORE_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ObjectStore;

Expected<std::unique_ptr<ObjectStore>>
createLuxonObjectStore(const Twine &Path);

void registerLuxonCAS();

} // namespace llvm::cas

#endif
