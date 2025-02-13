struct Point {
  int x, y;
};

enum CMYK {
  cyan, magenta, yellow, black
};

typedef struct Point TPoint;

union DoubleLongUnion {
  double double_val;
  long long_val;
};

struct WithPointer {
  void *ptr;
  const void *const_ptr;
};
