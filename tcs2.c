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
#include <math.h>
#include "tool.h"
#include "chart.h"

/* What level tile to process */
#define THE_LEVEL 17

struct conflict/*{{{*/
{
  struct conflict *next;
  int other_index;
  int count;
};
/*}}}*/
struct conflict_table/*{{{*/
{
  struct conflict **c;
};
/*}}}*/

struct ranges/*{{{*/
{
  int xmin,xmax,ymin,ymax;
};
/*}}}*/
struct charts/*{{{*/
{
  struct chart *central;
  struct chart *inner;
  struct chart *outer;
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
static void make_halos(struct charts *cs)/*{{{*/
{
  if (cs->central) {
    /* To be found empirically whether this level of smudging is enough to
     * force colour separation in areas where we have little data... */
    struct chart *temp1, *temp2, *temp4, *temp8;
    struct chart *temp16;
    temp1 = smudge_chart(cs->central, 1);
    temp2 = smudge_chart(temp1, 1);
    cs->inner = difference_chart(temp2, cs->central);
    temp4 = smudge_chart(temp2, 2);
    temp8 = smudge_chart(temp4, 4);
    temp16 = smudge_chart(temp8, 8);
    cs->outer = difference_chart(temp16, temp2);

    free_chart(temp1);
    free_chart(temp2);
    free_chart(temp4);
    free_chart(temp8);
    free_chart(temp16);
  } else {
    cs->inner = NULL;
    cs->outer = NULL;
  }
}
/*}}}*/
static void make_all_halos(int n_towers,/*{{{*/
    struct tower_data *data)
{
  int i;
  for (i=0; i<n_towers; i++) {
    struct tower_data *d = data + i;
    make_halos(&d->c2);
    make_halos(&d->c3);
  }
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

  make_all_halos(table->n_towers, data);

}
/*}}}*/
/*}}}*/
static void build_conflicts(/*{{{*/
    const struct tower_table *table,
    const struct tower_data *data,
    struct conflict_table *ctable,
    int verbose)
{
  int i;
  int j;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    if (verbose) {
      printf("Tower %d (%s,%s) %s\n",
          i,
          tow->label_2g ?: "-",
          tow->label_3g ?: "-",
          tow->description);
    }

    for (j=0; j<table->n_towers; j++) {
      int score2;
      int score3;
      int score;
      int inner_score2;
      int inner_score3;
      int inner_score;
      int outer_score2;
      int outer_score3;
      int outer_score;
      int weighted_score;
      struct conflict *c;
      if (j == i) continue;
      score2 = score_overlap(data[i].c2.central, data[j].c2.central);
      score3 = score_overlap(data[i].c3.central, data[j].c3.central);
      score = score2 + score3;
      inner_score2 = score_overlap(data[i].c2.central, data[j].c2.inner);
      inner_score3 = score_overlap(data[i].c3.central, data[j].c3.inner);
      inner_score = inner_score2 + inner_score3;
      outer_score2 = score_overlap(data[i].c2.central, data[j].c2.outer);
      outer_score3 = score_overlap(data[i].c3.central, data[j].c3.outer);
      outer_score = outer_score2 + outer_score3;
      weighted_score = (score << 18) + (inner_score << 8) + (outer_score);
      if (weighted_score > 0) {
        struct tower2 *towj = table->towers[j];
        if (verbose) {
          printf("  |%4d %4d %4d|%4d %4d %4d|%4d %4d %4d|%10d| %3d (%s,%s) %s\n",
            score,
            score2, score3,
            inner_score,
            inner_score2, inner_score3,
            outer_score,
            outer_score2, outer_score3,
            weighted_score,
            j,
            towj->label_2g ?: "-",
            towj->label_3g ?: "-",
            towj->description);
        }

        for (c=ctable->c[i]; c; c = c->next) {
          if (c->other_index == j) break;
        }
        if (!c) {
          c = malloc(sizeof(struct conflict));
          c->count = 0;
          c->other_index = j;
          c->next = ctable->c[i];
          ctable->c[i] = c;
        }
        c->count += weighted_score;

      }
    }
  }
}
/*}}}*/
static struct conflict_table *make_conflict_table(struct tower_table *table)/*{{{*/
{
  struct conflict_table *result;
  result = malloc(sizeof(struct conflict_table));
  result->c = calloc(table->n_towers, sizeof(struct conflict *));
  return result;
}
/*}}}*/
static void free_tower_data(struct tower_data *d)/*{{{*/
{
  if (d->c2.central) free(d->c2.central);
  if (d->c2.inner) free(d->c2.inner);
  if (d->c2.outer) free(d->c2.outer);
  if (d->c3.central) free(d->c3.central);
  if (d->c3.inner) free(d->c3.inner);
  if (d->c3.outer) free(d->c3.outer);
  free(d);
}
/*}}}*/

#define NSAT 9
#define NSAT2 (NSAT+NSAT)
#define NCOLS (3*NSAT)
#define REF(r,c) conf[((c)*NSAT) + (((r)+NSAT)%NSAT)]
#define NOT_SAT_MULT 2

struct contrib/*{{{*/
{
  struct contrib *next;
  int index;
  int amount;
};
/*}}}*/
struct colour/*{{{*/
{
  struct contrib *towers;
  int score;
};
/*}}}*/
static void free_colour_array(struct colour *conf)/*{{{*/
{
  int i;
  for (i=0; i<NCOLS; i++) {
    struct contrib *c, *nc;
    for (c = conf[i].towers; c; c = nc) {
      nc = c->next;
      free(c);
    }
  }
  free(conf);
}
/*}}}*/
static void put_score(struct colour *conf, int oi, int r, int c, int incr)/*{{{*/
{
  int rr = (r + NSAT) % NSAT;
  int cc = c * NSAT;
  struct contrib *nc;
  if (incr > 0) {
    nc = malloc(sizeof(struct colour));
    nc->amount = incr;
    nc->index = oi;
    nc->next = conf[cc+rr].towers;
    conf[cc+rr].towers = nc;
    conf[cc+rr].score += incr;
  }
}
/*}}}*/
static void add_scores(struct colour *conf, int oi, int ci0, int count)/*{{{*/
{
  int L, H;
  int scountA = count >> 3;
  int scountB = count >> 6;
#if 0
  conf[ci0] += scount;
  return;
#endif

  L = ci0 / NSAT;
  H = ci0 % NSAT;
  if (L == 0) { /* the saturated row */
    put_score(conf, oi, H, 0, count);
    /* 1/2 conflict to neighbouring saturated colours */
    put_score(conf, oi, H+1, 0, scountA);
    put_score(conf, oi, H-1, 0, scountA);
    /* 1/4 conflict to 2-away saturated colours */
    put_score(conf, oi, H+2, 0, scountB);
    put_score(conf, oi, H-2, 0, scountB);
    /* and 1/2 conflict to same hue in the dark and light groups */
    put_score(conf, oi, H, 1, scountA);
    put_score(conf, oi, H, 2, scountA);
    /* 1/4 conflict to 1-away dark/light colours */
    put_score(conf, oi, H-1, 1, scountB);
    put_score(conf, oi, H-1, 2, scountB);
    put_score(conf, oi, H+1, 1, scountB);
    put_score(conf, oi, H+1, 2, scountB);
  } else { /* the dark/light row */
    put_score(conf, oi, H, L, count);
    /* full conflict count onto the neighbouring colours */
    put_score(conf, oi, H+1, L, count);
    put_score(conf, oi, H-1, L, count);
    /* 1/2 conflict into the saturated row */
    put_score(conf, oi, H, 0, scountA);
    /* 1/2 conflict into the colours 2 away in dark/light row */
    put_score(conf, oi, H+2, L, scountA);
    put_score(conf, oi, H-2, L, scountA);
    /* 1/4 conflict into the colours 1 away in saturated row */
    put_score(conf, oi, H+1, 0, scountB);
    put_score(conf, oi, H-1, 0, scountB);
  }
}
/*}}}*/
static void score_conflicts(int index, const struct tower_table *table, const struct conflict_table *ctable, struct colour conf[NCOLS])/*{{{*/
{
  struct conflict *c;
  /* Scale close-in conflicts by a large amount to ensure these are taken
   * account of as a priority.  But include further-away conflicts as a
   * tie-breaker */
  for (c = ctable->c[index]; c; c = c->next) {
    int oi = c->other_index;
    int ci0 = table->towers[oi]->colour_index;
    if (ci0 >= 0) {
      add_scores(conf, oi, ci0, c->count);
    }
  }
}
/*}}}*/
static void find_best_colour(const struct colour conf[NCOLS], int tried, int *best_score, int *best_colour)/*{{{*/
{
  int best_index, best_so_far;
  int ci;
  best_index = -1;
  for (ci=1; ci<NCOLS; ci++) {
    int this_conf = conf[ci].score;
    if (ci >= NSAT) {
      this_conf *= NOT_SAT_MULT;
    }
    if ((best_index < 0) || (this_conf < best_so_far)) {
      if ((tried & (1<<ci)) == 0) {
        best_so_far = this_conf;
        best_index = ci;
      }
    }
  }
  *best_score = best_so_far;
  *best_colour = best_index;
}
/*}}}*/
static int get_score(const struct colour conf[NCOLS], int index)/*{{{*/
{
  int score;
  score = conf[index].score;
  if (index >= NSAT) {
    score *= NOT_SAT_MULT;
  }
  return score;
}
/*}}}*/
static int do_one_round(struct tower_table *table, struct conflict_table *ctable, int *tried)/*{{{*/
{
  /* Find the tower with the worst conflict count and replace its allocation
   * with something better.  What to do with the case where it already has the
   * colour that gives minimal conflicts though?
   * */

  struct tower2 *tow;
  int worst_score = -1;
  int chosen_tower_index = -1;
  int chosen_new_colour = -1;
  int chosen_new_score = -1;

  int i;

  /* Find tower with the highest conflict count on its present colour */
  for (i=0; i<table->n_towers; i++) {
    struct colour *conf;
    int current_score;
    tow = table->towers[i];
    conf = calloc(NCOLS, sizeof(struct colour));
    score_conflicts(tow->index, table, ctable, conf);
    current_score = get_score(conf, tow->colour_index);
    if (current_score > worst_score) {
      int best_score, best_colour;
      find_best_colour(conf, tried[i], &best_score, &best_colour);
      if ((best_colour != tow->colour_index) && (best_score < current_score)) {
        worst_score = current_score;
        chosen_tower_index = i;
        chosen_new_colour = best_colour;
        chosen_new_score = best_score;
      }
    }
    free_colour_array(conf);
  }

  if (chosen_tower_index >= 0) {
    tow = table->towers[chosen_tower_index];
    printf ("solve_all allocate colour %d (replace %d) to tower %s (%s,%s) : %d->%d conflicts\n",
        chosen_new_colour, tow->colour_index,
        tow->description,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        worst_score, chosen_new_score);
    tow->colour_index = chosen_new_colour;
    tried[chosen_tower_index] |= (1 << chosen_new_colour);
    return 1;
  }

  return 0;
}
/*}}}*/
static void solve_all(struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  int i;
  int iteration;
  int did_anything;
  int *tried;
  /* Audit */
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    int ci = tow->colour_index;
    if (ci >= 0) {
      if ((ci < 1) || (ci >= NCOLS)) {
        fprintf(stderr, "Index for tower <%s> is %d, out of range, forcing to 1\n",
            tow->description, ci);
        tow->colour_index = 1;
      }
    }
  }
  tried = calloc(table->n_towers, sizeof(int));
  for (i=0; i<table->n_towers; i++) {
    tried[i] |= (1 << table->towers[i]->colour_index);
  }

  iteration = 0;
  do {
    did_anything = 0;
    printf("Iteration %d\n", iteration);
    iteration++;
    fflush(stdout);
    if (do_one_round(table, ctable, tried)) {
      did_anything = 1;
    }
  } while (did_anything);
}
/*}}}*/
static void solve2(struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  int i;
  int n_todo;
  unsigned char *flag;
  flag = malloc(table->n_towers);
  memset(flag, 0, table->n_towers);

  /* Assume the tower colour of each tower we must process has been invalidated
   * before we're called. */

  /* Audit and count workload */
  n_todo = 0;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    int ci = tow->colour_index;
    if (ci >= 0) {
      if ((ci < 1) || (ci >= NCOLS)) {
        fprintf(stderr, "Index for tower <%s> is %d, out of range, forced to 1\n",
            tow->description, ci);
        tow->colour_index = 1;
      }
    } else {
      flag[i] = 1;
      ++n_todo;
    }
  }

  printf("%d towers to do\n", n_todo);

  /* Must allocate to everything : do the hardest ones first */
  while (n_todo > 0) {
    struct tower2 *tow;
    int hardest_tower_index = -1;
    int hardest_best_score = -1;
    int colour_for_hardest = -1;
    for (i=0; i<table->n_towers; i++) {
      if (flag[i]) {
        tow = table->towers[i];
        struct colour *conf;
        int best_score, best_colour;
        conf = calloc(NCOLS, sizeof(struct colour));
        score_conflicts(tow->index, table, ctable, conf);
        find_best_colour(conf, 0, &best_score, &best_colour);
        if (best_score > hardest_best_score) {
          /* This tower scores higher, so is 'harder' to allocate to than the previous hardest */
          hardest_tower_index = i;
          hardest_best_score = best_score;
          colour_for_hardest = best_colour;
        }
        free_colour_array(conf);
      }
    }
    if (hardest_tower_index < 0) {
      fprintf(stderr, "Badness happened\n");
      exit(2);
    }
    /* Allocate */
    tow = table->towers[hardest_tower_index];
    tow->colour_index = colour_for_hardest;
    flag[hardest_tower_index] = 0;
    printf ("solve2 allocate colour %d to tower %s (%s,%s) : %d conflicts\n",
        colour_for_hardest, tow->description,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        hardest_best_score);
    fflush(stdout);
    n_todo--;
  }

}
/*}}}*/
static void report(struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  int i;

  printf("======================================\n");
  /* Must allocate to everything : do the hardest ones first */
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow;
    struct colour *conf;
    int best_score, best_colour;
    int j;
    tow = table->towers[i];
    conf = calloc(NCOLS, sizeof(struct colour));
    score_conflicts(tow->index, table, ctable, conf);
    find_best_colour(conf, 0, &best_score, &best_colour);
    printf ("Tower %d %s (%s,%s) :\n",
        i,
        tow->description,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-");
    for (j=0; j<NCOLS; j++) {
      struct contrib *tt;
      if (j == tow->colour_index) {
        printf("  ** ");
      } else {
        printf("     ");
      }
      if ((j == tow->colour_index) || (conf[j].score > 0)) {
        printf("%2d : %d conflicts\n", j, conf[j].score);
        for (tt = conf[j].towers; tt; tt = tt->next) {
          int oi = tt->index;
          struct tower2 *tow2 = table->towers[oi];
          printf("        %8d %3d %s (%s,%s)\n",
              tt->amount,
              oi,
              tow2->description,
              tow2->label_2g ?: "-",
              tow2->label_3g ?: "-");
        }
      }
    }
    fflush(stdout);
    free_colour_array(conf);
  }

}
/*}}}*/
void tcs(struct node *top, struct tower_table *table, int all)/*{{{*/
{
  struct tower_data *data;
  struct conflict_table *ctable;

  data = malloc(table->n_towers * sizeof(struct tower_data));
  make_charts(top, table, data);
  ctable = make_conflict_table(table);
  build_conflicts(table, data, ctable, 0);
  fflush(stdout);
  free_tower_data(data);
  solve2(table, ctable);
  if (all) {
    solve_all(table, ctable);
  }
  report(table, ctable);
  return;
}
/*}}}*/
