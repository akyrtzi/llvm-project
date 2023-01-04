//===- PluginCAS.cpp --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "PluginAPI.h"
#include "llvm/ADT/ScopeExit.h"
#include "llvm/CAS/ObjectStore.h"
#include <dlfcn.h>

using namespace llvm;
using namespace llvm::cas;

namespace {

class PluginCAS;

class PluginCASContext : public CASContext {
public:
  void printIDImpl(raw_ostream &OS, const CASID &ID) const final;

  StringRef getHashSchemaIdentifier() const final {
    return "llvm.cas.plugin.v1";
  }

  PluginCASContext(PluginCAS &CAS) : CAS(CAS) {}

private:
  PluginCAS &CAS;
};

class PluginCAS : public ObjectStore {
public:
  //===--------------------------------------------------------------------===//
  // ObjectStore API
  //===--------------------------------------------------------------------===//

  Expected<CASID> parseID(StringRef ID) final;
  Expected<ObjectRef> store(ArrayRef<ObjectRef> Refs,
                            ArrayRef<char> Data) final;
  CASID getID(ObjectRef Ref) const final;
  CASID getID(ObjectHandle Handle) const final;
  Optional<ObjectRef> getReference(const CASID &ID) const final;
  ObjectRef getReference(ObjectHandle Handle) const final;
  Expected<ObjectHandle> load(ObjectRef Ref) final;
  Error validate(const CASID &ID) final {
    // Not supported yet. Always return success.
    return Error::success();
  }
  uint64_t getDataSize(ObjectHandle Node) const final;
  Error forEachRef(ObjectHandle Node,
                   function_ref<Error(ObjectRef)> Callback) const final;
  ObjectRef readRef(ObjectHandle Node, size_t I) const final;
  size_t getNumRefs(ObjectHandle Node) const final;
  ArrayRef<char> getData(ObjectHandle Node,
                         bool RequiresNullTerminator = false) const final;

  Expected<ObjectRef>
  storeFromOpenFileImpl(sys::fs::file_t FD,
                        Optional<sys::fs::file_status> Status) final;

  //===--------------------------------------------------------------------===//
  // PluginCAS API
  //===--------------------------------------------------------------------===//

  void printID(const CASID &ID, raw_ostream &OS) const;

  static Expected<std::unique_ptr<PluginCAS>>
  create(StringRef LibraryPath, ArrayRef<std::string> PluginArgs);

  PluginCAS();
  ~PluginCAS();

private:
  PluginCASContext Context;

  void *DLHandle = nullptr;
  llcasplug_functions_t Functions{};
  llcasplug_cas_t c_cas = nullptr;

  static Error errorFromCMessage(char *c_err,
                                 const llcasplug_functions_t &Funcs) {
    Error E = createStringError(inconvertibleErrorCode(), c_err);
    Funcs.string_dispose(c_err);
    return E;
  }

  Error errorWithConsumed(char *c_err) const {
    return errorFromCMessage(c_err, Functions);
  }
};

} // anonymous namespace

Expected<CASID> PluginCAS::parseID(StringRef ID) {
  SmallString<91> IDBuf(ID);
  SmallVector<uint8_t, 65> BytesBuf;
  BytesBuf.resize(65);

  auto parseDigest = [&]() -> Expected<unsigned> {
    char *c_err = nullptr;
    unsigned NumBytes = Functions.digest_parse(IDBuf.c_str(), BytesBuf.data(),
                                               BytesBuf.size(), &c_err);
    if (NumBytes == 0)
      return errorWithConsumed(c_err);
    return NumBytes;
  };

  Expected<unsigned> NumBytes = parseDigest();
  if (!NumBytes)
    return NumBytes.takeError();

  if (*NumBytes > BytesBuf.size()) {
    BytesBuf.resize(*NumBytes);
    NumBytes = parseDigest();
    if (!NumBytes)
      return NumBytes.takeError();
    assert(*NumBytes == BytesBuf.size());
  } else {
    BytesBuf.resize(*NumBytes);
  }

  return CASID::create(&getContext(), toStringRef(BytesBuf));
}

Expected<ObjectRef> PluginCAS::store(ArrayRef<ObjectRef> Refs,
                                     ArrayRef<char> Data) {
  SmallVector<llcasplug_objectid_t, 64> c_ids;
  c_ids.reserve(Refs.size());
  for (ObjectRef Ref : Refs) {
    c_ids.push_back(llcasplug_objectid_t{Ref.getInternalRef(*this)});
  }

  llcasplug_objectid_t c_stored_id;
  char *c_err = nullptr;
  if (Functions.cas_store_object(
          c_cas, llcasplug_data_t{Data.data(), Data.size()}, c_ids.data(),
          c_ids.size(), &c_stored_id, &c_err))
    return errorWithConsumed(c_err);

  return ObjectRef::getFromInternalRef(*this, c_stored_id.opaque);
}

static StringRef toStringRef(llcasplug_digest_t c_digest) {
  return StringRef((const char *)c_digest.data, c_digest.size);
}

CASID PluginCAS::getID(ObjectRef Ref) const {
  llcasplug_objectid_t c_id{Ref.getInternalRef(*this)};
  llcasplug_digest_t c_digest = Functions.objectid_get_digest(c_cas, c_id);
  return CASID::create(&getContext(), toStringRef(c_digest));
}

CASID PluginCAS::getID(ObjectHandle Handle) const {
  // FIXME: Remove getID(ObjectHandle) from API requirement.
  report_fatal_error("not implemented");
}

Optional<ObjectRef> PluginCAS::getReference(const CASID &ID) const {
  ArrayRef<uint8_t> Hash = ID.getHash();
  llcasplug_objectid_t c_id;
  char *c_err = nullptr;
  if (Functions.cas_get_objectid(
          c_cas, llcasplug_digest_t{Hash.data(), Hash.size()}, &c_id, &c_err))
    report_fatal_error(toString(errorWithConsumed(c_err)).c_str());
  return ObjectRef::getFromInternalRef(*this, c_id.opaque);
}

ObjectRef PluginCAS::getReference(ObjectHandle Handle) const {
  // FIXME: Remove getReference(ObjectHandle) from API requirement.
  report_fatal_error("not implemented");
}

Expected<ObjectHandle> PluginCAS::load(ObjectRef Ref) {
  llcasplug_objectid_t c_id{Ref.getInternalRef(*this)};
  llcasplug_loaded_object_t c_obj;
  char *c_err = nullptr;
  llcasplug_load_result_t c_result =
      Functions.cas_load_object(c_cas, c_id, &c_obj, &c_err);
  switch (c_result) {
  case LLCASPLUG_LOAD_RESULT_SUCCESS:
    return makeObjectHandle(c_obj.opaque);
  case LLCASPLUG_LOAD_RESULT_NOTFOUND:
    report_fatal_error("PluginCAS: object reference not found");
  case LLCASPLUG_LOAD_RESULT_ERROR:
    return errorWithConsumed(c_err);
  }
}

uint64_t PluginCAS::getDataSize(ObjectHandle Node) const {
  // FIXME: Remove getDataSize(ObjectHandle) from API requirement,
  // getData(ObjectHandle) should be enough.
  ArrayRef<char> Data = getData(Node);
  return Data.size();
}

// FIXME: Replace forEachRef/readRef/getNumRefs with an iterator interface.
Error PluginCAS::forEachRef(ObjectHandle Node,
                            function_ref<Error(ObjectRef)> Callback) const {
  llcasplug_refs_range_t c_refs = Functions.loaded_object_get_refs_range(
      c_cas, llcasplug_loaded_object_t{Node.getInternalRef(*this)});
  for (llcasplug_refs_iterator_t c_iter = c_refs.begin;
       c_iter.opaque != c_refs.end.opaque;
       c_iter = Functions.refs_iterator_offset(c_cas, c_iter, 1)) {
    llcasplug_objectid_t c_id = Functions.refs_iterator_get_id(c_cas, c_iter);
    ObjectRef Ref = ObjectRef::getFromInternalRef(*this, c_id.opaque);
    if (Error E = Callback(Ref))
      return E;
  }
  return Error::success();
}

ObjectRef PluginCAS::readRef(ObjectHandle Node, size_t I) const {
  llcasplug_refs_range_t c_refs = Functions.loaded_object_get_refs_range(
      c_cas, llcasplug_loaded_object_t{Node.getInternalRef(*this)});
  llcasplug_refs_iterator_t c_iter =
      Functions.refs_iterator_offset(c_cas, c_refs.begin, I);
  llcasplug_objectid_t c_id = Functions.refs_iterator_get_id(c_cas, c_iter);
  return ObjectRef::getFromInternalRef(*this, c_id.opaque);
}

size_t PluginCAS::getNumRefs(ObjectHandle Node) const {
  llcasplug_refs_range_t c_refs = Functions.loaded_object_get_refs_range(
      c_cas, llcasplug_loaded_object_t{Node.getInternalRef(*this)});
  return Functions.refs_iterator_distance(c_cas, c_refs.begin, c_refs.end);
}

ArrayRef<char> PluginCAS::getData(ObjectHandle Node,
                                  bool RequiresNullTerminator) const {
  // FIXME: Remove RequiresNullTerminator from API requirement. Choose whether
  // to always require it or not.
  llcasplug_data_t c_data = Functions.loaded_object_get_data(
      c_cas, llcasplug_loaded_object_t{Node.getInternalRef(*this)});
  return makeArrayRef((const char *)c_data.data, c_data.size);
}

Expected<ObjectRef>
PluginCAS::storeFromOpenFileImpl(sys::fs::file_t FD,
                                 Optional<sys::fs::file_status> Status) {
  // FIXME: Remove storeFromOpenFileImpl from API requirement.
  report_fatal_error("not implemented");
}

void PluginCAS::printID(const CASID &ID, raw_ostream &OS) const {
  ArrayRef<uint8_t> Hash = ID.getHash();
  char *c_printed_id = nullptr;
  char *c_err = nullptr;
  if (Functions.digest_print(llcasplug_digest_t{Hash.data(), Hash.size()},
                             &c_printed_id, &c_err))
    report_fatal_error(toString(errorWithConsumed(c_err)).c_str());
  OS << c_printed_id;
  Functions.string_dispose(c_printed_id);
}

void PluginCASContext::printIDImpl(raw_ostream &OS, const CASID &ID) const {
  CAS.printID(ID, OS);
}

PluginCAS::PluginCAS() : ObjectStore(Context), Context(*this) {}

PluginCAS::~PluginCAS() {
  Functions.cas_dispose(c_cas);
  // Intentionally leak the DLHandle; we have no reason to dlclose it and it may
  // be unsafe.
}

Expected<std::unique_ptr<PluginCAS>>
PluginCAS::create(StringRef LibraryPath, ArrayRef<std::string> PluginArgs) {
  auto reportError = [LibraryPath](const Twine &Description) -> Error {
    return createStringError(inconvertibleErrorCode(),
                             "PluginCAS: error loading '" + LibraryPath +
                                 "': " + Description);
  };

  SmallString<256> PathBuf = LibraryPath;
  void *DLHandle = dlopen(PathBuf.c_str(), RTLD_LOCAL | RTLD_FIRST);
  if (!DLHandle)
    return reportError("failed opening library");

  llcasplug_functions_t Functions{};

#define CASPLUGINAPI_FUNCTION(name, required)                                  \
  if (!(Functions.name = (decltype(llcasplug_functions_t::name))dlsym(         \
            DLHandle, "llcasplug_" #name))) {                                  \
    if (required)                                                              \
      return reportError("failed symbol 'llcasplug_" #name "' lookup");        \
  }
#include "PluginAPI_functions.def"
#undef CASPLUGINAPI_FUNCTION

  llcasplug_cas_options_t c_opts = Functions.cas_options_create();
  auto _ = make_scope_exit([&]() { Functions.cas_options_dispose(c_opts); });

  SmallVector<const char *, 10> CArgs;
  for (const std::string &Arg : PluginArgs)
    CArgs.push_back(Arg.c_str());

  char *c_err = nullptr;
  if (Functions.cas_options_parse(c_opts, CArgs.data(), CArgs.size(), &c_err))
    return errorFromCMessage(c_err, Functions);

  llcasplug_cas_t c_cas = Functions.cas_create(c_opts, &c_err);
  if (!c_cas)
    return errorFromCMessage(c_err, Functions);

  auto CAS = std::make_unique<PluginCAS>();
  CAS->DLHandle = DLHandle;
  CAS->Functions = Functions;
  CAS->c_cas = c_cas;
  return CAS;
}

Expected<std::unique_ptr<ObjectStore>>
cas::createPluginCAS(StringRef LibraryPath, ArrayRef<std::string> PluginArgs) {
  return PluginCAS::create(LibraryPath, PluginArgs);
}

Expected<std::unique_ptr<ObjectStore>>
cas::createPluginCASFromPathAndOptions(const Twine &PathAndOptions) {
  SmallString<256> Buf;
  PathAndOptions.toVector(Buf);
  auto [Path, URLOpts] = Buf.str().split('?');

  SmallVector<StringRef, 10> Opts;
  URLOpts.split(Opts, '&');
  SmallVector<std::string, 10> OptsStr;
  for (StringRef Opt : Opts)
    OptsStr.push_back(Opt.str());

  return createPluginCAS(Path, OptsStr);
}
