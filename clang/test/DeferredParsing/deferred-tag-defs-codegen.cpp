// RUN: %clang_cc1 -std=c++17 -fsyntax-only -emit-llvm-only -debug-info-kind=standalone -fdefer-tag-parsing %s

struct StructWithErrorInDef1 {
  nonexistent + nonexistent;
};
class MyCls1 {
public:
  StructWithErrorInDef1 &SWE;
  MyCls1(StructWithErrorInDef1 *swe) : SWE(*swe) {}
  int v = 1;
};
int test1(StructWithErrorInDef1 *swe) {
  return MyCls1(swe).v;
}

typedef unsigned long size_t;
namespace llvm {
class StringRef {
private:
  const char *Data;
public:
  constexpr StringRef(const char *Str) : Data(Str) {}
};
bool ParseCommandLineOptions(StringRef Overview = "");
void apply(const char * M) {
  StringRef s = M;
}
} // namespace llvm
