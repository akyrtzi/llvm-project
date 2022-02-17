// RUN: %clang_cc1 -std=c++17 -emit-llvm-only -debug-info-kind=standalone -fdefer-tag-parsing -fdefer-body-parsing %s

namespace llvm {
class BumpPtrAllocator {
  template <typename T> friend class SpecificBumpPtrAllocator;
};
template <typename T> class SpecificBumpPtrAllocator;
template <typename NodeT> class DominatorTreeBase {
  void findNearestCommonDominator() { BumpPtrAllocator Allocator; }
};
template class DominatorTreeBase<int>;
} // namespace llvm
