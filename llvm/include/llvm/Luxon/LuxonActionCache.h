#ifndef LLVM_LUXON_LUXONACTIONCACHE_H
#define LLVM_LUXON_LUXONACTIONCACHE_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ActionCache;

Expected<std::unique_ptr<ActionCache>> createLuxonActionCache(StringRef Path);

Expected<std::unique_ptr<ActionCache>>
createPluginActionCache(StringRef PluginPath,
                        ArrayRef<std::string> PluginArgs = std::nullopt);
Expected<std::unique_ptr<ActionCache>>
createPluginActionCacheFromPathAndOptions(StringRef PathAndOptions);

} // namespace llvm::cas

#endif
