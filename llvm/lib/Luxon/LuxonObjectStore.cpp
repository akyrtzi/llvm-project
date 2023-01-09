#include "llvm/Luxon/LuxonObjectStore.h"
#include "llvm/CAS/ActionCache.h"
#include "llvm/CAS/ObjectStore.h"
#include "llvm/Luxon/LuxonCAS.h"
#include "llvm/Support/Errc.h"

using namespace llvm;
using namespace llvm::cas;

static constexpr StringLiteral LuxonCASScheme = "luxon://";

void cas::registerLuxonCAS() {
  cas::registerCASURLScheme(LuxonCASScheme, &createLuxonObjectStore);
}
