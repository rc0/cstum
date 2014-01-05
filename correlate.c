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
#include "tool.h"

static int offsets[] =
{
  0,  2,  1,  0,  1,  2,  0,  0,  2,  0,  0, -1,  2, -1,  0,  0,  0, -2, -1,  0, -1, -2,  0,  0, 
  -2,  0,  0,  1, -2,  1,  0,  0,  0,  3,  1,  0,  1,  3,  0,  0,  3,  0,  0, -1,  3, -1,  0,  0, 
  0, -3, -1,  0, -1, -3,  0,  0, -3,  0,  0,  1, -3,  1,  0,  0,  0,  3,  1,  0,  1,  2,  0,  0, 
  3,  0,  0, -1,  2, -1,  0,  0,  0, -3, -1,  0, -1, -2,  0,  0, -3,  0,  0,  1, -2,  1,  0,  0, 
  0,  2,  1,  0,  1,  3,  0,  0,  2,  0,  0, -1,  3, -1,  0,  0,  0, -2, -1,  0, -1, -3,  0,  0, 
  -2,  0,  0,  1, -3,  1,  0,  0,  1, -1,  1,  2,  2,  1,  0,  0, -1, -1,  2, -1,  1, -2,  0,  0, 
  -1,  1, -1, -2, -2, -1,  0,  0,  1,  1, -2,  1, -1,  2,  0,  0,  1, -1,  2,  2,  3,  1,  0,  0, 
  -1, -1,  2, -2,  1, -3,  0,  0, -1,  1, -2, -2, -3, -1,  0,  0,  1,  1, -2,  2, -1,  3,  0,  0, 
  1, -1,  2,  1,  3,  0,  0,  0, -1, -1,  1, -2,  0, -3,  0,  0, -1,  1, -2, -1, -3,  0,  0,  0, 
  1,  1, -1,  2,  0,  3,  0,  0
};

#define LIMIT (sizeof(offsets) / sizeof(offsets[0]))

static int do_gen(struct node *top, int level, int I, int J, int gen)
{
  int dx, dy;
  int ix;
  int matched;
  int score = 0;
  matched = 1;
  for (ix=0; ix<LIMIT; ) {
    dx = offsets[ix++];
    dy = offsets[ix++];
    if ((dx == 0) && (dy == 0)) {
      if (matched) {
        score++;
      }
      matched = 1; /* yes until found otherwise */
    } else {
      int II = I + dx;
      int JJ = J + dy;
      struct node *target;
      target = lookup_node(top, level, II, JJ);
      if (target && target->d &&
          ((gen == 2) ? target->d->cids2 : target->d->cids3)) {
        /* no action */
      } else {
        matched = 0;
      }
    }
  }
  return score;
}

static void inner_correlate(struct node **parents, int level, struct node *top, struct node *cursor, int I, int J)
{
  int i, j;

  parents[level] = cursor;
  cursor->tile_score2 = 0;
  cursor->tile_score3 = 0;
  cursor->max_score2 = 0;
  cursor->max_score3 = 0;

  if (cursor->d && cursor->d->cids2) {
    cursor->my_score2 = do_gen(top, level, I, J, 2);
  } else {
    cursor->my_score2 = 0;
  }
  if (cursor->d && cursor->d->cids3) {
    cursor->my_score3 = do_gen(top, level, I, J, 3);
  } else {
    cursor->my_score3 = 0;
  }

  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      struct node *child = cursor->c[i][j];
      if (child) {
        inner_correlate(parents, level+1, top, child, (I<<1)+i, (J<<1)+j);
        if (child->tile_score2 > cursor->max_score2) {
          cursor->max_score2 = child->tile_score2;
        }
        if (child->tile_score3 > cursor->max_score3) {
          cursor->max_score3 = child->tile_score3;
        }
      }
    }
  }
  if (cursor->tile_score2 > cursor->max_score2) {
    cursor->max_score2 = cursor->tile_score2;
  }
  if (cursor->tile_score3 > cursor->max_score3) {
    cursor->max_score3 = cursor->tile_score3;
  }

  if (level >= 4) {
    parents[level - 4]->tile_score2 += cursor->my_score2;
    parents[level - 4]->tile_score3 += cursor->my_score3;
  }

}

void correlate(struct node *top)
{
  struct node *parents[20];
  inner_correlate(parents, 0, top, top, 0, 0);

}

