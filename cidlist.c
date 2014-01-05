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

/* List all known CIDs */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include <stdint.h>
#include "tool.h"

struct cidrec
{
  struct cidrec *next;
  int lac;
  int cid;
  int gen;

  /* Per-CID data */
  int n20; /* number of level 20 tiles where CID appears */
  uint64_t tw;  /* Total weight */
  /* Total weighted X and Y coords */
  uint64_t tx;
  uint64_t ty;
  int max_asu;

};

#define CHAIN_SHIFT 16
#define N_CHAINS (1<<CHAIN_SHIFT)
#define CHAIN_MASK ((N_CHAINS) - 1)

static struct cidrec *chains[N_CHAINS];

static int hash(int lac, int cid)/*{{{*/
{
  int y;
  y = (cid & CHAIN_MASK) ^ ((cid >> CHAIN_SHIFT) & CHAIN_MASK);
  y ^= (lac & CHAIN_MASK) ^ ((lac >> CHAIN_SHIFT) & CHAIN_MASK);
  return y;
}
/*}}}*/

static struct cidrec *lookup_cid(int cid, int lac, int gen)/*{{{*/
{
  int h = hash(lac, cid);
  struct cidrec *r;
  for (r = chains[h]; r; r = r->next) {
    if ((r->lac == lac) && (r->cid == cid) && (r->gen == gen)) {
      return r;
    }
  }
  r = malloc(sizeof(struct cidrec));
  r->next = chains[h];
  r->lac = lac;
  r->cid = cid;
  r->gen = gen;
  r->n20 = 0;
  r->tw = 0;
  r->tx = 0;
  r->ty = 0;
  r->max_asu = -1;
  chains[h] = r;
  return r;
}
/*}}}*/
static void do_network(int zoom, int I, int J, int gen, struct percid *p)/*{{{*/
{
  while (p) {
    struct cidrec *cr = lookup_cid(p->cid, p->lac, gen);
    struct readings *r;
    uint64_t weight;
    for (r=p->r; r; r=r->next) {
      int i;
      for (i=31; i>=0; i--) {
        if (r->asus[i]) {
          if (i > cr->max_asu) {
            cr->max_asu = i;
            break;
          }
        }
      }
    }

    weight = 1 + cr->max_asu;
    weight *= weight;
    cr->tw += weight;
    cr->tx += (I<<8) * weight;
    cr->ty += (J<<8) * weight;
    ++cr->n20;

    p = p->next;
  }
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
static void emit_cidrec(struct cidrec *cr, struct tower_table *tt)/*{{{*/
{
  struct tower2 *tow;
  struct M28 m28;
  char *osgrid;
  char *hexcode;
  int cidlo, cidhi;
  m28.X = cr->tx / cr->tw;
  m28.Y = cr->ty / cr->tw;
  osgrid = m28_to_osgb_nospace(&m28);
  hexcode = compute_hexcode(m28.X, m28.Y);
  tow = cid_to_tower(tt, cr->lac, cr->cid);
  cidlo = cr->cid % 10000;
  cidhi = cr->cid / 10000;
  printf("%1c %1d  %6d %10d  %2dasu  %5d  %12s  %16s  %4d  %5d\n",
      tow ? '.' : 'X',
      cr->gen,
      cr->lac,
      cr->cid,
      cr->max_asu,
      cr->n20,
      osgrid,
      hexcode,
      cidlo, cidhi);
  free(osgrid);
  free(hexcode);
}
/*}}}*/
static void emit_table(struct tower_table *tt)/*{{{*/
{
  int i;
  for (i=0; i<N_CHAINS; i++) {
    struct cidrec *cr;
    for (cr=chains[i]; cr; cr=cr->next) {
      emit_cidrec(cr, tt);
    }
  }
}
/*}}}*/
void cidlist(struct node *cursor) /*{{{*/
{
  struct tower_table *tower_table;
  inner(0, cursor, 0, 0);
  tower_table = read_towers2("towers.dat");
  emit_table(tower_table);
}
/*}}}*/


/* vim:et:sts=2:sw=2:ht=2
 * */
