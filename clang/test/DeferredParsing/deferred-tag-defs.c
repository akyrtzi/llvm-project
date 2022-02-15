// RUN: %clang_cc1 -fsyntax-only -verify -fdefer-tag-parsing %s

// expected-no-diagnostics

struct StructWithErrorInDef1 {
  nonexistent + nonexistent;
};
void forw(struct StructWithErrorInDef1 o);
struct StructWithErrorInDef1 *gv1;
void test1(struct StructWithErrorInDef1 *p) {
  struct StructWithErrorInDef1 *v = 0;
}

struct MyS1 {
  int x;
};
int test2(struct MyS1 *o) {
  return o->x;
}
