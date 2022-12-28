// Symbols are prefixed with 'llvm' to avoid a potential conflict with
// another BLAKE2 version within the same program.

#define blake2s_init llvm_blake2s_init
#define blake2s_init_key llvm_blake2s_init_key
#define blake2s_init_param llvm_blake2s_init_param
#define blake2s_update llvm_blake2s_update
#define blake2s_final llvm_blake2s_final
#define blake2b_init llvm_blake2b_init
#define blake2b_init_key llvm_blake2b_init_key
#define blake2b_init_param llvm_blake2b_init_param
#define blake2b_update llvm_blake2b_update
#define blake2b_final llvm_blake2b_final
#define blake2sp_init llvm_blake2sp_init
#define blake2sp_init_key llvm_blake2sp_init_key
#define blake2sp_update llvm_blake2sp_update
#define blake2sp_final llvm_blake2sp_final
#define blake2bp_init llvm_blake2bp_init
#define blake2bp_init_key llvm_blake2bp_init_key
#define blake2bp_update llvm_blake2bp_update
#define blake2bp_final llvm_blake2bp_final
#define blake2xs_init llvm_blake2xs_init
#define blake2xs_init_key llvm_blake2xs_init_key
#define blake2xs_update llvm_blake2xs_update
#define blake2xs_final llvm_blake2xs_final
#define blake2xb_init llvm_blake2xb_init
#define blake2xb_init_key llvm_blake2xb_init_key
#define blake2xb_update llvm_blake2xb_update
#define blake2xb_final llvm_blake2xb_final
#define blake2s llvm_blake2s
#define blake2b llvm_blake2b
#define blake2sp llvm_blake2sp
#define blake2bp llvm_blake2bp
#define blake2xs llvm_blake2xs
#define blake2xb llvm_blake2xb

#if __x86_64__
#include "sse/blake2b.c"
#elif __arm64__
#include "neon/blake2b.c"
#else
#include "ref/blake2b.c"
#endif
