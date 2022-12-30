//===- SQLiteCAS.cpp --------------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Luxon/SQLiteCAS.h"
#include "LuxonBase.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/Errc.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/Path.h"

using namespace llvm;
using namespace llvm::cas;

using DataID = SQLiteCAS::DataID;
using CASObject = SQLiteCAS::CASObject;

Expected<DataID> SQLiteCAS::parseID(StringRef ID) {
  return LuxonCASContext::rawParseID(ID);
}

// Helper macro checking and returning error messages for failed SQLite calls
#define checkSQLiteResultOK(result) assert(result == SQLITE_OK)

std::string SQLiteCAS::getCurrentErrorMessage() {
  int err_code = sqlite3_errcode(db);
  const char *err_message = sqlite3_errmsg(db);
  const char *filename = sqlite3_db_filename(db, "main");

  std::string out;
  llvm::raw_string_ostream outStream(out);
  outStream << "error: accessing build database \"" << filename
            << "\": " << err_message;

  if (err_code == SQLITE_BUSY || err_code == SQLITE_LOCKED) {
    outStream << " Possibly there are two concurrent builds running in the "
                 "same filesystem location.";
  }

  outStream.flush();
  return out;
}

Expected<Optional<CASObject>> SQLiteCAS::get(const DataID &ID) {
  int result = sqlite3_reset(FindObjectForIDStmt);
  checkSQLiteResultOK(result);
  result = sqlite3_clear_bindings(FindObjectForIDStmt);
  checkSQLiteResultOK(result);
  result = sqlite3_bind_blob(FindObjectForIDStmt, /*index=*/1, ID.data(),
                             ID.size(), SQLITE_STATIC);
  checkSQLiteResultOK(result);

  result = sqlite3_step(FindObjectForIDStmt);
  if (result != SQLITE_ROW)
    return createStringError(errc::invalid_argument, "ID not found");
  assert(sqlite3_column_count(FindObjectForIDStmt) == 1);

  const void *blobPtr = sqlite3_column_blob(FindObjectForIDStmt, 0);
  const int blobSize = sqlite3_column_bytes(FindObjectForIDStmt, 0);

  StringRef bytes((const char *)blobPtr, blobSize);

  uint32_t numberOfReferences =
      support::endian::read<uint32_t, support::big>(bytes.data());

  size_t headerOffset = sizeof(uint32_t);

  SmallVector<DataID, 16> Refs;
  for (unsigned I = 0; I != numberOfReferences; ++I) {
    assert(headerOffset < bytes.size());
    uint8_t referenceLength = bytes[headerOffset];
    headerOffset += 1;
    assert(referenceLength > 0 &&
           (headerOffset + referenceLength) < bytes.size());
    StringRef reference = bytes.substr(headerOffset, referenceLength);
    DataID id;
    assert(reference.size() == sizeof(DataID));
    std::uninitialized_copy(reference.begin(), reference.end(), id.data());
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

SQLiteCAS::SQLiteCAS(StringRef Path) {
  SmallString<128> PathBuf(Path);
  sys::path::append(PathBuf, "sqlite.db");

  static int sqliteConfigureResult = []() -> int {
    // We access a single connection from multiple threads.
    return sqlite3_config(SQLITE_CONFIG_MULTITHREAD);
  }();
  if (sqliteConfigureResult != SQLITE_OK) {
    if (!sqlite3_threadsafe()) {
      report_fatal_error("unable to configure database: not thread-safe");
    }
  }

  int result =
      sqlite3_open_v2(PathBuf.c_str(), &db, SQLITE_OPEN_READONLY, nullptr);
  if (result != SQLITE_OK) {
    report_fatal_error(Twine("unable to open database: ") +
                       std::string(sqlite3_errstr(result)));
  }

  // Initialize prepared statements.
  result = sqlite3_prepare_v2(db, FindObjectForIDStmtSQL, -1,
                              &FindObjectForIDStmt, nullptr);
  checkSQLiteResultOK(result);
}

SQLiteCAS::~SQLiteCAS() {
  sqlite3_finalize(FindObjectForIDStmt);
  FindObjectForIDStmt = nullptr;

  int result = sqlite3_close(db);
  (void)result; // use the variable if we're building without asserts
  assert(result == SQLITE_OK &&
         "The database connection could not be closed. That means there are "
         "prepared statements that are not finalized, data blobs that are not "
         "closed or backups not finished.");
}

Expected<std::unique_ptr<SQLiteCAS>> SQLiteCAS::create(StringRef FilePath) {
  std::unique_ptr<SQLiteCAS> Ptr;
  Ptr.reset(new SQLiteCAS(FilePath));
  return std::move(Ptr);
}
