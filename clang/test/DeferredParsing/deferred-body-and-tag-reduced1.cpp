// RUN: %clang_cc1 -std=c++17 -emit-llvm-only -verify -fdefer-body-parsing -fdefer-tag-parsing -w %s

// expected-no-diagnostics

typedef unsigned long size_t;
namespace std {
template <class _T1, class _T2> struct pair {
  _T1 first;
  _T2 second;
  pair();
};
template <class _T1, class _T2> pair<_T1, _T2>::pair(){};
class string {};
template <class _Tp> class __tree_iterator {};
template <class _Tp> class __tree {
public:
  typedef __tree_iterator<_Tp> iterator;
  template <class _Key, class... _Args>
  pair<iterator, bool> __emplace_unique_key_args(_Key const &,
                                                 _Args &&...__args);
};
template <class _Tp>
template <class _Key, class... _Args>
pair<typename __tree<_Tp>::iterator, bool>
__tree<_Tp>::__emplace_unique_key_args(_Key const &__k, _Args &&...__args) {}
template <class _Key, class _Tp> class map {
public:
  typedef _Key key_type;
  typedef _Tp mapped_type;

private:
  typedef pair<key_type, mapped_type> __value_type;
  typedef __tree<__value_type> __base;
  __base __tree_;

public:
  void operator[](const key_type &__k);
};
template <class _Key, class _Tp>
void map<_Key, _Tp>::operator[](const key_type &__k) {
  __tree_.__emplace_unique_key_args(__k);
}
} // namespace std
namespace llvm {
class StringRef {
public:
  constexpr StringRef(const char *Str);
  explicit operator std::string() const;
};
template <class T> class UniqueVector {
private:
  std::map<T, unsigned> Map;

public:
  unsigned insert(const T &Entry) { Map[Entry]; }
};
class DebugCounter {
public:
  static DebugCounter &instance();
  static unsigned registerCounter(StringRef Name, StringRef Desc) {
    return instance().addCounter(std::string(Name), std::string(Desc));
  }
  typedef UniqueVector<std::string> CounterVector;
  unsigned addCounter(const std::string &Name, const std::string &Desc) {
    unsigned Result = RegisteredCounters.insert(Name);
  }
  CounterVector RegisteredCounters;
};
} // namespace llvm
static const unsigned AssumeQueryCounter = llvm::DebugCounter::registerCounter(
    "assume-queries-counter", "Controls which assumes gets created");
