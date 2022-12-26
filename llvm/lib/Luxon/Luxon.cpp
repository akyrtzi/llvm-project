#include "llvm/Luxon/Luxon.h"
#include "llvm/CAS/ActionCache.h"
#include "llvm/CAS/ObjectStore.h"
#include "llvm/Luxon/LuxonActionCache.h"
#include "llvm/Luxon/LuxonCAS.h"
#include "llvm/Support/Errc.h"

using namespace llvm;
using namespace llvm::cas;

static constexpr StringRef LuxonCASScheme = "luxon://";

void cas::registerLuxonCAS() {
  cas::registerCASURLScheme(LuxonCASScheme, &createLuxonCAS);
  cas::registerActionCacheURLScheme(LuxonCASScheme, &createLuxonActionCache);
}
