#ifndef LLVM_LUXON_LMDBCAS_H
#define LLVM_LUXON_LMDBCAS_H

#include "llvm-c/lmdb.h"
#include "llvm/Support/Error.h"

namespace llvm::cas {

class LMDBCAS {
public:
  using DataID = std::array<uint8_t, 65>;
  using DataRef = ArrayRef<uint8_t>;

  struct CASObject {
    StringRef Data;
    SmallVector<DataRef, 32> Refs;
  };

  Expected<DataID> parseID(StringRef ID);

  Expected<Optional<CASObject>> get(DataRef ID);

  static Expected<std::unique_ptr<LMDBCAS>> create(StringRef FilePath);
  ~LMDBCAS();

private:
  LMDBCAS(StringRef Path);

  MDB_env *DBEnv;
  MDB_dbi DBHandle;
};

} // namespace llvm::cas

#endif
