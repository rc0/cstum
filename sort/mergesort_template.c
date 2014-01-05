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

/*
 * Assume the following are defined in the including file:
 *   SORT_FUNCTION_NAME : the name of the callable sorting function
 *   TYPE : the type of the array members
 *   int SORT_COMPARE_NAME(const TYPE *, const TYPE *) : a compare function a la strcmp
 *   */

#include <string.h>

#define INSERTION_THRESHOLD 12

static void insertion_right(int i0, int n, TYPE *d)
{
  int i;
  int status;
  for (i=1; i<n; i++) {
    int k;
    int H, L;
    TYPE t = d[i0+i];
    L = 0, H = i;
    while (L < H) {
      int M = (L + H)>>1;
      status = SORT_COMPARE_NAME(&t, d+i0+M);
      if (status >= 0) L = M + 1;
      else H = M;
    }

    for (k=i; k>L; k--) {
      d[i0+k] = d[i0+k-1];
    }
    d[i0+L] = t;
  }
}

static void insertion_left(int i0, int n, int z0, TYPE *d, TYPE *z)
{
  int i;
  int status;
  z[z0] = d[i0];
  for (i=1; i<n; i++) {
    int k;
    int H, L;
    TYPE t = d[i0+i];
    L = 0, H = i;
    while (L < H) {
      int M = (L + H) >> 1;
      status = SORT_COMPARE_NAME(&t, z+z0+M);
      if (status >= 0) L = M + 1;
      else H = M;
    }
    for (k=i; k>L; k--) {
      z[z0+k] = z[z0+k-1];
    }
    z[z0+L] = t;
  }
}

static void inner_mergesort_right(int i0, int n, TYPE *d, TYPE *z);

static void inner_mergesort_left(int i0, int n, int z0, TYPE *d, TYPE *z)
{
  int im, nl, nr;
  int xl, xr;
  int op;
  if (n <= INSERTION_THRESHOLD) {
    insertion_left(i0, n, z0, d, z);
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
        TYPE a = z[im+xl];
        TYPE b = d[im+xr];
        int status = SORT_COMPARE_NAME(&a, &b);
        z[op+z0] = (status <= 0) ? a : b;
        xl = (status <= 0) ? (xl+1) : xl;
        xr = (status <= 0) ? xr : (xr+1);
      } else {
        z[op+z0] = z[im+xl];
        xl++;
      }
    } else {
      z[op+z0] = d[im+xr];
      xr++;
    }
  }
done:
  (void) 0;
  return;
}

static void inner_mergesort_right(int i0, int n, TYPE *d, TYPE *z)
{
  int im, nl, nr;
  int xl, xr;
  int op;
  if (n <= INSERTION_THRESHOLD) {
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
        TYPE a = z[im+xl];
        TYPE b = d[im+xr];
        int status = SORT_COMPARE_NAME(&a, &b);
        d[op+i0] = (status <= 0) ? a : b;
        xl = (status <= 0) ? (xl+1) : xl;
        xr = (status <= 0) ? xr : (xr+1);
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

void SORT_FUNCTION_NAME(int n, TYPE *d, TYPE *z)
{
  inner_mergesort_right(0, n, d, z);
}

