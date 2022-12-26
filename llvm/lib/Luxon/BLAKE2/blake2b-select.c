#if __x86_64__
#include "sse/blake2b.c"
#elif __arm64__
#include "neon/blake2b.c"
#else
#include "ref/blake2b.c"
#endif
