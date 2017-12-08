#include "balloc.h"
#ifdef B_USE_STDIO
#include <stdio.h>
#endif

#define MIN2(a, b) (a<b?a:b)

#define B_OFS_16   0
#define B_OFS_32   (B_OFS_16+16*B16QTY)
#define B_OFS_64   (B_OFS_32+32*B32QTY)
#define B_OFS_128  (B_OFS_64+64*B64QTY)
#define B_OFS_256  (B_OFS_128+128*B128QTY)
#define B_OFS_512  (B_OFS_256+256*B256QTY)
#define B_OFS_1024 (B_OFS_512+512*B512QTY)
#define B_OFS_2048 (B_OFS_1024+1024*B1024QTY)

#define B_BUSY_OFS_16 (0)
#define B_BUSY_OFS_32 (B_BUSY_OFS_16+B16QTY)
#define B_BUSY_OFS_64 (B_BUSY_OFS_32+B32QTY)
#define B_BUSY_OFS_128 (B_BUSY_OFS_64+B64QTY)
#define B_BUSY_OFS_256 (B_BUSY_OFS_128+B128QTY)
#define B_BUSY_OFS_512 (B_BUSY_OFS_256+B256QTY)
#define B_BUSY_OFS_1024 (B_BUSY_OFS_512+B512QTY)
#define B_BUSY_OFS_2048 (B_BUSY_OFS_1024+B1024QTY)

static const int b_sizes[BN] = {16, 32, 64, 128, 256, 512, 1024, 2048};
static const int b_qtys[BN] = {B16QTY, B32QTY, B64QTY, B128QTY, B256QTY, B512QTY, B1024QTY, B2048QTY};
static const int b_busy_ofs[BN] = {B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16, B_BUSY_OFS_16};
static const int b_offsets[BN] = {B_OFS_16,
                                  B_OFS_32,
                                  B_OFS_64,
                                  B_OFS_128,
                                  B_OFS_256,
                                  B_OFS_512,
                                  B_OFS_1024,
                                  B_OFS_2048};

#ifdef B_ALLOC_STORAGE
balloc_storage_t blc;
balloc_storage_t *pblc = &blc;
#else
extern balloc_storage_t *pblc;
#endif

#ifndef NULL
#define NULL (void*)0
#endif

#ifndef B_USE_SYSTEM_MEMSET
static void*
balloc_memset(void *ap, int c, unsigned int n) {
	char *p;
	int m = (int)n;

	p = ap;
	while(m > 0) {
		*p++ = c;
		m--;
	}
	return ap;
}
#else
#define balloc_memset memset
#endif

#ifndef B_USE_SYSTEM_MEMCPY
static void *
balloc_memcpy(void *dest, const void *src, size_t n) {
  char *dptr = (char*)dest;
  char *sptr = (char*)src;

  for (; n>0; n--, sptr++, dptr++) {
    *dptr=*sptr;
  }
  return dest;
}
#else
#define balloc_memcpy memcpy
#endif


int
balloc_init(void) {
  balloc_memset(pblc, 0, sizeof(balloc_storage_t));
  return 0;
}

//get busy bit for bucket number n(n<b_sizes[s]) of size s (B_SIZES)
/* static int */
/* balloc_get_busy_bit(enum B_SIZES s, int n) { */
/*   int offt = b_busy_ofs[s]+n; */
/*   int byte = offt/8; */
/*   int bit = offt%8; */
/*   int busy = pblc->buck_info.busy[byte]&(1<<bit); */
/*   return (busy==0?0:1); */
/* } */

static int
balloc_set_busy_bit(enum B_SIZES s, int n) {
  int offt = b_busy_ofs[s]+n;
  int byte = offt/8;
  int bit = offt%8;
  int busy = pblc->buck_info.busy[byte]&(1<<bit);
  if (busy) {
    berr("Trying to set busy bucket busy again, this is like bad kinda, mmkay? Dont do this");
    return -1;
  }
  pblc->buck_info.busy[byte] |= (1<<bit);
  return 0;
}

static int
balloc_unset_busy_bit(enum B_SIZES s, int n) {
  int offt = b_busy_ofs[s]+n;
  int byte = offt/8;
  int bit = offt%8;
  int busy = pblc->buck_info.busy[byte]&(1<<bit);
  if (!busy) {
    berr("Trying to unset busy flag for free bucket");
    return -1;
  }
  pblc->buck_info.busy[byte] &= ~(1<<bit);
  return 0;
}

static int
balloc_find_free_bit(enum B_SIZES s) {
  int i = 0;
  if (pblc->buck_info.n_busy[s] >= b_qtys[s]) {
    return -2;
  }
  for (;i<b_qtys[s];i++) {
    int offt = b_busy_ofs[s]+i;
    int byte = offt/8;
    int bit = offt%8;
    int busy = pblc->buck_info.busy[byte]&(1<<bit);
    if (!busy) {
      return bit;
    }
  }
  berr("Free bit not found even though n_busy reports free buckets [%i/%i]", pblc->buck_info.n_busy[s], b_qtys[s]);
  return -1;
}

static void*
balloc_get_bucket_ptr(enum B_SIZES s, int n) {
  int offt = b_offsets[s];
  int offt2 = n*b_sizes[s];
  void *ptr = &pblc->storage[offt+offt2];
  return ptr;
}

static int
balloc_get_bucket_from_ptr(void *ptr, enum B_SIZES *s, int *n) {
  char *cptr = (char *)ptr;
  char *sptr = (char *)(pblc->storage);
  int i;
  int r;
  ptrdiff_t diff = 0;

  for (i=BN_MAX;i>=0;i--) {
    if (cptr>=(sptr+b_offsets[i])) {
      diff = (ptrdiff_t)(cptr - (sptr+b_offsets[i]));
      r = diff/(b_sizes[i]);
      if (r>b_qtys[i]) {
        berr("Pointer %p is out of upper bound %p", ptr, sptr+b_offsets[BN_MAX]);
        return -1;
      }
      *s = i;
      *n = r;
      return 0;
    }
  }
  berr("Pointer %p is out of lower bound %p", ptr, sptr);
  return -2;
}

void*
balloc(size_t size) {
  int i = 0;
  int bn = 0;

  dbg("balloc %li", size);
  for (i=0;i<BN;i++) {
    if (size<=((size_t)b_sizes[i])) {
      bn = balloc_find_free_bit(i);
      if (bn>=0) {
        dbg("Allocating bucket of size %i\n", b_sizes[i]);
        balloc_set_busy_bit(i, bn);
        pblc->buck_info.n_busy[i] += 1;
#ifdef B_STATISTICS
        pblc->buck_info.allocs[i]++;
#endif
        return balloc_get_bucket_ptr(i, bn);
      } else {
        berr("No more %iB buckets, consider increasing num of these", b_sizes[i]);
      }
    }
  }
  return NULL;
}

static inline void
bfree_ex(void *ptr, int is_realloc) {
  int r = 0;
  enum B_SIZES s;
  int n;
  void *cptr;
  r = balloc_get_bucket_from_ptr(ptr, &s, &n);
  if (r == 0) {
    cptr = balloc_get_bucket_ptr(s, n);
    if (cptr != ptr) {
      berr("Pointer %p is missaligned relative to bucket %p", ptr, cptr);
      return;
    }
    balloc_unset_busy_bit(s, n);
    pblc->buck_info.n_busy[s] -= 1;
#ifdef B_STATISTICS
    if (!is_realloc) {
      pblc->buck_info.frees[s]++;
    }
#endif

  }
}

void *
brealloc(void *ptr, size_t new_size) {
  int r;
  enum B_SIZES s;
  int n;
  void *cptr;
  enum B_SIZES i;
  int bn = 0;
  void *nptr;
  dbg("brealloc %li\n", new_size);

  if (ptr == NULL) {
    return balloc(new_size);
  }
  r = balloc_get_bucket_from_ptr(ptr, &s, &n);
  if (r == 0) {
    cptr = balloc_get_bucket_ptr(s, n);
    if (cptr != ptr) {
      berr("Pointer %p is missaligned relative to bucket %p", ptr, cptr);
      return NULL;
    }
    if (new_size <= ((size_t)b_sizes[s])) {
      dbg("New size is smaller than current bucket\n");
      //if new size is smaller than currently allocated bucket, there
      //are two posibilities:
      //1. the size is bigger than previously allocated
      //2. the size is smaller than previously allocated
      //anyhow, we dont store requested size, so just check if this data
      //can be reallocated in smaller bucket
      for (i=0;i<s;i++) {
        dbg("Checking bucket size %i/%li\n", b_sizes[i], new_size);
        if (new_size<=((size_t)b_sizes[i])) {
          dbg("This bucket size fits\n");
          //smaller bucket fits
          bn = balloc_find_free_bit(i);
          if (bn>=0) {
            balloc_set_busy_bit(i, bn);
            pblc->buck_info.n_busy[i] += 1;
            nptr = balloc_get_bucket_ptr(i, bn);
            if (nptr == NULL) {
              return NULL;
            }
            balloc_memcpy(nptr, ptr, new_size);
            bfree_ex(ptr, 1);
#ifdef B_STATISTICS
            pblc->buck_info.reallocs_to[i]++;
            pblc->buck_info.reallocs_from[s]++;
#endif
            return nptr;
          } else {
            berr("No more %iB buckets, consider increasing num of these", b_sizes[i]);
            //no free smaller bucket
#ifdef B_STATISTICS
            pblc->buck_info.reallocs_to[i]++;
            pblc->buck_info.reallocs_from[s]++;
#endif
            return ptr;
          }
        }
      }
      return ptr;
    } else {
      nptr = balloc(new_size);
      if (nptr == NULL) {
        return NULL;
      }
      balloc_memcpy(nptr, ptr, b_sizes[s]);
      bfree(ptr);
#ifdef B_STATISTICS
      pblc->buck_info.reallocs_from[s]++;
      balloc_get_bucket_from_ptr(nptr, &s, &n);
      pblc->buck_info.reallocs_to[s]++;
#endif
      return nptr;
    }
  }
  return NULL;
}

void
bfree(void *ptr) {
  bfree_ex(ptr, 0);
}

int
balloc_total_buckets_used(void) {
  enum B_SIZES s = 0;
  int busy = 0;
  for (;s<BN;s++) {
    busy+=pblc->buck_info.n_busy[s];
  }
  return busy;
}

int
balloc_get_buckets_used(enum B_SIZES s) {
  return pblc->buck_info.n_busy[s];
}

int
balloc_get_size(enum B_SIZES s) {
  return b_sizes[s];
}

#ifdef B_STATISTICS
int
balloc_get_buckets_allocs(enum B_SIZES s) {
  return pblc->buck_info.allocs[s];
}

int
balloc_get_buckets_frees(enum B_SIZES s) {
  return pblc->buck_info.frees[s];
}

int
balloc_get_buckets_reallocs_to(enum B_SIZES s) {
  return pblc->buck_info.reallocs_to[s];
}

int
balloc_get_buckets_reallocs_from(enum B_SIZES s) {
  return pblc->buck_info.reallocs_from[s];
}
#endif

#ifdef B_STD_PROCS
void *
malloc(size_t size) {
  return balloc(size);
}

void
free(void *ptr) {
  bfree(ptr);
}

void *
realloc(void *ptr, size_t new_size) {
  return brealloc(ptr, new_size);
}
#endif
