#ifndef LLVM_CAS_PLUGINCAS_H
#define LLVM_CAS_PLUGINCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ActionCache;
class ObjectStore;

Expected<std::pair<std::shared_ptr<ObjectStore>, std::shared_ptr<ActionCache>>>
createPluginCAS(StringRef PluginPath,
                ArrayRef<std::string> PluginArgs = std::nullopt);
Expected<std::pair<std::shared_ptr<ObjectStore>, std::shared_ptr<ActionCache>>>
createPluginCASFromPathAndOptions(StringRef PathAndOptions);

} // namespace llvm::cas

#endif
