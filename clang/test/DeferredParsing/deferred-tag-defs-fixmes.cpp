// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -fdefer-tag-parsing %s

// FIXME: Could we avoid needing to parse this definition?
struct StructWithErrorInDef1 {
  nonexistent x; // expected-error {{unknown type name}}
};
class MyCls1 {
public:
  StructWithErrorInDef1 &SWE;
  MyCls1(StructWithErrorInDef1 &swe) : SWE(swe) {}
  int v = 1;
};
int testRef(StructWithErrorInDef1 &swe) {
  return MyCls1(swe).v;
}
