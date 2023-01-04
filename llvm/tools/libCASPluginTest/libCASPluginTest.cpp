#include "libCASPluginTest.h"
#include "llvm/Luxon/LuxonCAS.h"

using namespace llvm;
using namespace llvm::cas;
using namespace llvm::cas::luxon;

template <typename ResT>
static ResT reportError(Error &&E, char **error, ResT Result = ResT()) {
  if (error) {
    std::string errMsg = toString(std::move(E));
    *error = (char *)malloc(errMsg.size() + 1);
    const char *source = errMsg.c_str();
    std::uninitialized_copy(source, source + errMsg.size() + 1, *error);
  }
  return Result;
}

unsigned llcasplug_digest_parse(const char *printed_digest, uint8_t *bytes,
                                size_t bytes_size, char **error) {
  if (bytes_size < sizeof(DigestTy))
    return sizeof(DigestTy);

  Expected<DigestTy> Digest = LuxonCAS::parseID(printed_digest);
  if (!Digest)
    return reportError(Digest.takeError(), error, 0);
  std::uninitialized_copy(Digest->begin(), Digest->end(), bytes);
  return Digest->size();
}

bool llcasplug_digest_print(llcasplug_digest_t c_digest, char **printed_id,
                            char **error) {
  SmallString<90> PrintDigest;
  raw_svector_ostream OS(PrintDigest);
  // FIXME: LuxonCAS::printID should do validation and return Expected<>.
  LuxonCAS::printID(makeArrayRef(c_digest.data, c_digest.size), OS);
  char *copied_str = (char *)malloc(PrintDigest.size() + 1);
  const char *source = PrintDigest.c_str();
  std::uninitialized_copy(source, source + PrintDigest.size() + 1, copied_str);
  *printed_id = copied_str;
  return false;
}

namespace {

struct CASPluginOptions {
  std::string Path;
  Error parseOptions(ArrayRef<const char *> Opts);
};

} // namespace

Error CASPluginOptions::parseOptions(ArrayRef<const char *> Opts) {
  for (StringRef Opt : Opts) {
    auto [Name, Value] = Opt.split('=');
    if (Name == "cas-path")
      Path = Value;
  }
  return Error::success();
}

llcasplug_cas_options_t llcasplug_cas_options_create(void) {
  return new CASPluginOptions();
}

void llcasplug_cas_options_dispose(llcasplug_cas_options_t c_opts) {
  delete static_cast<CASPluginOptions *>(c_opts);
}

bool llcasplug_cas_options_parse(llcasplug_cas_options_t c_opts,
                                 const char **opts, size_t opts_count,
                                 char **error) {
  CASPluginOptions *Opts = static_cast<CASPluginOptions *>(c_opts);
  ArrayRef<const char *> Inputs = makeArrayRef(opts, opts_count);
  if (Error E = Opts->parseOptions(Inputs))
    return reportError(std::move(E), error, true);
  return false;
}

llcasplug_cas_t llcasplug_cas_create(llcasplug_cas_options_t c_opts,
                                     char **error) {
  CASPluginOptions *Opts = static_cast<CASPluginOptions *>(c_opts);
  Expected<std::unique_ptr<LuxonCAS>> CAS = LuxonCAS::create(Opts->Path);
  if (!CAS)
    return reportError<llcasplug_cas_t>(CAS.takeError(), error);
  return CAS->release();
}

void llcasplug_cas_dispose(llcasplug_cas_t c_cas) {
  delete static_cast<LuxonCAS *>(c_cas);
}

void llcasplug_string_dispose(char *str) { free(str); }

bool llcasplug_cas_get_objectid(llcasplug_cas_t c_cas,
                                llcasplug_digest_t c_digest,
                                llcasplug_objectid_t *c_id_p, char **error) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  Expected<ObjectID> ID = CAS.getID(makeArrayRef(c_digest.data, c_digest.size));
  if (!ID)
    return reportError(ID.takeError(), error, true);
  *c_id_p = llcasplug_objectid_t{ID->getOpaqueRef()};
  return false;
}

llcasplug_digest_t llcasplug_objectid_get_digest(llcasplug_cas_t c_cas,
                                                 llcasplug_objectid_t c_id) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  ObjectID ID = ObjectID::fromOpaqueRef(c_id.opaque);
  ArrayRef<uint8_t> Digest = ID.getDigest(CAS);
  return llcasplug_digest_t{Digest.data(), Digest.size()};
}

char *llcasplug_objectid_print_digest(llcasplug_cas_t c_cas,
                                      llcasplug_objectid_t c_id) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  ObjectID ID = ObjectID::fromOpaqueRef(c_id.opaque);

  SmallString<90> PrintDigest;
  raw_svector_ostream OS(PrintDigest);
  ID.print(OS, CAS);
  char *copied_str = (char *)malloc(PrintDigest.size() + 1);
  const char *source = PrintDigest.c_str();
  std::uninitialized_copy(source, source + PrintDigest.size() + 1, copied_str);
  return copied_str;
}

llcasplug_load_result_t
llcasplug_cas_load_object(llcasplug_cas_t c_cas, llcasplug_objectid_t c_id,
                          llcasplug_loaded_object_t *c_obj_p, char **error) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  ObjectID ID = ObjectID::fromOpaqueRef(c_id.opaque);
  Expected<Optional<LoadedObject>> ObjOpt = CAS.load(ID);
  if (!ObjOpt)
    return reportError(ObjOpt.takeError(), error, LLCASPLUG_LOAD_RESULT_ERROR);
  if (!*ObjOpt)
    return LLCASPLUG_LOAD_RESULT_NOTFOUND;

  const LoadedObject &Obj = **ObjOpt;
  *c_obj_p = llcasplug_loaded_object_t{Obj.getOpaqueRef()};
  return LLCASPLUG_LOAD_RESULT_SUCCESS;
}

bool llcasplug_cas_store_object(llcasplug_cas_t c_cas, llcasplug_data_t c_data,
                                const llcasplug_objectid_t *c_refs,
                                size_t c_refs_count,
                                llcasplug_objectid_t *c_id_p, char **error) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  SmallVector<ObjectID, 64> IDs;
  IDs.reserve(c_refs_count);
  for (unsigned I = 0; I != c_refs_count; ++I) {
    IDs.push_back(ObjectID::fromOpaqueRef(c_refs[I].opaque));
  }
  Expected<ObjectID> StoredID =
      CAS.store(StringRef((const char *)c_data.data, c_data.size), IDs);
  if (!StoredID)
    return reportError(StoredID.takeError(), error, true);
  *c_id_p = llcasplug_objectid_t{StoredID->getOpaqueRef()};
  return false;
}

llcasplug_data_t
llcasplug_loaded_object_get_data(llcasplug_cas_t c_cas,
                                 llcasplug_loaded_object_t c_obj) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  LoadedObject Obj = LoadedObject::fromOpaqueRef(c_obj.opaque, CAS);
  StringRef Data = Obj.getData();
  return llcasplug_data_t{Data.data(), Data.size()};
}

llcasplug_refs_range_t
llcasplug_loaded_object_get_refs_range(llcasplug_cas_t c_cas,
                                       llcasplug_loaded_object_t c_obj) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  LoadedObject Obj = LoadedObject::fromOpaqueRef(c_obj.opaque, CAS);
  iterator_range<LoadedObject::refs_iterator> RefsR = Obj.refs_range();
  return llcasplug_refs_range_t{{RefsR.begin().getOpaqueValue()},
                                {RefsR.end().getOpaqueValue()}};
}

llcasplug_refs_iterator_t llcasplug_refs_iterator_offset(
    llcasplug_cas_t c_cas, llcasplug_refs_iterator_t c_iter, ptrdiff_t offset) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  LoadedObject::refs_iterator RefsI(c_iter.opaque, &CAS);
  RefsI += offset;
  return llcasplug_refs_iterator_t{RefsI.getOpaqueValue()};
}

ptrdiff_t llcasplug_refs_iterator_distance(llcasplug_cas_t c_cas,
                                           llcasplug_refs_iterator_t begin,
                                           llcasplug_refs_iterator_t end) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  LoadedObject::refs_iterator B(begin.opaque, &CAS);
  LoadedObject::refs_iterator E(end.opaque, &CAS);
  return E - B;
}

llcasplug_objectid_t
llcasplug_refs_iterator_get_id(llcasplug_cas_t c_cas,
                               llcasplug_refs_iterator_t c_iter) {
  auto &CAS = *static_cast<LuxonCAS *>(c_cas);
  LoadedObject::refs_iterator RefsI(c_iter.opaque, &CAS);
  ObjectID Ref = *RefsI;
  return llcasplug_objectid_t{Ref.getOpaqueRef()};
}
