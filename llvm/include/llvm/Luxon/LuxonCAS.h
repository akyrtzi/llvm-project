#ifndef LLVM_LUXON_LUXONCAS_H
#define LLVM_LUXON_LUXONCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas {
class ObjectStore;

Expected<std::unique_ptr<ObjectStore>> createLuxonCAS(const Twine &Path);

} // namespace llvm::cas

#endif
