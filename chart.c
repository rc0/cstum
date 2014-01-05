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
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include "chart.h"

/* A 'chart' is a binary bitmap showing the areas where a signal has been seen from a particular tower
 */

struct chart/*{{{*/
{
  int x0, x1; /* In "user" units */
  int y0, y1; /* In "user" units */
  int nx, ny; /* In "user" units */

  int xx0, xx1; /* In >>5 units */
  int nnx; /* In >>5 units */

  uint32_t **map;
};
/*}}}*/
struct chart *new_chart(int xmin, int xmax, int ymin, int ymax)/*{{{*/
{
  struct chart *result;
  int i;

  if (xmin < 0) return NULL;

  result = malloc(sizeof(struct chart));
  memset(result, 0, sizeof(struct chart));
  result->x0 = xmin;
  result->y0 = ymin;
  result->x1 = xmax;
  result->y1 = ymax;
  result->ny = 1 + ymax - ymin;
  result->nx = 1 + xmax - xmin;
  result->xx0 = xmin >> 5;
  result->xx1 = xmax >> 5;
  result->nnx = 1 + (xmax >> 5) - (xmin >> 5);

  result->map = malloc(result->ny * sizeof(uint32_t *));
  for (i=0; i<result->ny; i++) {
    result->map[i] = calloc(result->nnx, sizeof(uint32_t));
  }
  return result;
}
/*}}}*/
struct chart *smudge_chart(const struct chart *in, int by)/*{{{*/
{
  int xmin, xmax, ymin, ymax;
  struct chart *result;
  int i, j;
  xmin = in->x0 - by;
  ymin = in->y0 - by;
  xmax = in->x1 + by;
  ymax = in->y1 + by;
  result = new_chart(xmin, xmax, ymin, ymax);
  /* Clone the existing map */
  for (i=in->y0; i<=in->y1; i++) {
    int i0 = i - in->y0;
    int ii = i - result->y0;
    for (j=in->xx0; j<=in->xx1; j++) {
      int j0 = j - in->xx0;
      int jj = j - result->xx0;
      result->map[ii][jj] = in->map[i0][j0];
    }
  }

  for (i=in->y0; i<=in->y1; i++) {
    int ii = i - in->y0;
    int ir = i - result->y0;
    for (j=in->xx0; j<=in->xx1; j++) {
      int jj = j - in->xx0;
      int jr = j - result->xx0;
      int dy;
      uint32_t val;
      uint32_t ll, lc, rc, rr;
      val = in->map[ii][jj];
      ll = val >> (32 - by);
      lc = val << by;
      rc = val >> by;
      rr = val << (32 - by);
      for (dy=-by; dy<=by; dy+=by) {
        result->map[ir+dy][jr] |= (lc | rc);

        if (jr > 0) {
          result->map[ir+dy][jr-1] |= ll;
        }
        if (jr+1 < result->nnx) {
          result->map[ir+dy][jr+1] |= rr;
        }
      }
    }
  }

  return result;
}
/*}}}*/
struct chart *difference_chart(const struct chart *in, const struct chart *mask)/*{{{*/
{
  struct chart *result;
  int i, j;
  int xmin, xmax, ymin, ymax;

  if (!in) return NULL;

  result = new_chart(in->x0, in->x1, in->y0, in->y1);
  /* Clone the existing map */
  for (i=in->y0; i<=in->y1; i++) {
    int i0 = i - in->y0;
    int ii = i - result->y0;
    for (j=in->xx0; j<=in->xx1; j++) {
      int j0 = j - in->xx0;
      int jj = j - result->xx0;
      result->map[ii][jj] = in->map[i0][j0];
    }
  }
  if (!mask) return result;

  xmin = (in->xx0 < mask->xx0) ? mask->xx0 : in->xx0; /* the larger minimum */
  xmax = (in->xx1 < mask->xx1) ? in->xx1 : mask->xx1; /* the smaller maximum */
  if (xmax < xmin) return result;
  ymin = (in->y0 < mask->y0) ? mask->y0 : in->y0; /* the larger minimum */
  ymax = (in->y1 < mask->y1) ? in->y1 : mask->y1; /* the smaller maximum */
  if (ymax < ymin) return result;

  for (i=ymin; i<=ymax; i++) {
    int im = i - mask->y0;
    int ir = i - result->y0;
    for (j=xmin; j<=xmax; j++) {
      int jm = j - mask->xx0;
      int jr = j - mask->xx0;
      result->map[ir][jr] &= ~mask->map[im][jm];
    }
  }
  return result;
}
/*}}}*/
void set_point(struct chart *c, int x, int y)/*{{{*/
{
  int block;
  int offset;
  int row;
  uint32_t mask;
  if ((x < c->x0) ||
      (y < c->y0) ||
      (x > c->x1) ||
      (y > c->y1)) {
    fprintf(stderr, "set_point : (%d,%d) out of range\n", x, y);
    exit(2);
  }
  block = (x >> 5) - c->xx0;
  /* Want bit[31] to be the lower X coord so that the concepts of left and
   * right shift to move the bitmaps left and right remain intuitive. */
  offset = 31 - (x & 0x1f);
  mask = 1U << offset;
  row = y - c->y0;
  c->map[row][block] |= mask;
  return;
}
/*}}}*/
static int cbs(uint32_t x)/*{{{*/
{
  uint32_t y;
  y = ((x >>  1) & 0x55555555U) + (x & 0x55555555U);
  y = ((y >>  2) & 0x33333333U) + (y & 0x33333333U);
  y = ((y >>  4) & 0x0f0f0f0fU) + (y & 0x0f0f0f0fU);
  y = ((y >>  8) & 0x00ff00ffU) + (y & 0x00ff00ffU);
  y = ((y >> 16) & 0x0000ffffU) + (y & 0x0000ffffU);
  return (int) y;
}
/*}}}*/
int score_overlap(struct chart *c1, struct chart *c2)/*{{{*/
{
  int xmin, xmax, ymin, ymax;
  int total;
  int i, j;

  if (!c1 || !c2) return 0;
  xmin = (c1->xx0 < c2->xx0) ? c2->xx0 : c1->xx0; /* the larger minimum */
  xmax = (c1->xx1 < c2->xx1) ? c1->xx1 : c2->xx1; /* the smaller maximum */
  if (xmax < xmin) return 0;
  ymin = (c1->y0 < c2->y0) ? c2->y0 : c1->y0; /* the larger minimum */
  ymax = (c1->y1 < c2->y1) ? c1->y1 : c2->y1; /* the smaller maximum */
  if (ymax < ymin) return 0;

  /* If we get here, there are some fields to check. */
  total = 0;
  for (i=ymin; i<=ymax; i++) {
    int i1 = i - c1->y0;
    int i2 = i - c2->y0;
    for (j=xmin; j<=xmax; j++) {
      int j1 = j - c1->xx0;
      int j2 = j - c2->xx0;
      total += cbs(c1->map[i1][j1] & c2->map[i2][j2]);
    }
  }

  return total;
}
/*}}}*/
void display_chart(struct chart *c)/*{{{*/
{
  int i, j;
  printf("            ");
  printf("%16d\n", c->xx0);
  for (i=c->y0; i<=c->y1; i++) {
    printf("    %5d :", i);
    for (j=c->xx0; j<=c->xx1; j++) {
      int b;
      uint32_t val = c->map[i - c->y0][j - c->xx0];
      putchar (' ');
      for (b=31; b>=0; b--) {
        putchar((val & (1<<b)) ? '#' : '.');
      }
    }
    printf("\n");
  }
}
/*}}}*/
void free_chart(struct chart *x)/*{{{*/
{
  int i;
  for (i=0; i<x->ny; i++) {
    free(x->map[i]);
  }
  free(x->map);
  free(x);
}
/*}}}*/
int find_closest_point(struct chart *c, int X, int Y)/*{{{*/
{
  int result = -1;
  int xmin, xmax, ymin, ymax;
  int i, j;
  if (c) {
    xmin = c->xx0;
    xmax = c->xx1;
    ymin = c->y0;
    ymax = c->y1;
    for (i=ymin; i<=ymax; i++) {
      for (j=xmin; j<=xmax; j++) {
        int row, block;
        uint32_t mask;
        int bit;
        row = i - ymin;
        block = j - xmin;
        for (bit=0, mask = (1UL<<31); bit<32; bit++, mask>>=1) {
          if (c->map[row][block] & mask) {
            int x, y;
            int dx, dy, d;
            y = i;
            x = bit + (j<<5);
            dx = abs(x - X);
            dy = abs(y - Y);
            d = dx + dy;
            if ((result < 0) || (d < result)) {
              result = d;
            }
          }
        }
      }
    }
  }
  return result;
}
/*}}}*/
