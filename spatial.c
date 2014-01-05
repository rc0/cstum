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

/* Implement spatial merging */

#include <stddef.h>
#include <stdlib.h>
#include "tool.h"

static void aggregate_readings(struct data *super, struct data *sub, int gen)/*{{{*/
{
  struct percid *p;
  struct percid **head = pergen(sub, gen);
  for (p=*head; p; p=p->next) {
    struct percid *sp;
    struct readings *r, *sr;
    sp = lookup_cell(super, gen, p->cid, 1);
    sr = lookup_readings(sp, 0); /* aggregate into just one timeframe */
    for (r=p->r; r; r=r->next) {
      int i;
      for (i=0; i<32; i++) {
        sr->asus[i] += r->asus[i];
      }
    }
  }
}
/*}}}*/
static void inner_spatial_merge(int level, struct node *cursor) {/*{{{*/
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_spatial_merge(level+1, cursor->c[i][j]);
      }
    }
  }
  if (level == 20) {
    /* Just compress all reading timeframes into the '0' timeframe */
    aggregate_readings(cursor->d, cursor->d, 2);
    aggregate_readings(cursor->d, cursor->d, 3);
  } else if (level >= 10) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          if (!cursor->d) {
            cursor->d = malloc(sizeof(struct data));
            cursor->d->cids2 = NULL;
            cursor->d->cids3 = NULL;
          }
          aggregate_readings(cursor->d, cursor->c[i][j]->d, 2);
          aggregate_readings(cursor->d, cursor->c[i][j]->d, 3);
        }
      }
    }
  }
}
/*}}}*/
void spatial_merge(struct node *cursor) /*{{{*/
{
  inner_spatial_merge(0, cursor);
}
/*}}}*/
