#ifndef LLVM_LUXON_LUXONACTIONCACHE_H
#define LLVM_LUXON_LUXONACTIONCACHE_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ActionCache;

Expected<std::unique_ptr<ActionCache>> createLuxonActionCache(StringRef Path);

} // namespace llvm::cas

#endif
