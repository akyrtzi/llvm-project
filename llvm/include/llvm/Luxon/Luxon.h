#ifndef LLVM_LUXON_LUXON_H
#define LLVM_LUXON_LUXON_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ActionCache;
class ObjectStore;

Expected<std::unique_ptr<ObjectStore>> createLuxonCAS(const Twine &Path);

Expected<std::unique_ptr<ActionCache>> createLuxonActionCache(StringRef Path);

void registerLuxonCAS();

} // namespace llvm::cas

#endif
