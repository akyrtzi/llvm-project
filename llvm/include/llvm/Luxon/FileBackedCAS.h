#ifndef LLVM_LUXON_FILEBACKEDCAS_H
#define LLVM_LUXON_FILEBACKEDCAS_H

#include "llvm/Support/Error.h"

namespace llvm::cas {

class FileBackedCAS {
public:
  using DataID = std::array<uint8_t, 65>;

  struct CASObject {
    SmallString<32> Data;
    SmallVector<DataID, 16> Refs;
  };

  static void printID(const DataID &ID, raw_ostream &OS);
  Expected<DataID> parseID(StringRef ID);

  Expected<Optional<CASObject>> get(const DataID &ID);

  static Expected<std::unique_ptr<FileBackedCAS>> create(StringRef FilePath);

private:
  FileBackedCAS(StringRef Path) : Path(Path) {}

  void getFilenameForID(const DataID &ID, StringRef Prefix,
                        SmallVectorImpl<char> &PathBuf);

  Expected<SmallVector<DataID, 16>> readRefs(const DataID &ID);

  SmallString<128> Path;
};

} // namespace llvm::cas

#endif
