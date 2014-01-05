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
#include "tool.h"

static void emit_cids2(FILE *out, int gen, struct percid *p)
{
  while (p) {
    struct readings *r;
    for (r=p->r; r; r=r->next) {
      int i;
      int gap;
      fprintf(out, "%1d %5d %d %d ", gen, r->timeframe, p->lac, p->cid);
      gap = 0;
      for (i=0; i<=31; i++) {
        if (r->asus[i] > 0) {
          if (gap < 26) {
            fputc('a' + gap, out);
          } else {
            fputc('A' + (gap - 26), out);
          }
          gap = 0;
          fprintf(out, "%d", r->asus[i]);
        } else {
          gap++;
        }
      }
      fputc('\n', out);
    }
    p = p->next;
  }
}

static void inner_write(FILE *out, struct node *cursor, int level, int I, int J)/*{{{*/
{
  int i, j;
  if (level < 20) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_write(out, cursor->c[i][j], level+1, (I<<1)+i, (J<<1)+j);
        }
      }
    }
  } else {
    fprintf(out, ": %6d %6d\n", I, J);
    emit_cids2(out, 2, cursor->d->cids2);
    emit_cids2(out, 3, cursor->d->cids3);
  }
}
/*}}}*/
void write_db(FILE *out, struct node *top)/*{{{*/
{
  if (top) {
    inner_write(out, top, 0, 0, 0);
  }
}
/*}}}*/
