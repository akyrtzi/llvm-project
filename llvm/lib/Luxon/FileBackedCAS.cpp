//===- FileBackedCAS.cpp ----------------------------------------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Luxon/FileBackedCAS.h"
#include "LuxonBase.h"
#include "llvm/Support/MemoryBuffer.h"

using namespace llvm;
using namespace llvm::cas;

using DataID = FileBackedCAS::DataID;
using CASObject = FileBackedCAS::CASObject;

static constexpr StringLiteral DirComponent1 = "2nd2last-";
static constexpr StringLiteral DirComponent2 = "last-";

void FileBackedCAS::getFilenameForID(const DataID &ID, StringRef Prefix,
                                     SmallVectorImpl<char> &PathBuf) {
  ArrayRef<uint8_t> IDRef = makeArrayRef(ID);
  raw_svector_ostream OS(PathBuf);

  auto addHex = [&](uint8_t val) {
    SmallString<2> HexBuf;
    toHex(val, /*LowerCase=*/true, HexBuf);
    StringRef Hex = HexBuf;
    if (Hex.front() == '0')
      Hex = Hex.drop_front();
    OS << Hex;
  };

  OS << Path;

  OS << '/';
  OS << DirComponent1;
  addHex(IDRef.drop_back().back());

  OS << '/';
  OS << DirComponent2;
  addHex(IDRef.back());

  OS << '/';
  OS << Prefix;
  LuxonCASContext::printID(OS, IDRef);
}

static unsigned readInt(ArrayRef<uint8_t> Bytes, unsigned Offset,
                        unsigned &Value) {
  unsigned Length = Bytes[Offset] & 0x7f;

  if ((Bytes[Offset] & 0x80) == 0) {
    Value = Length;
    return 1;
  }

  assert(Length >= 1 && Length <= 4);
  assert(Bytes.size() >= Offset + 1 + Length);

  unsigned V = 0;

  for (unsigned I = 1; I <= Length; ++I) {
    V <<= 8;
    V += Bytes[Offset + I];
  }

  Value = V;
  return 1 + Length;
}

template <typename T> static T fromRawBytes(ArrayRef<uint8_t> rawBytes);

template <> DataID fromRawBytes(ArrayRef<uint8_t> rawBytes) {
  assert(rawBytes.size() == sizeof(DataID));
  DataID ID;
  std::uninitialized_copy(rawBytes.begin(), rawBytes.end(), ID.data());
  return ID;
}

template <typename T>
static void decode(ArrayRef<uint8_t> rawBytes, SmallVectorImpl<T> &Vec) {
  unsigned arrayElementsCount = 0;
  unsigned offset = 0;
  offset += readInt(rawBytes, offset, arrayElementsCount);

  Vec.reserve(arrayElementsCount);
  for (unsigned I = 0; I != arrayElementsCount; ++I) {
    unsigned countBytes = 0;
    unsigned countLength = readInt(rawBytes, offset, countBytes);
    offset += countLength;
    assert(countBytes <= rawBytes.size() - offset);
    Vec.push_back(fromRawBytes<T>(rawBytes.slice(offset, countBytes)));
    offset += countBytes;
  }
}

void FileBackedCAS::printID(const DataID &ID, raw_ostream &OS) {
  LuxonCASContext::printID(OS, ID);
}

Expected<DataID> FileBackedCAS::parseID(StringRef ID) {
  return LuxonCASContext::rawParseID(ID);
}

Expected<Optional<CASObject>> FileBackedCAS::get(const DataID &ID) {
  auto Refs = readRefs(ID);
  if (!Refs)
    return Refs.takeError();

  SmallString<256> PathBuf;
  getFilenameForID(ID, "data.", PathBuf);

  ErrorOr<std::unique_ptr<MemoryBuffer>> FileBuf =
      MemoryBuffer::getFile(PathBuf);
  if (!FileBuf)
    return createStringError(FileBuf.getError(),
                             "failed reading '" + PathBuf +
                                 "': " + FileBuf.getError().message());

  return CASObject{(*FileBuf)->getBuffer(), std::move(*Refs)};
}

Expected<SmallVector<DataID, 16>> FileBackedCAS::readRefs(const DataID &ID) {
  SmallString<256> PathBuf;
  getFilenameForID(ID, "refs.", PathBuf);

  ErrorOr<std::unique_ptr<MemoryBuffer>> FileBuf =
      MemoryBuffer::getFile(PathBuf);
  if (!FileBuf)
    return createStringError(FileBuf.getError(),
                             "failed reading '" + PathBuf +
                                 "': " + FileBuf.getError().message());

  SmallVector<DataID, 16> Refs;
  decode(arrayRefFromStringRef((*FileBuf)->getBuffer()), Refs);
  return Refs;
}

Expected<std::unique_ptr<FileBackedCAS>>
FileBackedCAS::create(StringRef FilePath) {
  std::unique_ptr<FileBackedCAS> Ptr;
  Ptr.reset(new FileBackedCAS(FilePath));
  return std::move(Ptr);
}
