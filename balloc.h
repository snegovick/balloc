#ifndef __BALLOC_H__
#define __BALLOC_H__

/*
Set MACHINE_ALIGNMENT to your machines word alignment
Set BXXQTY to needed values
There are two options: if you are living in OS, then you can do the following:

balloc_storage_t *pblc = (balloc_storage_t *)malloc(sizeof(balloc_storage_t));

If you are living outside OS, you can either allocate balloc_storage globally by defining B_ALLOC_STORAGE, or you can just set pointer like  this:
balloc_storage_t *pblc = (balloc_storage_t *)SOME_LOCATION_IN_RAM;

If you want to use system-defined or libc-defined definiton for size_t and ptrdiff_t, then define B_USE_STDDEF;
If you want to use system-defined or libc-defined definiton for printf, then define B_USE_STDIO;
If you want balloc to use system-wide memset, define B_USE_SYSTEM_MEMSET
If you want balloc to use system-wide memcpy, define B_USE_SYSTEM_MEMCPY
Redefine berr as you wish
If you want to have posix-like malloc, free, realloc, then define B_STD_PROCS
To change line ending, define B_LE "\r\n" for example
If you want to have buckets usage statistics, define B_STATISTICS
*/

#ifdef B_USE_STDDEF
#include <stddef.h>
#endif

#define MACHINE_ALIGNMENT 4

enum B_SIZES {
  B16=0,
  B32,
  B64,
  B128,
  B256,
  B512,
  B1024,
  B2048,
  BN_MAX=B2048,
  BN
};

#define B16QTY   128
#define B32QTY   64
#define B64QTY   32
#define B128QTY  16
#define B256QTY  8
#define B512QTY  4
#define B1024QTY 2
#define B2048QTY 1
#define BTOTALQTY (B16QTY+B32QTY+B64QTY+B128QTY+B256QTY+B512QTY+B1024QTY+B2048QTY)

#ifdef B_DEBUG
  #ifndef B_LE
    #define B_LE "\n"
  #endif
  #ifdef B_DBG_MSG
    #define dbg(...) {printf(__VA_ARGS__); printf(B_LE);}
  #else
    #define dbg(...)
  #endif
  #define berr(...) {printf("\r\nBALLOC ERROR: "); printf(__VA_ARGS__); printf(B_LE);}
#else
  #define berr(...)
#endif

typedef struct {
  char busy[BTOTALQTY/8+1];
  char n_busy[BN];
#ifdef B_STATISTICS
  int allocs[BN];
  int frees[BN];
  int reallocs_to[BN];
  int reallocs_from[BN];
#endif
} buck_info_t;

typedef struct __attribute__((aligned(MACHINE_ALIGNMENT))) {
  char storage[16*B16QTY+32*B32QTY+64*B64QTY+128*B128QTY+256*B256QTY+512*B512QTY+1024*B1024QTY+2048*B2048QTY];
  buck_info_t buck_info;
} balloc_storage_t;

#ifndef _SIZE_T
typedef unsigned int size_t;
#endif
#ifndef _PTRDIFF_T
typedef unsigned int ptrdiff_t;
#endif

int balloc_init(void);
void* balloc(size_t);
void bfree(void *);
void *brealloc(void *, size_t);
int balloc_total_buckets_used(void);
int balloc_get_buckets_used(enum B_SIZES);
int balloc_get_size(enum B_SIZES);

#ifdef B_STATISTICS
int balloc_get_buckets_allocs(enum B_SIZES);
int balloc_get_buckets_frees(enum B_SIZES);
int balloc_get_buckets_reallocs_to(enum B_SIZES);
int balloc_get_buckets_reallocs_from(enum B_SIZES);
#endif

#ifdef B_STD_PROCS
void *malloc(size_t);
void free(void *);
void * realloc(void *, size_t);
#endif


#endif/*__BALLOC_H__*/
