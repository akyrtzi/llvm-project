// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -fdefer-tag-parsing %s

// expected-no-diagnostics

struct StructWithErrorInDef1 {
  nonexistent + nonexistent;
};
void forw(StructWithErrorInDef1 o);

struct MyS1 {
  typedef int Inner;
  int x;
};
void test1(MyS1::Inner o);

struct MyS2 {
  int x;
};
void test2(MyS2 *o) {}

struct MyS3 {
  int x;
};
int test3(MyS3 *o) {
  return o->x;
}
