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

/* 3rd generation tower colour selection.
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"
#include "chart.h"
#include "tpgen.h"
#define THE_LEVEL 18
struct ranges/*{{{*/
{
  int xmin,xmax,ymin,ymax;
};
/*}}}*/
struct charts/*{{{*/
{
  struct chart *central;
};
/*}}}*/
struct tower_data/*{{{*/
{
  struct ranges r2;
  struct ranges r3;
  struct charts c2;
  struct charts c3;
};
/*}}}*/
static void gather(int gen,/*{{{*/
    struct data *d,
    struct tower_table *table,
    int I, int J,
    struct tower_data *data)
{
  struct percid *p;
  p = (gen == 3) ? d->cids3 : d->cids2;
  while (p) {
    struct tower2 *tow;
    tow = cid_to_tower(table, p->lac, p->cid);
    if (tow) {
      int ti = tow->index;
      struct tower_data *dd = data + ti;
      struct ranges *r = (gen == 3) ? &dd->r3 : &dd->r2;
      if ((r->xmin < 0) || (I < r->xmin)) r->xmin = I;
      if ((r->xmax < 0) || (I > r->xmax)) r->xmax = I;
      if ((r->ymin < 0) || (J < r->ymin)) r->ymin = J;
      if ((r->ymax < 0) || (J > r->ymax)) r->ymax = J;
    }
    p = p->next;
  }
}
/*}}}*/
static void inner_get_ranges(struct node *cursor,/*{{{*/
    struct tower_table *table,
    int level, int I, int J,
    struct tower_data *data)
{
  if (level == THE_LEVEL) {
    if (cursor->d) {
      gather(2, cursor->d, table, I, J, data);
      gather(3, cursor->d, table, I, J, data);
    }

  } else {
    int i, j;
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_get_ranges(cursor->c[i][j], table, level + 1, (I<<1)+i, (J<<1)+j, data);
        }
      }
    }
  }

}
/*}}}*/
static void init_ranges(struct ranges *r)/*{{{*/
{
  r->xmin = -1;
  r->ymin = -1;
  r->xmax = -1;
  r->ymax = -1;
}
/*}}}*/
static void get_ranges(struct node *top,/*{{{*/
    struct tower_table *table,
    struct tower_data *data)
{
  int i;
  for (i=0; i<table->n_towers; i++) {
    struct tower_data *d = data + i;
    init_ranges(&d->r2);
    init_ranges(&d->r3);
  }
  inner_get_ranges(top, table, 0, 0, 0, data);
}
/*}}}*/
static void chart_gather(int gen,/*{{{*/
    struct data *d,
    struct tower_table *table,
    int I, int J,
    struct tower_data *data)
{
  struct odata *od;
  od = compute_odata(d, gen);
  if (od) {
    int i, n;
    int max;
    int thresh;
    n = od->n_lcs;
    max = od->rel[0];
    thresh = max >> 2;
    for (i=0; i<n; i++) {
      struct tower2 *tow;
      int ti;
      struct tower_data *dd;
      struct chart *c;
      if (od->rel[i] < thresh) break;
      tow = cid_to_tower(table, od->lcs[i].lac, od->lcs[i].cid);
      if (tow) {
        ti = tow->index;
        dd = data + ti;
        c = (gen == 3) ? dd->c3.central : dd->c2.central;
        set_point(c, I, J);
      }
    }
    free_odata(od);
  }
}
/*}}}*/
static void inner_fill_charts(struct node *cursor,/*{{{*/
    struct tower_table *table,
    int level, int I, int J,
    struct tower_data *data)
{
  if (level == THE_LEVEL) {
    if (cursor->d) {
      chart_gather(2, cursor->d, table, I, J, data);
      chart_gather(3, cursor->d, table, I, J, data);
    }

  } else {
    int i, j;
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_fill_charts(cursor->c[i][j], table, level + 1, (I<<1)+i, (J<<1)+j, data);
        }
      }
    }
  }

}
/*}}}*/
static void fill_charts(struct node *top,/*{{{*/
    struct tower_table *table,
    struct tower_data *data)
{
  inner_fill_charts(top, table, 0, 0, 0, data);
}
/*}}}*/
static void make_charts(struct node *top,/*{{{*/
    struct tower_table *table,
    struct tower_data *data)
{
  int i;
  get_ranges(top, table, data);
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    printf("X:%5d-%5d Y:%5d-%5d (%s,%s) %s\n",
        data[i].r2.xmin,
        data[i].r2.xmax,
        data[i].r2.ymin,
        data[i].r2.ymax,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        tow->description);
  }

  for (i=0; i<table->n_towers; i++) {
    data[i].c2.central = new_chart(data[i].r2.xmin,
        data[i].r2.xmax,
        data[i].r2.ymin,
        data[i].r2.ymax);
    data[i].c3.central = new_chart(data[i].r3.xmin,
        data[i].r3.xmax,
        data[i].r3.ymin,
        data[i].r3.ymax);
  }
  fill_charts(top, table, data);
#if 0
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    printf("Tower %3d (%s,%s) %s\n",
        i,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        tow->description);
    if (data[i].c2) {
      printf("  2D:\n");
      display_chart(data[i].c2);
    }
    if (data[i].c3) {
      printf("  3D:\n");
      display_chart(data[i].c3);
    }
  }
#endif

}
/*}}}*/

struct foo
{
  int i;
  int d2;
  int d3;
  int d;
};

static int compare_foo(const void *A, const void *B)
{
  const struct foo *a = A;
  const struct foo *b = B;

  if (a->d < b->d) return -1;
  else if (a->d > b->d) return +1;
  else if (a->i < b->i) return -1;
  else if (a->i > b->i) return +1;
  else return 0;
}

static void compute_distances(struct tower_table *table, struct tower_data *data)/*{{{*/
{
  int i, j;
  struct foo *board;
  board = malloc(table->n_towers * sizeof(struct foo));
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *towi = table->towers[i];
    int X, Y;
    int n;
    printf("Tower %3d : (%s,%s) %s\n",
        i,
        towi->label_2g ?: "-",
        towi->label_3g ?: "-",
        towi->description);
    if (towi->has_observed) {
      X = towi->observed.X;
      Y = towi->observed.Y;
    } else if (towi->has_estimated) {
      X = towi->estimated.X;
      Y = towi->estimated.Y;
    } else {
      get_rough_tp_estimate(towi, &X, &Y);
    }
    X >>= (28 - THE_LEVEL);
    Y >>= (28 - THE_LEVEL);
    n = 0;
    for (j=0; j<table->n_towers; j++) {
      int d2, d3, d;
      if (j==i) continue;
      d2 = d3 = -1;
      if (data[i].c2.central && data[j].c2.central) {
        d2 = find_closest_point(data[j].c2.central, X, Y);
      }
      if (data[i].c3.central && data[j].c3.central) {
        d3 = find_closest_point(data[j].c3.central, X, Y);
      }
      if ((d2 >= 0) && (d3 >= 0)) {
        d = (d2 < d3) ? d2 : d3;
      } else if (d2 >= 0) {
        d = d2;
      } else if (d3 >= 0) {
        d = d3;
      } else {
        d = -1;
      }
      if (d >= 0) {
        board[n].d2 = d2;
        board[n].d3 = d3;
        board[n].d = d;
        board[n].i = j;
        n++;
      }
    }
    qsort(board, n, sizeof(struct foo), compare_foo);
    for (j=0; (j<20) && (j<n); j++) {
      struct tower2 *towj = table->towers[board[j].i];
      printf("    %8d  %8d  %8d   %3d : (%s,%s) %s\n",
          board[j].d,
          board[j].d2, board[j].d3,
          board[j].i,
          towj->label_2g ?: "-",
          towj->label_3g ?: "-",
          towj->description);
    }
  }
}
/*}}}*/
void tcs(struct node *top, struct tower_table *table, int all)/*{{{*/
{
  struct tower_data *data;
  data = malloc(table->n_towers * sizeof(struct tower_data));
  make_charts(top, table, data);
  compute_distances(table, data);

}
/*}}}*/
