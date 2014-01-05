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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "tool.h"

static void do_network(int zoom, int I, int J, int gen, struct percid *p)/*{{{*/
{
}
/*}}}*/
static void do_tile(int zoom, int I, int J, struct node *cursor)/*{{{*/
{
  if (cursor->d) {
    do_network(zoom, I, J, 2, cursor->d->cids2);
    do_network(zoom, I, J, 3, cursor->d->cids3);
  }
}
/*}}}*/
static void inner(int level, struct node *cursor, int I, int J) {/*{{{*/
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner(level+1, cursor->c[i][j], (I<<1)+i, (J<<1)+j);
      }
    }
  }
  if ((level >= 20) && (level <= 20)) {
    do_tile(level, I, J, cursor);
  }
}
/*}}}*/
void timecull(struct node *cursor) /*{{{*/
{
  inner(0, cursor, 0, 0);
}
/*}}}*/


