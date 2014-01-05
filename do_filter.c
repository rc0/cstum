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
#include <ctype.h>
#include "tool.h"

struct chain {/*{{{*/
  struct chain *next;
  int cid;
};
/*}}}*/
static int hash(int x)/*{{{*/
{
  return (x & 0xffff) ^ ((x >> 16) & 0xffff);
}
/*}}}*/


static void insert_cid(struct chain **ch, int cid)/*{{{*/
{
  int c;
  struct chain *nch;
  c = hash(cid);
  nch = ch[c];
  while (nch) {
    if (nch->cid == cid) {
      fprintf(stderr, "Duplicate entry for cid %d\n", cid);
      exit(2);
    }
    nch = nch->next;
  }

  nch = malloc(sizeof(struct chain));
  nch->cid = cid;
  nch->next = ch[c];
  ch[c] = nch;
}
/*}}}*/
static void insert_cids(int nc, int *c, struct chain **ch)/*{{{*/
{
  int i;
  for (i=0; i<nc; i++) {
    insert_cid(ch, c[i]);
  }
}
/*}}}*/
static void free_chains(struct chain **chains)/*{{{*/
{
  int i;
  struct chain *c, *nc;
  for (i=0; i<1<<16; i++) {
    for (c=chains[i]; c; c=nc) {
      nc = c->next;
      free(c);
    }
  }
}
/*}}}*/
static void free_percid(struct percid *p)/*{{{*/
{
  struct readings *r, *nr;
  for (r=p->r; r; r=nr) {
    nr = r->next;
    free(r);
  }
  free(p);
}
/*}}}*/
static int check_cid(int cid, struct chain **chains)/*{{{*/
{
  int h;
  struct chain *c;
  h = hash(cid);
  c = chains[h];
  while (c) {
    if (c->cid == cid) return 1;
    else c = c->next;
  }
  return 0;
}
/*}}}*/
static void filter_percid(struct percid **pp, struct chain **chains)/*{{{*/
{
  struct percid *p, *nextp;
  struct percid *z;
  for (p=*pp, z=NULL; p; p=nextp) {
    nextp = p->next;
    if (check_cid(p->cid, chains)) {
      p->next = z;
      z = p;
    } else {
      free_percid(p);
    }
  }
  *pp = z;
}
/*}}}*/
static int filter_data(struct data *d, struct chain **chains)/*{{{*/
{
  filter_percid(&d->cids2, chains);
  filter_percid(&d->cids3, chains);
  if (d->cids2 || d->cids3) {
    return 1;
  } else {
    return 0;
  }
}
/*}}}*/
static int inner_filter(int level, struct node *cursor, struct chain **chains)/*{{{*/
{
  int i, j;
  int result;
  if (level == 20) {
    int status;
    status = filter_data(cursor->d, chains);
    if (status) {
      result = 1;
    } else {
      result = 0;
      free(cursor->d);
    }
  } else { /* (level < 20) */
    int count = 0;
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          int status;
          status = inner_filter(level+1, cursor->c[i][j], chains);
          if (!status) {
            free(cursor->c[i][j]);
            cursor->c[i][j] = NULL;
          }
          count += status;
        }
      }
    }
    result = (count > 0) ? 1 : 0;
  }
  return result;
}
/*}}}*/
void cid_filter(struct node *top, int n_cids, int *cids)/*{{{*/
{
  struct chain **chains;
  chains = calloc(1<<16, sizeof(struct chain *));
  insert_cids(n_cids, cids, chains);
  inner_filter(0, top, chains);
  free_chains(chains);
}
/*}}}*/
