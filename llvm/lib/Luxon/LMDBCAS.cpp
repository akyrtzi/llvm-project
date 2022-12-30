//===- LMDBCAS.cpp ----------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Luxon/LMDBCAS.h"
#include "LuxonBase.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;
using namespace llvm::cas;

using DataID = LMDBCAS::DataID;
using CASObject = LMDBCAS::CASObject;

Expected<DataID> LMDBCAS::parseID(StringRef ID) {
  return LuxonCASContext::rawParseID(ID);
}

template <typename T>
static T withTransaction(MDB_env *env, function_ref<T(MDB_txn *)> body) {
  MDB_txn *txn;
  int err = mdb_txn_begin(env, nullptr, MDB_RDONLY, &txn);
  assert(!err);

  T res = body(txn);
  err = mdb_txn_commit(txn);
  assert(!err);

  return res;
}

Expected<Optional<CASObject>> LMDBCAS::get(DataRef ID) {
  StringRef bytes =
      withTransaction<StringRef>(DBEnv, [&](MDB_txn *txn) -> StringRef {
        MDB_val keyVal{ID.size(), (void *)ID.data()};
        MDB_val data;
        int err = mdb_get(txn, DBHandle, &keyVal, &data);
        assert(!err);

        return StringRef((const char *)data.mv_data, data.mv_size);
      });

  uint32_t numberOfReferences =
      support::endian::read<uint32_t, support::big>(bytes.data());

  size_t headerOffset = sizeof(uint32_t);

  SmallVector<DataRef, 32> Refs;
  for (unsigned I = 0; I != numberOfReferences; ++I) {
    assert(headerOffset < bytes.size());
    uint8_t referenceLength = bytes[headerOffset];
    headerOffset += 1;
    assert(referenceLength > 0 &&
           (headerOffset + referenceLength) < bytes.size());
    DataRef id =
        arrayRefFromStringRef(bytes.substr(headerOffset, referenceLength));
    Refs.push_back(id);
    headerOffset += referenceLength;
  }

  assert((headerOffset + sizeof(uint64_t)) <= bytes.size());
  uint64_t ctField = support::endian::read<uint64_t, support::big>(
      bytes.data() + headerOffset);
  uint64_t contentLength = ctField & UINT64_MAX >> (8);

  headerOffset += sizeof(uint64_t);
  assert(headerOffset <= bytes.size());
  assert(headerOffset + contentLength <= bytes.size());

  StringRef Data = bytes.substr(headerOffset, contentLength);
  return CASObject{Data, std::move(Refs)};
}

LMDBCAS::LMDBCAS(StringRef Path) {
  int err = mdb_env_create(&DBEnv);
  assert(!err);
  err = mdb_env_set_maxdbs(DBEnv, 1);
  assert(!err);
  err = mdb_env_set_mapsize(DBEnv, uint64_t(512) * 1024 * 1024 * 1024);
  assert(!err);

  SmallString<128> PathBuf(Path);
  err = mdb_env_open(DBEnv, PathBuf.c_str(), MDB_RDONLY, 0644);
  assert(!err);

  DBHandle = withTransaction<MDB_dbi>(DBEnv, [](MDB_txn *txn) -> MDB_dbi {
    MDB_dbi handle;
    int err = mdb_dbi_open(txn, "objects", 0, &handle);
    assert(!err);
    return handle;
  });
}

LMDBCAS::~LMDBCAS() { mdb_env_close(DBEnv); }

Expected<std::unique_ptr<LMDBCAS>> LMDBCAS::create(StringRef FilePath) {
  std::unique_ptr<LMDBCAS> Ptr;
  Ptr.reset(new LMDBCAS(FilePath));
  return std::move(Ptr);
}
