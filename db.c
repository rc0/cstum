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


#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include "tool.h"

void insert_asus(struct data *cursor, int gen, int lac, int cid, int timeframe, int *asus) /*{{{*/
{
  int i;
  struct percid *c;
  struct readings *r;
  c = lookup_cell(cursor, gen, lac, cid, 1);
  r = lookup_readings(c, timeframe);
  for (i=0; i<32; i++) {
    r->asus[i] += asus[i];
  }
}
/*}}}*/
static struct percid *new_percid(int lac, int cid, struct percid *next)/*{{{*/
{
  struct percid *res;
  res = malloc(sizeof(struct percid));
  res->next = next;
  res->lac = lac;
  res->cid = cid;
  res->r = NULL;
  return res;
}
/*}}}*/
struct percid *lookup_cell(struct data *d, int gen, int lac, int cid, int create)/*{{{*/
{
  struct percid *res;
  struct percid **head;
  head = pergen(d, gen);
  res = *head;
  while (res) {
    if ((res->lac == lac) &&
        (res->cid == cid)) {
      return res;
    }
    res = res->next;
  }
  if (create) {
    res = new_percid(lac, cid, *head);
    *head = res;
  }
  return res;
}
/*}}}*/
struct data *lookup_spatial(struct node *parent, int xi, int yi) /*{{{*/
{
  struct node *here = parent;
  int sel = 1 << 19;
  while (sel) {
    int i = (xi & sel) ? 1 : 0;
    int j = (yi & sel) ? 1 : 0;
    if (here->c[i][j] == NULL) {
      here->c[i][j] = new_node();
    }
    here = here->c[i][j];
    sel >>= 1;
  }
  if (here->d == NULL) {
    here->d = malloc(sizeof(struct data));
    memset(here->d, 0, sizeof(struct data));
  }
  return here->d;
}
/*}}}*/
struct node *new_node(void) /*{{{*/
{
  struct node *res;
  res = malloc(sizeof(struct node));
  memset(res, 0, sizeof(struct node));
  return res;
}
/*}}}*/
static struct readings *new_readings(int timeframe, struct readings *next)/*{{{*/
{
  struct readings *res;
  res = malloc(sizeof(struct readings));
  res->timeframe = timeframe;
  res->newest = timeframe;
  res->next = next;
  memset(res->asus, 0, sizeof(res->asus));
  return res;
}
/*}}}*/
struct readings *lookup_readings(struct percid *c, int timeframe)/*{{{*/
{
  struct readings *res;
  res = c->r;
  while (res) {
    if (res->timeframe == timeframe) {
      return res;
    }
    res = res->next;
  }
  res = new_readings(timeframe, c->r);
  c->r = res;
  return res;
}
/*}}}*/
struct node *lookup_node(struct node *top, int level, int ii, int jj)/*{{{*/
{
  struct node *cursor = top;
  while (cursor && (level > 0)) {
    level--;
    int i0 = (ii >> level) & 1;
    int j0 = (jj >> level) & 1;
    cursor = cursor->c[i0][j0];
  }
  return cursor;
}
/*}}}*/

