/************************************************************************
 * Copyright (c) 2012, 2013, 2014 Richard P. Curnow
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

#define MIN_LEVEL (10+4)

#define RANGE 3

/* Colour 0 is assumed to be reserved */
#define N_COLOURS 15
/* Number of 'vivid' colours, which are preferentially allocated to the towers
 * which have a bigger display area. */
#define N_COLOURS2 8

static int conflict_penalty[N_COLOURS][N_COLOURS];
static int paint_penalty[N_COLOURS];


#define THRESHOLD 4

#define DARK_PENALTY 1
#define LIGHT_PENALTY 2
#define SAME_COLOUR_PENALTY 100
#define BOTH_DARK_PENALTY 20
#define BOTH_LIGHT_PENALTY 4
#define SIMILAR_PENALTY 3

static void init_colour_scores(void)
{
  /* TODO : initialise this from a file */
  int i, j;
  memset(conflict_penalty, 0, sizeof(conflict_penalty));
  memset(paint_penalty, 0, sizeof(paint_penalty));
  for (i=0; i<N_COLOURS; i++) {
    if (i <= 7) {
      paint_penalty[i] = 0; /* vivid colours */
    } else if (i <= 10) {
      paint_penalty[i] = LIGHT_PENALTY;
    } else if (i <= 14) {
      paint_penalty[i] = DARK_PENALTY;
    }

    for (j=0; j<N_COLOURS; j++) {
      if (i == j) {
        conflict_penalty[i][j] = SAME_COLOUR_PENALTY;
      } else if ((i>=11) && (i<=14) && (j>=11) && (j<=14)) {
        conflict_penalty[i][j] = BOTH_DARK_PENALTY;
      } else if ((i>=8) && (i<=10) && (j>=8) && (j<=10)) {
        conflict_penalty[i][j] = BOTH_LIGHT_PENALTY;
      } else {
        if ((i == 8) && (j == 0)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        } else if ((i == 8) && (j == 7)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        } else if ((i == 9) && (j == 2)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        } else if ((i == 9) && (j == 3)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        } else if ((i == 10) && (j == 6)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        } else if ((i == 10) && (j == 7)) {
          conflict_penalty[i][j] = SIMILAR_PENALTY;
        }
      }
    }
  }
}

static void do_tile_towers(int level, struct node *cursor, int I, int J, struct tower_table *table)/*{{{*/
{
  struct odata *od;
  struct tower2 *tow;
  cursor->d->t2 = NULL;
  od = compute_odata(cursor->d, 2);
  if (od) {
    if (od->n_lcs > 0) {
      cursor->d->t2 = tow = cid_to_tower(table, od->lcs[0].lac, od->lcs[0].cid);
      if (tow) ++tow->display_count;
    }
    free_odata(od);
  }

  cursor->d->t3 = NULL;
  od = compute_odata(cursor->d, 3);
  if (od) {
    if (od->n_lcs > 0) {
      cursor->d->t3 = tow = cid_to_tower(table, od->lcs[0].lac, od->lcs[0].cid);
      if (tow) ++tow->display_count;
    }
    free_odata(od);
  }
}
/*}}}*/
static void inner_find(int level, struct node *cursor, int I, int J, struct tower_table *table)/*{{{*/
{
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_find(level+1, cursor->c[i][j], (I<<1)+i, (J<<1)+j, table);
      }
    }
  }
  if ((level >= MIN_LEVEL)) {
    do_tile_towers(level, cursor, I, J, table);
  }
}
/*}}}*/
static void find_display_towers(struct node *top, struct tower_table *table)/*{{{*/
{
  inner_find(0, top, 0, 0, table);
}
/*}}}*/

struct conflict/*{{{*/
{
  struct conflict *next;
  int index;
  int score; /* Higher scores if the conflicts are closer together */
};
/*}}}*/
static void add_conflict(struct conflict **conf, struct tower2 *t0, struct tower2 *t1, int score)/*{{{*/
{
  int i0, i1;
  struct conflict *c;
  i0 = t0->index;
  i1 = t1->index;
  if (i0 == i1) return;
  c = conf[i0];
  while (c) {
    if (c->index == i1) {
      c->score += score;
      return;
    } else {
      c = c->next;
    }
  }

  c = malloc(sizeof(struct conflict));
  c->index = i1;
  c->score = score;
  c->next = conf[i0];
  conf[i0] = c;
}
/*}}}*/
static void do_neighbour_v2(int level, struct node *top, struct tower2 *t2, struct tower2 *t3, int ii, int jj, struct conflict **conf, int score)/*{{{*/
{
  /* The v1 version only bothers about colour conflicts within either the 2g
   * map or the 3g map.  The problem is that there can be different nearby
   * sites on the 2 maps that get the same colour, and this is confusing to
   * look at if you're switching between the maps whilst viewing.  So this
   * version considers conflicts between the maps as well. */
  struct node *neighbour;
  neighbour = lookup_node(top, level, ii, jj);
  if (neighbour) {
    struct data *nd = neighbour->d;
    if (t2) {
      if (nd->t2) {
        add_conflict(conf, t2, nd->t2, score);
        add_conflict(conf, nd->t2, t2, score);
      }
      if (nd->t3) {
        add_conflict(conf, t2, nd->t3, score);
        add_conflict(conf, nd->t3, t2, score);
      }
    }
    if (t3) {
      if (nd->t2) {
        add_conflict(conf, t3, nd->t2, score);
        add_conflict(conf, nd->t2, t3, score);
      }
      if (nd->t3) {
        add_conflict(conf, t3, nd->t3, score);
        add_conflict(conf, nd->t3, t3, score);
      }
    }
  }
}
/*}}}*/
static void do_tile_conflicts(int level, struct node *top, struct node *cursor, int I, int J, struct conflict **conf)/*{{{*/
{
  struct tower2 *t2, *t3;
  int range = RANGE;
  int di, dj;
  int score;
  t2 = cursor->d->t2;
  t3 = cursor->d->t3;
  if (t2 || t3) {
    /* Scan rightwards and below only to halve the work and avoid double counting */
    for (di=1; di<=range; di++) {
      score = 1; /* TODO : fix */
      do_neighbour_v2(level, top, t2, t3, I+di, J, conf, score);
    }
    for (dj=1; dj<=range; dj++) {
      for (di=-range; di<=range; di++) {
        score = 1;
        do_neighbour_v2(level, top, t2, t3, I+di, J+dj, conf, score);
      }
    }
  }
}
/*}}}*/
static void inner_walk(int level, struct node *top, struct node *cursor, int I, int J, struct conflict **conf)/*{{{*/
{
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_walk(level+1, top, cursor->c[i][j], (I<<1)+i, (J<<1)+j, conf);
      }
    }
  }
  if ((level >= MIN_LEVEL)) {
    do_tile_conflicts(level, top, cursor, I, J, conf);
  }
}
/*}}}*/
static void walk(struct node *top, struct conflict **conf)/*{{{*/
{
  inner_walk(0, top, top, 0, 0, conf);
}
/*}}}*/
static void display_foo(struct tower_table *table, struct conflict **conf)/*{{{*/
{
  int i;
  int conf_num;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow;
    struct conflict *c;
    tow = table->towers[i];
    printf("Tower %4d : dc=%d (%s,%s) %s\n",
        i,
        tow->display_count,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        tow->description);
    c = conf[tow->index];
    conf_num = 0;
    printf("  ##: conf#      dcnt#  idx labels\n");
    while (c) {
      struct tower2 *tc = table->towers[c->index];
      printf("  %2d: %5d %6d %4d (%s,%s) %s\n",
          conf_num,
          c->score,
          tc->display_count,
          tc->index,
          tc->label_2g ?: "-",
          tc->label_3g ?: "-",
          tc->description);
      conf_num++;
      c = c->next;
    }
    printf("------------------------------------\n");
  }
}
/*}}}*/
struct conflict2/*{{{*/
{
  int index;
  int score;
};
/*}}}*/
struct conflict_per_tower/*{{{*/
{
  int n;
  struct conflict2 *list;
};
/*}}}*/
struct conflict_table/*{{{*/
{
  struct conflict_per_tower *table;
};
/*}}}*/
static struct conflict_table *make_conflict_table(int n, struct conflict **conf)/*{{{*/
{
  struct conflict_table *result;
  int i;
  result = malloc(sizeof(struct conflict_table));
  result->table = calloc(n, sizeof(struct conflict_per_tower));
  for (i=0; i<n; i++) {
    struct conflict *c;
    struct conflict_per_tower *e = result->table + i;
    int nc;
    int j;
    for (c=conf[i], nc=0; c; c=c->next, nc++) {}
    e->n = nc;
    e->list = malloc(nc * sizeof(struct conflict2));
    for (c=conf[i], j=0; c; c=c->next, j++) {
      e->list[j].index = c->index;
      e->list[j].score = c->score;
    }
  }
  return result;
}
/*}}}*/

static inline int colgroup(int index)/*{{{*/
{
  if (index < 0) return 0;
  else if (index >= N_COLOURS) return 0;
  else if (index >= N_COLOURS2) return 2;
  else return 1;
}
/*}}}*/
static void score_tower(struct tower_table *table, int index, struct conflict_per_tower *cpt, int score[N_COLOURS])/*{{{*/
{
  int i;
  struct tower2 *tow1;
  memset(score, 0, N_COLOURS * sizeof(int));
  for (i=0; i<cpt->n; i++) {
    struct conflict2 *c = cpt->list + i;
    int other_colour;
    int other_score;
    /* Truncate small scores to zero. */
    other_score = c->score / THRESHOLD;
    tow1 = table->towers[c->index];
    other_colour = tow1->colour_index;
    if ((other_colour >= 0) && (other_colour < N_COLOURS)) {
      if (other_score > score[other_colour]) {
        score[other_colour] = other_score;
      }
    }
  }
}
/*}}}*/
static void score_tower_v2(struct tower_table *table, int index, struct conflict_per_tower *cpt, int score[N_COLOURS])/*{{{*/
{
  int i, j;
  struct tower2 *tow0, *tow1;
  memset(score, 0, N_COLOURS * sizeof(int));
  tow0 = table->towers[index];
  for (i=0; i<N_COLOURS; i++) {
    score[i] = paint_penalty[i] * tow0->display_count;
  }
  for (i=0; i<cpt->n; i++) {
    struct conflict2 *c = cpt->list + i;
    int other_colour;
    int other_score;
    /* Truncate small scores to zero. */
    other_score = c->score / THRESHOLD;
    tow1 = table->towers[c->index];
    other_colour = tow1->colour_index;
    if ((other_colour >= 0) && (other_colour < N_COLOURS)) {
      for (j=0; j<N_COLOURS; j++) {
        score[j] += other_score * conflict_penalty[j][other_colour];
      }
    }
  }
}
/*}}}*/
static unsigned char *initialise_flags(struct tower_table *table, int all, int verbose)/*{{{*/
{
  int i, nt;
  unsigned char *flags;

  nt = table->n_towers;
  flags = calloc(nt, 1);
  if (all) {
    for (i=0; i<nt; i++) flags[i] = 1;
  } else {
    for (i=0; i<nt; i++) {
      struct tower2 *tow = table->towers[i];
      if (tow->colour_index < 0) {
        printf("Doing tower (%s,%s) (%s) : no colour assigned\n",
            tow->label_2g ?: "-",
            tow->label_3g ?: "-",
            tow->description);
        flags[i] = 1;
      } else if ((tow->colour_index == 0) || (tow->colour_index >= N_COLOURS)) {
        printf("Doing tower (%s,%s) (%s) : colour index %d out of range\n",
            tow->label_2g ?: "-",
            tow->label_3g ?: "-",
            tow->description,
            tow->colour_index);
        flags[i] = 1;
      }
    }
  }
  return flags;
}
/*}}}*/
static void solve(struct tower_table *table, struct conflict_table *ctable, int all, int verbose)/*{{{*/
{
  unsigned char *flags;
  int nt;
  int nsince, index;
  nt = table->n_towers;
  flags = initialise_flags(table, all, verbose);

  nsince = 0;
  index = 0;
  do {
    nsince++;
    if (flags[index]) {
      int score[N_COLOURS];
      int i;
      int picked;
      struct conflict_per_tower *cpt = ctable->table + index;
      struct tower2 *tow = table->towers[index];
      int limit;
      score_tower_v2(table, index, cpt, score);
      limit = N_COLOURS;
      if ((tow->colour_index > 0) && (tow->colour_index < limit)) {
        /* Keep the current colour if that has the minimum weight available */
        picked = tow->colour_index;
      } else {
        /* But start safe if the present colour is invalid */
        picked = 1;
      }
      for (i=1; i<limit; i++) {
        if (score[i] < score[picked]) {
          /* Only change colour for a strict improvement */
          picked = i;
        } else if ((score[i] == score[picked]) &&
            (i < N_COLOURS2) && (picked >= N_COLOURS2)) {
          /* Try to promote to a better class of colour, all things being equal */
          picked = i;
        }
      }
      if (picked != tow->colour_index) {
        printf("Assign colour %2d (was %2d, score %5d->%5d) to tower %4d (%s,%s) : %s\n",
            picked, tow->colour_index,
            ((tow->colour_index >= 0) && (tow->colour_index < N_COLOURS)) ? score[tow->colour_index] : 0,
            score[picked],
            index,
            tow->label_2g ?: "-",
            tow->label_3g ?: "-",
            tow->description);
        tow->colour_index = picked;
        nsince = 0;
      }
    }
    index++;
    if (index >= nt) index = 0;

  } while (nsince <= nt);

  free(flags);
  return;
}
/*}}}*/
static void show_verbose_report(struct tower_table *table, struct conflict_per_tower *cpt, int conf_index)/*{{{*/
{
  int i;
  int j;
  for (j=1; j<N_COLOURS; j++) {
    for (i=0; i<cpt->n; i++) {
      struct conflict2 *c = cpt->list + i;
      struct tower2 *tow = table->towers[c->index];
      if (j == tow->colour_index) {
        printf("   %2s %2d %4d %6d %5d (%s,%s) %s\n",
            (j == conf_index) ? "**" : "  ",
            j,
            c->index,
            tow->display_count,
            c->score,
            tow->label_2g ?: "-",
            tow->label_3g ?: "-",
            tow->description);
      }
    }
  }
}
/*}}}*/
static void report(struct tower_table *table, struct conflict_table *ctable, int verbose)/*{{{*/
{
  int i, j;
  int score[N_COLOURS];
  int min_pos;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    score_tower(table, i, ctable->table + i, score);
    min_pos = 1;
    for (j=2; j<N_COLOURS; j++) {
      if (score[j] < score[min_pos]) min_pos = j;
    }
    printf("Tower %4d dc=%6d col=%2d (score %5d, min %5d at %2d) (%s,%s) : %s\n",
        i,
        tow->display_count,
        tow->colour_index,
        score[tow->colour_index], score[min_pos], min_pos,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        tow->description);
    if (verbose) {
      show_verbose_report(table, ctable->table + i, tow->colour_index);
      printf("----------------------------\n");
    }
  }
}
/*}}}*/
void tcs(struct node *top, struct tower_table *table, int all, int verbose)/*{{{*/
{
  struct conflict **conf;
  struct conflict_table *ctable;
  init_colour_scores();
  find_display_towers(top, table);
  conf = calloc(table->n_towers, sizeof(struct conflict *));
  walk(top, conf);
  if (verbose) {
    printf("=== CONFLICT DATA\n");
    display_foo(table, conf);
  }
  ctable = make_conflict_table(table->n_towers, conf);
  if (verbose) {
    printf("=== SOLUTION\n");
  }
  solve(table, ctable, all, verbose);
  if (verbose) {
    printf("=== BRIEF REPORT\n");
    report(table, ctable, 0);
    printf("=== FULL REPORT\n");
    report(table, ctable, 1);
  }
}
/*}}}*/

/* vim:et:sts=2:sw=2:ht=2
 * */
