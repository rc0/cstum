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
#include "tool.h"

#define MAX_LCS 10

/* Information about what to show for each 'network' (2g or 3g) in each 1/256
 * of the tile */
struct network/*{{{*/
{
  int visited;
  int newest_timeframe; /* todo : could combine with visited! */
  int asu;
  int n_lcs;
  struct laccid lcs[MAX_LCS];
  unsigned short rel[MAX_LCS];
};
/*}}}*/
/* Information about each 1/256 of the tile */
struct micro/*{{{*/
{
  struct network g2;
  struct network g3;
};
/*}}}*/
struct range {/*{{{*/
  int x0;
  int x1;
  int y0;
  int y1;
};
/*}}}*/
static void do_net3(int I, int J, int gen, struct micro sub[16][16], struct tower_table *table, FILE *out)/*{{{*/
{
  int i;
  int j;
  for (i=0; i<16; i++) {
    for (j=0; j<16; j++) {
      struct micro *m = &sub[i][j];
      struct network *n = (gen == 2) ? &(m->g2) : &(m->g3);
      if (n->visited) {
        int kk;
        fprintf(out, "%1d %2d %2d %5d %2d 0", gen, i, j, n->newest_timeframe, n->asu);
        for (kk=0; kk<n->n_lcs; kk++) {
          int highlight = 0;
          if (table) {
            struct tower2 *tow = cid_to_tower(table, n->lcs[kk].lac, n->lcs[kk].cid);
            if (tow && tow->active_flag) {
              highlight = 1;
            }
          }
          fprintf(out, " %d/%d:%d%s",
              n->lcs[kk].lac, n->lcs[kk].cid, n->rel[kk]/656,
              highlight ? "#" : "");
        }
        fprintf(out, "\n");
      }
    }
  }
}
/*}}}*/
static void render(int I, int J, struct node *cursor, struct micro sub[16][16], struct tower_table *table, FILE *out)/*{{{*/
{
  fprintf(out, "<< %5d %5d\n", I, J);
  fprintf(out, "*2 %d %d\n", cursor->s2.n_active_subtiles, cursor->s2.n_highest_act_subs);
  fprintf(out, "*3 %d %d\n", cursor->s3.n_active_subtiles, cursor->s3.n_highest_act_subs);
  do_net3(I, J, 2, sub, table, out);
  do_net3(I, J, 3, sub, table, out);
  fprintf(out, ">>\n");
}
/*}}}*/
static void do_network(struct data *d, int gen, struct network *net)/*{{{*/
{
  struct odata *od;
  od = compute_odata(d, gen);
  if (od) {
    int n_lcs, i;
    net->visited = 1;
    net->newest_timeframe = od->newest_timeframe;
    net->asu = od->asu;
    n_lcs = od->n_lcs;
    if (n_lcs > MAX_LCS) {
      n_lcs = MAX_LCS;
    }
    net->n_lcs = n_lcs;
    for (i=0; i<n_lcs; i++) {
      net->lcs[i] = od->lcs[i];
      net->rel[i] = od->rel[i];
    }
    free_odata(od);
  } else {
    net->visited = 0;
  }
}
/*}}}*/
static void do_fragment(struct data *d, struct micro *frag)/*{{{*/
{
  do_network(d, 2, &frag->g2);
  do_network(d, 3, &frag->g3);
}
/*}}}*/
static void gather(int to_go, int I, int J, struct node *cursor, struct micro sub[16][16])/*{{{*/
{
  int i;
  int j;
  if (to_go > 0) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          gather(to_go - 1, (I<<1)+i, (J<<1)+j, cursor->c[i][j], sub);
        }
      }
    }
  } else {
    /* Process this 1/16 x 1/16 block */
    do_fragment(cursor->d, &sub[I][J]);
  }
}
/*}}}*/
static void clear_sub(struct micro sub[16][16])/*{{{*/
{
  int i, j;
  for (i=0; i<16; i++) {
    for (j=0; j<16; j++) {
      sub[i][j].g2.visited = 0;
      sub[i][j].g3.visited = 0;
    }
  }
}
/*}}}*/
static void do_tile(int I, int J, struct node *cursor, struct tower_table *table, FILE *out)/*{{{*/
{
  struct micro sub[16][16];
  /* Clear the array */
  clear_sub(sub);

  gather(4, 0, 0, cursor, sub);
  render(I, J, cursor, sub, table, out);
}
/*}}}*/
static void inner(int level_to_do, int level, struct node *cursor,
    int I, int J,
    struct range *rangez, struct tower_table *table,
    FILE *out) {/*{{{*/
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner(level_to_do, level+1, cursor->c[i][j], (I<<1)+i, (J<<1)+j, rangez, table, out);
      }
    }
  }
  if (level == level_to_do) {
    if (!rangez ||
        ((I >= rangez->x0) &&
         (I <= rangez->x1) &&
         (J >= rangez->y0) &&
         (J <= rangez->y1))) {
      do_tile(I, J, cursor, table, out);
    }
  }
}
/*}}}*/
static void inner_get_range(struct node *cursor,/*{{{*/
    struct tower_table *table,
    int level, int I, int J,
    struct range *r)
{
  int i, j;
  if (level < 20) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_get_range(cursor->c[i][j], table, level+1, (I<<1)+i, (J<<1)+j, r);
        }
      }
    }
  } else {
    struct percid *p;
    int matched = 0;
    for (p = cursor->d->cids2; p; p = p->next) {
      struct tower2 *tow = cid_to_tower(table, p->lac, p->cid);
      if (tow && tow->active_flag) {
        matched = 1;
        goto done;
      }
    }
    for (p = cursor->d->cids3; p; p = p->next) {
      struct tower2 *tow = cid_to_tower(table, p->lac, p->cid);
      if (tow && tow->active_flag) {
        matched = 1;
        goto done;
      }
    }
done:
    if (matched) {
      if ((r->x0 < 0) || (r->x0 > I)) r->x0 = I;
      if ((r->x1 < 0) || (r->x1 < I)) r->x1 = I;
      if ((r->y0 < 0) || (r->y0 > J)) r->y0 = J;
      if ((r->y1 < 0) || (r->y1 < J)) r->y1 = J;
    }
  }
}
/*}}}*/
static void get_range(struct node *top, struct tower_table *table, struct range *r)/*{{{*/
{
  r->x0 = r->x1 = r->y0 = r->y1 = -1;
  inner_get_range(top, table, 0, 0, 0, r);
}
/*}}}*/
void tile_walk(int zoom, struct tower_table *table, struct node *cursor, FILE *out) /*{{{*/
{
  struct range *rangez;
  if (zoom < 6) {
    fprintf(stderr, "Can't do tiling for zoom %d\n", zoom);
    exit(2);
  }

  if (table) {
    struct range range20;
    int shift_amount = 20 - zoom;
    get_range(cursor, table, &range20);
    rangez = malloc(sizeof(struct range));
    rangez->x0 = range20.x0 >> shift_amount;
    rangez->x1 = range20.x1 >> shift_amount;
    rangez->y0 = range20.y0 >> shift_amount;
    rangez->y1 = range20.y1 >> shift_amount;
    fprintf(stderr, "Tile range X:[%d,%d] Y:[%d,%d]\n",
        rangez->x0, rangez->x1, rangez->y0, rangez->y1);
  } else {
    rangez = NULL;
  }

  inner(zoom, 0, cursor, 0, 0, rangez, table, out);
  if (rangez) free(rangez);
}

/* vim:et:sts=2:sw=2:ht=2
 * */
