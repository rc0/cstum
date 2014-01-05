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


static void init_scoring(struct scoring *s)
{
  s->n_active_subtiles = 0;
  s->n_highest_act_subs = 0;
}

static void merge_highest_gen(struct scoring *from, struct scoring *to)
{
  if (from->n_highest_act_subs > to->n_highest_act_subs) {
    to->n_highest_act_subs = from->n_highest_act_subs;
  }
  if (from->n_active_subtiles > to->n_highest_act_subs) {
    to->n_highest_act_subs = from->n_active_subtiles;
  }
}

static void merge_highest(struct node *from, struct node *to)
{
  merge_highest_gen(&from->s2, &to->s2);
  merge_highest_gen(&from->s3, &to->s3);
}

static void inner_correlate(struct node **parents, int level, struct node *top, struct node *cursor, int I, int J)
{
  int i, j;
  parents[level] = cursor;
  init_scoring(&cursor->s2);
  init_scoring(&cursor->s3);

  if (level >= 4) {
    if (cursor->d) {
      if (cursor->d->cids2) {
        ++parents[level-4]->s2.n_active_subtiles;
      }
      if (cursor->d->cids3) {
        ++parents[level-4]->s3.n_active_subtiles;
      }
    }
  }

  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      struct node *child = cursor->c[i][j];
      if (child) {
        inner_correlate(parents, level+1, top, child, (I<<1)+i, (J<<1)+j);
        merge_highest(child, cursor);
      }
    }
  }
  merge_highest(cursor, cursor);
}

void correlate(struct node *top)
{
  struct node *parents[20];
  inner_correlate(parents, 0, top, top, 0, 0);
}

