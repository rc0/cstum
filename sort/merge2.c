/************************************************************************
 * Copyright (c) 2012, 2013 Richard P. Curnow
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************/

/* Play with merge sort, eventually to create one with an inline comparison */

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

static inline uint32_t genrand(int context)
{
  static uint32_t xx[4] = {0xdeadbeef, 0xfee1c001, 0xbeefca6e, 0xc001babe};
  uint32_t x = xx[context];
  x ^= x<<11;
  x ^= x>>21;
  x ^= x<<13;
  xx[context] = x;
  return x;
}

#define THRESH 12

void insertion_right(int i0, int n, uint32_t *d)
{
  int i;
  for (i=1; i<n; i++) {
    int j;
    uint32_t t = d[i0+i];
    for (j=0; j<i; j++) {
      if (t < d[i0+j]) {
        int k;
        for (k=i; k>j; k--) {
          d[i0+k] = d[i0+k-1];
        }
        d[i0+j] = t;
        break;
      }
    }
  }
}

void insertion_left(int i0, int n, int out, uint32_t *d, uint32_t *z)
{
  int i;
  z[out] = d[i0];
  for (i=1; i<n; i++) {
    int j;
    uint32_t t = d[i0+i];
    for (j=0; j<i; j++) {
      if (t < z[out+j]) {
        int k;
        for (k=i; k>j; k--) {
          z[out+k] = z[out+k-1];
        }
        z[out+j] = t;
        goto early;
      }
    }
    z[out+i] = t;
early:
    (void) 0;
  }
}

void inner_mergesort_right(int i0, int n, uint32_t *d, uint32_t *z);

void inner_mergesort_left(int i0, int n, int out, uint32_t *d, uint32_t *z)
{
  int im, nl, nr;
  int xl, xr;
  int op;
  int i;
  if (n <= THRESH) {
    insertion_left(i0, n, out, d, z);
    goto done;
  }

#if 0
  printf("->IM i0=%d n=%d out=%d toz=%d\n", i0, n, out, toz);
#endif

  /* Normal */
  nl = n >> 1; /* left half must be no bigger than right or z buffer overruns */
  nr = n - nl;
  im = nl+i0;
  /* sort the right subarray first */
  inner_mergesort_right(im, nr, d, z);
  inner_mergesort_left(i0, nl, im, d, z);
  for (op=0, xl=0, xr=0; op<n; op++) {
    if (xl < nl) {
      if (xr < nr) {
        uint32_t a = z[im+xl];
        uint32_t b = d[im+xr];
        z[op+out] = (a <= b) ? a : b;
        xl = (a <= b) ? (xl+1) : xl;
        xr = (a <= b) ? xr : (xr+1);
      } else {
        z[op+out] = z[im+xl];
        xl++;
      }
    } else {
      z[op+out] = d[im+xr];
      xr++;
    }
  }
done:
  (void) 0;
#if 0
  printf("<-IM i0=%d n=%d out=%d toz=%d\n", i0, n, out, toz);
  for (i=0; i<n; i++) {
    if (toz) {
      printf("   z[%d]=%08x\n", out+i, z[out+i]);
    } else {
      printf("   d[%d]=%08x\n", i0+i, d[i0+i]);
    }
  }
#endif
  return;
}

void inner_mergesort_right(int i0, int n, uint32_t *d, uint32_t *z)
{
  int im, nl, nr;
  int xl, xr;
  int op;
  int i;
  if (n <= THRESH) {
    insertion_right(i0, n, d);
    goto done;
  }

  /* Normal */
  nl = n >> 1; /* left half must be no bigger than right or z buffer overruns */
  nr = n - nl;
  im = nl+i0;
  /* sort the right subarray first */
  inner_mergesort_right(im, nr, d, z);
  inner_mergesort_left(i0, nl, im, d, z);
  for (op=0, xl=0, xr=0; op<n; op++) {
    if (xl < nl) {
      if (xr < nr) {
        uint32_t a = z[im+xl];
        uint32_t b = d[im+xr];
        d[op+i0] = (a <= b) ? a : b;
        xl = (a <= b) ? (xl+1) : xl;
        xr = (a <= b) ? xr : (xr+1);
      } else {
        d[op+i0] = z[im+xl];
        xl++;
      }
    } else {
      /* left subarray exhausted */
      /* we're done - the rest of the right sub-array is already in-place */
      goto done;
    }
  }
done:
  (void) 0;
  return;
}

void mergesort(int n, uint32_t *d, uint32_t *z)
{
  inner_mergesort_right(0, n, d, z);
}

void insertion(int n, uint32_t *d)
{
  int i;
  for (i=1; i<n; i++) {
    int j;
    uint32_t t = d[i];
    for (j=0; j<i; j++) {
      if (t < d[j]) {
        int k;
        for (k=i; k>j; k--) {
          d[k] = d[k-1];
        }
        d[j] = t;
        break;
      }
    }
  }
}

#ifdef TEST
#define MAX (1<<22)

int main (int argc, char **argv)
{
  int i, n;
  uint32_t *data, *scratch;
  uint32_t sum, sum2;
  int iter;
  iter = 0;
  data = malloc(MAX * sizeof(uint32_t));
  scratch = malloc(MAX * sizeof(uint32_t));
  do {
    iter++;
    n = genrand(0);
#if 1
    n = 1 + (n & ((1<<21) - 1));
#else
    n = 1 + (n & ((1<<6) - 1));
#endif
    sum = 0;
    for (i=0; i<n; i++) {
      data[i] = genrand(0);
      sum += data[i];
#if 0
      printf("i=%d d=%08x\n", i, data[i]);
#endif
    }
    printf("iter=%d n=%d\n", iter, n);
    fflush(stdout);

#if 0
    insertion(n, data);
#else
    mergesort(n, data, scratch);
#endif

#if 0
    printf("After sorting\n");
    for (i=0; i<n; i++) {
      printf("i=%d d=%08x\n", i, data[i]);
    }
#endif
    for (i=1; i<n; i++) {
      if (data[i-1] > data[i]) {
        fprintf(stderr, "Failed ordering iter=%d n=%d i=%d\n", iter, n, i);
        exit(1);
      }
    }
    sum2 = 0;
    for (i=0; i<n; i++) {
      sum2 += data[i];
    }
    if (sum2 != sum) {
      fprintf(stderr, "Failed checksum iter=%d n=%d\n", iter, n);
    }
  } while (1);

  return 0;
}
#endif /* TEST */

#ifdef TIMING
#define SIZE 256
#define NITER 10000000
int main (int argc, char **argv)
{
  int i;
  int do_insertion;
  int n;
  uint32_t data[SIZE], scratch[SIZE];
  int iter;
  if (argc < 2) {
    exit(2);
  }
  n = atoi(argv[1]);
  if (n < 0) {
    do_insertion = 1;
    n = -n;
  } else {
    do_insertion = 0;
  }

  for (iter=0; iter<NITER; iter++) {
    for (i=0; i<n; i++) {
      data[i] = genrand(0);
    }

    if (do_insertion) {
      insertion(n, data);
    } else {
      mergesort(n, data, scratch);
    }
  }
  return 0;
}

#endif


