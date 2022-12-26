#ifndef LLVM_LUXON_LUXONBASE_H
#define LLVM_LUXON_LUXONBASE_H

#include "llvm/CAS/CASID.h"

namespace llvm::cas {

class LuxonCASBaseContext : public CASContext {
  void printIDImpl(raw_ostream &OS, const CASID &ID) const final;
  void anchor() override;

public:
  static StringRef getHashName() { return "BLAKE2b"; }
  StringRef getHashSchemaIdentifier() const final {
    static const std::string ID =
        ("llvm.cas.luxon.v1[" + getHashName() + "]").str();
    return ID;
  }

  static const LuxonCASBaseContext &getDefaultContext();

  LuxonCASBaseContext() = default;
};

} // namespace llvm::cas

#endif
