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

#define NLEVELS 2
#define LEVEL0 16
#define LEVEL1 11

struct conflict/*{{{*/
{
  struct conflict *next;
  int other_index;
  int count;
};
/*}}}*/
struct my_tower/*{{{*/
{
  struct tower2 *t;
  struct conflict *c;
};
/*}}}*/
struct conflict_table/*{{{*/
{
  struct conflict ***c;
};
/*}}}*/
static struct conflict_table *make_conflict_table(struct tower_table *table)/*{{{*/
{
  int nbytes;
  int i;
  struct conflict_table *result;
  result = malloc(sizeof(struct conflict_table));
  nbytes = table->n_towers * sizeof(struct conflict *);
  result->c = malloc(NLEVELS * sizeof(struct conflict **));
  for (i=0; i<NLEVELS; i++) {
    result->c[i] = malloc(nbytes);
    memset(result->c[i], 0, nbytes);
  }
  return result;
}
/*}}}*/
static void add_conflict(struct conflict_table *ctable, int level, int index0, int other_index)/*{{{*/
{
  struct conflict *c;
  for (c = ctable->c[level][index0]; c; c = c->next) {
    if (c->other_index == other_index) break;
  }
  if (c) {
    ++c->count;
  } else {
    c = malloc(sizeof(struct conflict));
    c->other_index = other_index;
    c->count = 1;
    c->next = ctable->c[level][index0];
    ctable->c[level][index0] = c;
  }
}
/*}}}*/

/* Special to Samsung? or to O2? */
#define AMBIGUOUS 50594049

static void accumulate(struct percid *p0, struct percid *p1,/*{{{*/
    struct tower_table *table, struct conflict_table *ctable,
    int level)
{
  struct tower2 *tow0, *tow1;
  int cid0, cid1;
  cid0 = p0->cid;
  cid1 = p1->cid;

  /* Do not process 'special' CIDs */
  if (cid0 == 0) return;
  if (cid0 == AMBIGUOUS) return;
  if (cid1 == 0) return;
  if (cid1 == AMBIGUOUS) return;

  tow0 = cid_to_tower(table, p0->lac, p0->cid);
  tow1 = cid_to_tower(table, p1->lac, p1->cid);
  if (tow0 && tow1) {
    int index0, index1;
    index0 = tow0->index;
    index1 = tow1->index;
    if (index0 != index1) {
      /* avoid conflicting between two cells on the same tower */
      add_conflict(ctable, level, index0, index1);
    }
  }
}
/*}}}*/
static void process(struct percid *p, struct tower_table *table, struct conflict_table *ctable, int level)/*{{{*/
{
  struct percid *p0, *p1;
  for (p0 = p; p0; p0 = p0->next) {
    for (p1 = p; p1; p1 = p1->next) {
      if (p0 != p1) {
        accumulate(p0, p1, table, ctable, level);
      }
    }
  }
}
/*}}}*/
static void inner_build_conflicts(struct node *cursor, int level,/*{{{*/
  struct tower_table *table,
  struct conflict_table *ctable)
{
  if (level == LEVEL0) {
    if (cursor->d) {
      process(cursor->d->cids2, table, ctable, 0);
      process(cursor->d->cids3, table, ctable, 0);
    }
  } else {
    int i, j;
    if (level == LEVEL1) {
      if (cursor->d) {
        process(cursor->d->cids2, table, ctable, 1);
        process(cursor->d->cids3, table, ctable, 1);
      }
    } /* AND do the sublevels too! */
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_build_conflicts(cursor->c[i][j], level+1, table, ctable);
        }
      }
    }
  }
}
/*}}}*/
static void build_conflicts(struct node *top, struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  inner_build_conflicts(top, 0, table, ctable);
}
/*}}}*/

#define NSAT 9
#define NSAT2 (NSAT+NSAT)
#define NCOLS (3*NSAT)

#define REF(r,c) conf[((c)*NSAT) + ((r)%NSAT)]
static void add_scores(int *conf, int ci0, int count, int shift)/*{{{*/
{
  int L, H;
  int scount = count << shift;
  int scountA = scount >> 3;
  int scountB = scount >> 6;
#if 0
  conf[ci0] += scount;
  return;
#endif

  L = ci0 / NSAT;
  H = ci0 % NSAT;
  if (L == 0) { /* the saturated row */
    REF(H,0) += scount;
    /* 1/2 conflict to neighbouring saturated colours */
    REF(H+1, 0) += scountA;
    REF(H-1, 0) += scountA;
    /* 1/4 conflict to 2-away saturated colours */
    REF(H+2, 0) += scountB;
    REF(H-2, 0) += scountB;
    /* and 1/2 conflict to same hue in the dark and light groups */
    REF(H, 1) += scountA;
    REF(H, 2) += scountA;
    /* 1/4 conflict to 1-away dark/light colours */
    REF(H+1, 1) += scountB;
    REF(H-1, 1) += scountB;
    REF(H+1, 2) += scountB;
    REF(H-1, 2) += scountB;
  } else { /* the dark/light row */
    REF(H, L) += scount;
    /* full conflict count onto the neighbouring colours */
    REF(H+1, L) += scount;
    REF(H-1, L) += scount;
    /* 1/2 conflict into the saturated row */
    REF(H, 0) += scountA;
    /* 1/2 conflict into the colours 2 away in dark/light row */
    REF(H+2, L) += scountA;
    REF(H-2, L) += scountA;
    /* 1/4 conflict into the colours 1 away in saturated row */
    REF(H+1, 0) += scountB;
    REF(H-1, 0) += scountB;
  }
}
/*}}}*/
static void score_conflicts(int index, const struct tower_table *table, const struct conflict_table *ctable, int conf[NCOLS])/*{{{*/
{
  struct conflict *c;
  memset(conf, 0, NCOLS * sizeof(int));
  /* Scale close-in conflicts by a large amount to ensure these are taken
   * account of as a priority.  But include further-away conflicts as a
   * tie-breaker */
  for (c = ctable->c[0][index]; c; c = c->next) {
    int ci0 = table->towers[c->other_index]->colour_index;
    if (ci0 >= 0) {
      add_scores(conf, ci0, c->count, 13);
    }
  }
  for (c = ctable->c[1][index]; c; c = c->next) {
    int ci0 = table->towers[c->other_index]->colour_index;
    if (ci0 >= 0) {
      add_scores(conf, ci0, c->count, 0);
    }
  }

}
/*}}}*/

#define NOT_SAT_MULT 4

static void find_best_colour(const int conf[NCOLS], int tried, int *best_score, int *best_colour)/*{{{*/
{
  int best_index, best_so_far;
  int ci;
  best_index = -1;
  for (ci=1; ci<NCOLS; ci++) {
    int this_conf = conf[ci];
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
static int get_score(const int conf[NCOLS], int index)/*{{{*/
{
  int score;
  score = conf[index];
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
    int conf[NCOLS];
    int current_score;
    tow = table->towers[i];
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
        int conf[NCOLS];
        int best_score, best_colour;
        score_conflicts(tow->index, table, ctable, conf);
        find_best_colour(conf, 0, &best_score, &best_colour);
        if (best_score > hardest_best_score) {
          /* This tower scores higher, so is 'harder' to allocate to than the previous hardest */
          hardest_tower_index = i;
          hardest_best_score = best_score;
          colour_for_hardest = best_colour;
        }
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
    n_todo--;
  }

}
/*}}}*/
static void print_conflict_table(struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  int i;
  int lev;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    struct conflict *c;
    printf("=============\n");
    printf("Conflicts for i=%d col %2d : %s (%s,%s)\n",
        i,
        tow->colour_index,
        tow->description,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-"
        );
    for (lev = 0; lev < NLEVELS; lev++) {
      printf("  Level %d : tile %d\n", lev, (lev == 1) ? LEVEL1 : LEVEL0);
      for (c = ctable->c[lev][i]; c; c = c->next) {
        int oi = c->other_index;
        struct tower2 *ot = table->towers[oi];
        printf("  col %2d count %4d : %3d %s (%s,%s)\n",
            ot->colour_index,
            c->count, oi,
            ot->description,
            ot->label_2g ?: "-",
            ot->label_3g ?: "-");
      }
    }
  }

  fflush(stdout);

}
/*}}}*/
static void show_scoring(struct tower_table *table, struct conflict_table *ctable)/*{{{*/
{
  int i;
  int j;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    struct conflict *c;
    int ci = tow->colour_index;
    int level;
    if (ci < 0) {
      fprintf(stderr, "Towers should not have undefined colour index!\n");
      exit(2);
    }
    for (level=0; level<NLEVELS; level++) {
      int counts[NCOLS];
      for (j=0; j<NCOLS; j++) counts[j] = 0;
      if (level > 0) {
        printf(":");
      }
      for (c = ctable->c[level][i]; c; c = c->next) {
        struct tower2 *other_tower = table->towers[c->other_index];
        int other_colour_index = other_tower->colour_index;
        counts[other_colour_index] += c->count;
      }
      for (j=0; j<NCOLS; j++) {
        printf("%5d", counts[j]);
        fputc((j == ci) ? '*' : ' ', stdout);
      }
      if (counts[ci] > 0) {
        int the_min;
        unsigned int map;
        printf(":**");
        the_min = counts[0];
        map = 1;
        for (j=1; j<NCOLS; j++) {
          if (counts[j] < the_min) {
            the_min = counts[j];
            map = 1<<j;
          } else if (counts[j] == the_min) {
            map |= 1<<j;
          }
        }
        /* Now detect if the colour the tower is drawn with is one of those with
         * a minimal conflict count */
        if ((map & (1<<ci)) == 0) {
          printf("!!");
        } else {
          printf("  ");
        }
      } else {
        printf(":    ");
      }
    }
    printf(": (%s,%s) %s\n",
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        tow->description);

  }
}
/*}}}*/
void tcs(struct node *top, struct tower_table *table, int all)/*{{{*/
{
  struct conflict_table *ctable;

  ctable = make_conflict_table(table);
  build_conflicts(top, table, ctable);
  print_conflict_table(table, ctable);
  /* Always allocate the unassigned towers first */
  solve2(table, ctable);
  if (all) {
    solve_all(table, ctable);
  }
#if 0
  show_scoring(table, ctable);
#endif
  return;
}
/*}}}*/
