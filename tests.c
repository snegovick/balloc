#include <stddef.h>
#include "balloc.h"
#include <stdio.h>
#include <stdlib.h>

balloc_storage_t *pblc;

int
test_alloc_free(void) {
  void *p1;
  void *p2;
  void *p3;

  p1 = balloc(8);
  if (p1 == NULL) {
    berr("p1 ptr is null");
    return -1;
  }
  p2 = balloc(128);
  if (p2 == NULL) {
    berr("p2 ptr is null");
    return -2;
  }
  p3 = balloc(32);
  if (p3 == NULL) {
    berr("p3 ptr is null");
    return -3;
  }

  bfree(p1);
  bfree(p2);
  bfree(p3);

  return 0;
}

int
test_count_used(void) {
  void *p1;
  void *p2;
  void *p3;
  int r;

  p1 = balloc(8);
  if (p1 == NULL) {
    berr("p1 ptr is null");
    return -1;
  }
  p2 = balloc(128);
  if (p2 == NULL) {
    berr("p2 ptr is null");
    return -2;
  }
  p3 = balloc(32);
  if (p3 == NULL) {
    berr("p3 ptr is null");
    return -3;
  }

  r = balloc_total_buckets_used();
  if (r!=3) {
    berr("Total buckets used is not 3 [%i]", r);
    return -4;
  }

  bfree(p1);
  bfree(p2);
  bfree(p3);

  r = balloc_total_buckets_used();
  if (balloc_total_buckets_used()!=0) {
    berr("Total buckets used is not 0 [%i]", r);
    return -5;
  }

  return 0;
}

int
fill_compare(char *ptr, int len, int compare) {
  int i = 0;
  for (;i<len;i++) {
    if (!compare) {
      ptr[i] = (char)i;
    } else {
      if (ptr[i] != (char)i) {
        return -1;
      }
    }
  }
  return 0;
}

int
test_fill(void) {
  void *p1;
  void *p2;
  void *p3;

  p1 = balloc(8);
  if (p1 == NULL) {
    berr("p1 ptr is null");
    return -1;
  }
  p2 = balloc(128);
  if (p2 == NULL) {
    berr("p2 ptr is null");
    return -2;
  }
  p3 = balloc(32);
  if (p3 == NULL) {
    berr("p3 ptr is null");
    return -3;
  }

  fill_compare(p1, 8, 0);
  fill_compare(p2, 128, 0);
  fill_compare(p3, 32, 0);
  if (fill_compare(p1, 8, 1)!=0) {
    berr("p1 comparrison failed");
    return -4;
  }
  if (fill_compare(p2, 128, 1)) {
    berr("p2 comparrison failed");
    return -5;
  }
  if (fill_compare(p3, 32, 1)) {
    berr("p3 comparrison failed");
    return -6;
  }

  bfree(p1);
  bfree(p2);
  bfree(p3);

  return 0;
}

int
test_realloc(void) {
  void *newp;
  void *p = balloc(30);
  if (p == NULL) {
    berr("p is null");
    return -1;
  }

  fill_compare(p, 30, 0);
  newp = brealloc(p, 32);
  if (newp == NULL) {
    berr("newp is null");
    return -2;
  }
  if (newp != p) {
    berr("newp is not p");
    return -3;
  }
  if (fill_compare(p, 30, 1) != 0) {
    berr("comparrison failed p");
    return -4;
  }

  p = brealloc(newp, 8);
  if (p == NULL) {
    berr("realloc p is null");
    return -5;
  }
  if (p == newp) {
    berr("realloc p is newp");
    return -7;
  }
  if (fill_compare(p, 8, 1) != 0) {
    berr("realloc p comparrison failed");
    return -8;
  }
  bfree(p);
  return 0;
}

int
main(void) {
  int ctr = 0;
  int ok = 0;
  int failed = 0;
  enum B_SIZES s = 0;
  pblc = malloc(sizeof(balloc_storage_t));
  balloc_init();
  printf("Test alloc free ");
  if (test_alloc_free()!=0) {
    failed++;
    printf("FAILED\n");
  } else {
    ok++;
    printf("OK\n");
  }
  ctr++;

  printf("Test fill ");
  if (test_alloc_free()!=0) {
    failed++;
    printf("FAILED\n");
  } else {
    ok++;
    printf("OK\n");
  }
  ctr++;

  printf("Test realloc ");
  if (test_realloc()!=0) {
    failed++;
    printf("FAILED\n");
  } else {
    ok++;
    printf("OK\n");
  }
  ctr++;

  printf("Test count used ");
  if (test_count_used()!=0) {
    failed++;
    printf("FAILED\n");
  } else {
    ok++;
    printf("OK\n");
  }
  ctr++;

  printf("Executed %i tests, %i FAILED, %i PASSED\n", ctr, failed, ok);

  printf("Staistics:\n");
  printf("%-15s%-15s%-15s%-15s%-15s\n", "Size", "Allocs", "Frees", "Realloc to", "Realloc from");
  for (;s<BN;s++) {
    printf("%-15i%-15i%-15i%-15i%-15i\n", balloc_get_size(s), balloc_get_buckets_allocs(s), balloc_get_buckets_frees(s), balloc_get_buckets_reallocs_to(s), balloc_get_buckets_reallocs_from(s));
  }
  
  return 0;
}
