// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -fdefer-tag-parsing %s

// expected-no-diagnostics

struct StructWithErrorInDef1 {
  nonexistent + nonexistent;
};
void forw(StructWithErrorInDef1 o);
StructWithErrorInDef1 *gv1;

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


template <typename T>
struct TemplWithErrorInDef1 {
  nonexistent + nonexistent;
};
void forw2(TemplWithErrorInDef1<int> o);
TemplWithErrorInDef1<int> *gv2;

template <>
struct TemplWithErrorInDef1<char> {
  nonexistentInSpec + nonexistentInSpec;
};
void forw3(TemplWithErrorInDef1<char> o);
TemplWithErrorInDef1<char> *gv3;


template <typename T>
struct Templ1 {
  int x;
};
template <>
struct Templ1<char> {
  int y;
};
int test4(Templ1<int> *a1, Templ1<char> *a2) {
  return a1->x + a2->y;
}


template <class _Tp, _Tp __v>
struct integral_constant
{
  static constexpr const _Tp value = __v;
};
template <class _Tp, _Tp __v>
constexpr const _Tp integral_constant<_Tp, __v>::value;

template <class _Up>
struct __ignore_t
{
};
constexpr __ignore_t<unsigned char> ignore = __ignore_t<unsigned char>();
