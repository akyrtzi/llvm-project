//===- ActionCache.cpp ------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/CAS/ActionCache.h"
#include "llvm/ADT/StringMap.h"
#include "llvm/CAS/CASID.h"
#include "llvm/CAS/ObjectStore.h"
#include "llvm/Support/ManagedStatic.h"

using namespace llvm;
using namespace llvm::cas;

void ActionCache::anchor() {}

CacheKey::CacheKey(const CASID &ID) : Key(toStringRef(ID.getHash()).str()) {}
CacheKey::CacheKey(const ObjectProxy &Proxy)
    : CacheKey(Proxy.getCAS(), Proxy.getRef()) {}
CacheKey::CacheKey(const ObjectStore &CAS, const ObjectRef &Ref)
    : Key(toStringRef(CAS.getID(Ref).getHash())) {}

static ManagedStatic<StringMap<ActionCacheCreateFuncTy *>> RegisteredScheme;

static Expected<std::unique_ptr<ActionCache>>
createInMemoryActionCacheAdapter(StringRef) {
  return createInMemoryActionCache();
}

static StringMap<ActionCacheCreateFuncTy *> &getRegisteredScheme() {
  if (!RegisteredScheme.isConstructed()) {
    RegisteredScheme->insert({"mem://", &createInMemoryActionCacheAdapter});
    RegisteredScheme->insert({"file://", &createOnDiskActionCache});
  }
  return *RegisteredScheme;
}

Expected<std::unique_ptr<ActionCache>>
cas::createActionCacheFromIdentifier(StringRef Path) {
  for (auto &Scheme : getRegisteredScheme()) {
    if (Path.consume_front(Scheme.getKey()))
      return Scheme.getValue()(Path);
  }

  if (Path.empty())
    return createStringError(std::make_error_code(std::errc::invalid_argument),
                             "No ActionCache identifier is provided");

  return createOnDiskActionCache(Path);
}

void cas::registerActionCacheURLScheme(StringRef Prefix,
                                       ActionCacheCreateFuncTy *Func) {
  getRegisteredScheme().insert({Prefix, Func});
}
