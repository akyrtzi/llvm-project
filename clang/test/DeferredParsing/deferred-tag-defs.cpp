// RUN: %clang_cc1 -std=c++17 -fsyntax-only -verify -fdefer-tag-parsing %s

class FinalCls final {};
FinalCls fclsgv;

typedef union {
  char c;
} TagFromTypedef;

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

struct Top1 {
  struct Nested1 {
    nonexistent + nonexistent;
  };
  int x;
};
int testNested(Top1 *o) {
  return o->x;
}


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

void tagInFn() {
  struct S {
    nonexistent x; // expected-error {{unknown type name}}
  };
}

template <typename T>
class Templ;
template <typename T>
class Templ {
  typedef T SomeTy;
  void meth(SomeTy o);
};
template <typename T>
void Templ<T>::meth(SomeTy o) {}

template <>
class Templ<char>;
template <>
class Templ<char> {
  typedef char SomeTy;
  void meth(SomeTy o);
};
void Templ<char>::meth(SomeTy o) {}


class OptionalStorage {
  union {
    char empty;
    int value;
  };
public:
  OptionalStorage() : empty{} {}
};
OptionalStorage optSV;

namespace ns {
namespace mycls {
  class Cls {};
}
}
namespace ns {
  mycls::Cls cc;
}

namespace NS {
  template <typename T> class Expected;
  class Error {
    template <typename T> friend class Expected;
  };
  void cantFail(Error Err) {}
}

template <class T> T &makeCheck();
template <class NodeT>
class iplist_impl {
public:
  using iterator = NodeT*;
private:
  decltype(&makeCheck<NodeT>()) somefield;
};
template <class T>
class SymbolTableList: public iplist_impl<T> {};
class Instruction {
  void moveBefore(SymbolTableList<Instruction>::iterator I);
};
SymbolTableList<Instruction> stlV;

class MCRegisterInfo {
public:
  class DiffListIterator {
    unsigned short Val = 0;
  };
  class mc_difflist_iterator {
    MCRegisterInfo::DiffListIterator Iter;
  };
  class mc_subreg_iterator : public mc_difflist_iterator {
  };
  void subregs() const {
    mc_subreg_iterator();
  }
};
class MCSuperRegIterator : public MCRegisterInfo::DiffListIterator {
};
MCSuperRegIterator MCSRGV;

class ArenaAllocator {
  struct AllocatorNode {
    char *Buf = nullptr;
  };
  void addNode() { new AllocatorNode; }
};
ArenaAllocator AAGV;

class APFloat {
  struct Storage {
    void innerMeth() {
      outerMeth();
    }
  } U;
  static void outerMeth();
};
APFloat APFGV;

class ValueLatticeElement {
public:
  struct MergeOptions {
    MergeOptions() : MergeOptions(false) {}
    MergeOptions(bool MayIncludeUndef, unsigned MaxWidenSteps = 1);
  };
  bool markConstantRange(MergeOptions Opts = MergeOptions());
};
ValueLatticeElement VLEGV;


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

template <class ..._Tp>
class tuple {};
template <class _T1, class _T2>
struct pair {
  template <class... _Args1, class... _Args2>
  pair(tuple<_Args1...>& __first_args, tuple<_Args2...>& __second_args);
};
template <class _T1, class _T2>
template <class... _Args1, class... _Args2>
inline
pair<_T1, _T2>::pair(tuple<_Args1...>& __first_args, tuple<_Args2...>& __second_args)
{
}

template <class _Tp> struct decay {
  public: typedef _Tp type;
};
template <class _Tp> struct decay;
template <class _Tp> struct __common_type2_imp {
 typedef typename decay<_Tp>::type type;
};
template <class _Duration1> typename __common_type2_imp<_Duration1>::type
callme(const _Duration1& a);
auto const __elapsed = callme(0);

struct __is_referenceable_impl {
  template <class _Tp> void test(_Tp);
};
template <class _Tp> struct __is_referenceable : __is_referenceable_impl {};
__is_referenceable<int> gvisref;
