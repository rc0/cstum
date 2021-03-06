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

#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

struct pair
{
  uint32_t foo;
  uint32_t bar;
};

#define TYPE struct pair
#define SORT_FUNCTION_NAME pair_sort
#define SORT_COMPARE_NAME pair_compare
static inline int pair_compare(const TYPE *a, const TYPE *b)
{
  if (a->foo < b->foo) return -1;
  else if (a->foo > b->foo) return +1;
  else if (a->bar < b->bar) return -1;
  else if (a->bar > b->bar) return +1;
  else return 0;
}

#include "mergesort_template.c"

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


#define MAX (1<<22)
int main (int argc, char **argv)
{
  int i, n;
  struct pair *data, *scratch;
  uint32_t sum, sum2;
  int iter;
  iter = 0;
  data = malloc(MAX * sizeof(struct pair));
  scratch = malloc(MAX * sizeof(struct pair));
  do {
    iter++;
    n = genrand(0);
    if ((genrand(0) & 3) == 0) {
      n = 1 + (n & ((1<<6) - 1));
    } else {
      n = 1 + (n & ((1<<21) - 1));
    }
    sum = 0;
    for (i=0; i<n; i++) {
      data[i].foo = genrand(0);
      data[i].bar = genrand(0);
      sum += data[i].foo;
    }
    printf("iter=%d n=%d\n", iter, n);
    fflush(stdout);

    pair_sort(n, data, scratch);

    for (i=1; i<n; i++) {
      if (data[i-1].foo > data[i].foo) {
        fprintf(stderr, "Failed ordering iter=%d n=%d i=%d\n", iter, n, i);
        exit(1);
      }
    }
    sum2 = 0;
    for (i=0; i<n; i++) {
      sum2 += data[i].foo;
    }
    if (sum2 != sum) {
      fprintf(stderr, "Failed checksum iter=%d n=%d\n", iter, n);
    }
  } while (1);

  return 0;
}
