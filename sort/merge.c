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

void inner_mergesort(int i0, int n, int out, int toz, uint32_t *d, uint32_t *z)
{
  int im, nl, nr;
  int xl, xr;
  int op;
  int i;
#if 0
  printf("->IM i0=%d n=%d out=%d toz=%d\n", i0, n, out, toz);
#endif
  if (n == 1) {
    if (toz) {
      z[out] = d[i0];
    }
    goto done;
  }
  if (n == 2) {
    if (d[i0+0] > d[i0+1]) {
      uint32_t t;
      t = d[i0+0];
      if (toz) {
        z[out] = d[i0+1];
        z[out+1] = t;
      } else {
        d[i0] = d[i0+1];
        d[i0+1] = t;
      }
    } else {
      if (toz) {
        z[out]   = d[i0];
        z[out+1] = d[i0+1];
      }
    }
    goto done;
  }

  /* Normal */
  nl = n >> 1; /* left half must be no bigger than right or z buffer overruns */
  nr = n - nl;
  im = nl+i0;
  /* sort the right subarray first */
  inner_mergesort(im, nr, im, 0, d, z);
  inner_mergesort(i0, nl, im, 1, d, z);
  if (toz) {
    for (op=0, xl=0, xr=0; op<n; op++) {
      if (xl < nl) {
        if (xr < nr) {
          if (z[im+xl] <= d[im+xr]) {
            z[op+out] = z[im+xl];
            xl++;
          } else {
            z[op+out] = d[im+xr];
            xr++;
          }
        } else {
          z[op+out] = z[im+xl];
          xl++;
        }
      } else {
        z[op+out] = d[im+xr];
        xr++;
      }
    }
  } else {
    for (op=0, xl=0, xr=0; op<n; op++) {
      if (xl < nl) {
        if (xr < nr) {
          if (z[im+xl] <= d[im+xr]) {
            d[op+i0] = z[im+xl];
            xl++;
          } else {
            d[op+i0] = d[im+xr];
            xr++;
          }
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

void mergesort(int n, uint32_t *d, uint32_t *z)
{
  inner_mergesort(0, n, 0, 0, d, z);
}

#define MAX (1<<21)

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
    n = 1 + (n & ((1<<20) - 1));
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
    mergesort(n, data, scratch);
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
  } while (iter < 100);

  return 0;
}

