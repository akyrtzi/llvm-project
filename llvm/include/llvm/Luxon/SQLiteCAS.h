#ifndef LLVM_LUXON_SQLITECAS_H
#define LLVM_LUXON_SQLITECAS_H

#include "llvm/Support/Error.h"
#include <sqlite3.h>

namespace llvm::cas {

class SQLiteCAS {
public:
  using DataID = std::array<uint8_t, 65>;

  struct CASObject {
    SmallString<32> Data;
    SmallVector<DataID, 16> Refs;
  };

  Expected<DataID> parseID(StringRef ID);

  Expected<Optional<CASObject>> get(const DataID &ID);

  static Expected<std::unique_ptr<SQLiteCAS>> create(StringRef FilePath);
  ~SQLiteCAS();

private:
  SQLiteCAS(StringRef Path);

  std::string getCurrentErrorMessage();

  sqlite3 *db;

  static constexpr const char *FindObjectForIDStmtSQL =
      ("SELECT object FROM objects "
       "WHERE id == ? LIMIT 1;");
  sqlite3_stmt *FindObjectForIDStmt = nullptr;
};

} // namespace llvm::cas

#endif
