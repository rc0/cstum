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

/* Functions relating to the estimation of tower locations.  Shared by tpe and
 * tpcon, hence 'tower position general' -> tpgen. */

#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <assert.h>
#include <math.h>
#include "tool.h"
#include "tp_type.h"
#include "tpgen.h"

static struct tpe_lc *add_lc(struct tpe_lc *in, struct panel *panel, int lac, int cid)/*{{{*/
{
  struct tpe_lc *result;
  result = malloc(sizeof(struct tpe_lc));
  result->next = in;
  result->panel = panel;
  result->lac = lac;
  result->cid = cid;
  return result;
}
/*}}}*/
static struct tpe_lc *add_lcs(struct tpe_lc *in, struct tower2 *tow)/*{{{*/
{
  struct tpe_lc *list;
  struct group *grp;
  list = in;
  for (grp = tow->groups; grp; grp = grp->next) {
    struct panel *pan;
    for (pan = grp->panels; pan; pan = pan->next) {
      struct cell *c;
      for (c = pan->cells; c; c = c->next) {
        int j;
        switch (c->gen) {
          case CELL_G:
            for (j=0; j<tow->n_lacs_g; j++) {
              list = add_lc(list, pan, tow->lacs_g[j], c->cid);
            }
            break;
          case CELL_U:
            for (j=0; j<tow->n_lacs_u; j++) {
              list = add_lc(list, pan, tow->lacs_u[j], c->cid);
            }
            break;
        }
      }
    }
  }
  return list;
}
/*}}}*/
int compare_lc(int lac0, int cid0, int lac1, int cid1)/*{{{*/
{
  if (cid0 < cid1) return -1;
  else if (cid0 > cid1) return +1;
  else if (lac0 < lac1) return -1;
  else if (lac0 > lac1) return +1;
  else return 0;
}
/*}}}*/
static int compare_tpe_lc(const void *aa, const void *bb)/*{{{*/
{
  const struct tpe_lc *a = *(const struct tpe_lc **)aa;
  const struct tpe_lc *b = *(const struct tpe_lc **)bb;
  return compare_lc(a->lac, a->cid, b->lac, b->cid);
}
/*}}}*/
struct lc_table *build_lc_table(struct tower_table *towers)/*{{{*/
{
  struct lc_table *result;
  int i;
  int nc;
  struct tpe_lc *list = NULL, *p;
  result = calloc(1, sizeof(struct lc_table));
  for (i=0; i<towers->n_towers; i++) {
    struct tower2 *tow = towers->towers[i];
    if (tow->active_flag) {
      list = add_lcs(list, tow);
    }
  }
  for (nc=0, p = list; p; nc++, p = p->next) {}
  result->n_lcs = nc;
  result->lcs = malloc(nc * sizeof(struct tpe_lc *));
  for (i=0, p=list; i<nc; i++, p=p->next) {
    result->lcs[i] = p;
  }
  qsort(result->lcs, result->n_lcs, sizeof(struct tpe_lc *), compare_tpe_lc);
  return result;
}
/*}}}*/
void free_lc_table(struct lc_table *lct)/*{{{*/
{
  if (lct->n_lcs > 0) {
    free(lct->lcs);
  }
  free(lct);
}
/*}}}*/
struct panel *lookup_panel(struct lc_table *lct, int lac, int cid)/*{{{*/
{
  int L, H, M;
  L = 0;
  H = lct->n_lcs;
  for (;;) {
    int status;
    M = (L + H) >> 1;
    status = compare_lc(lac, cid, lct->lcs[M]->lac, lct->lcs[M]->cid);

    if (status == 0) return lct->lcs[M]->panel;
    if (M == L) return NULL;
    if (status > 0) L = M;
    if (status < 0) H = M;
  }
}
/*}}}*/

static int max_reading(struct readings *r)/*{{{*/
{
  int i;
  for (i=31; i>=0; i--) {
    if (r->asus[i] > 0) {
      return i;
    }
  }
  return 0;
}
/*}}}*/
static void gather(struct lc_table *lct, struct percid *pc, int I, int J)/*{{{*/
{
  for (; pc; pc = pc->next) {
    struct panel *pan = lookup_panel(lct, pc->lac, pc->cid);
    struct readings *r = lookup_readings(pc, 0);
    if (pan) {
      struct panel_data *data;
      data = malloc(sizeof(struct panel_data));
      data->next = pan->data;
      data->I = I;
      data->J = J;
      data->n = pan->data ? (pan->data->n + 1) : 1;
      data->max_asu = max_reading(r);
      pan->data = data;
    }
  }
}
/*}}}*/
static void inner_build_panels(struct lc_table *lct, struct node *cursor, int level, int I, int J)/*{{{*/
{
  int i, j;
  if (level == 20) {
    if (cursor->d) {
      gather(lct, cursor->d->cids2, I, J);
      gather(lct, cursor->d->cids3, I, J);
    }
  } else {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_build_panels(lct, cursor->c[i][j], level+1, (I<<1)+i, (J<<1)+j);
        }
      }
    }
  }
}
/*}}}*/
void build_panels(struct lc_table *lct, struct node *top)/*{{{*/
{
  inner_build_panels(lct, top, 0, 0, 0);
}
/*}}}*/
void get_rough_tp_estimate(const struct tower2 *tow, int *X, int *Y)/*{{{*/
{
  double XX, YY;
  int total_weight;
  struct group *grp;
  XX = 0.0;
  YY = 0.0;
  total_weight = 0.0;
  for (grp = tow->groups; grp; grp = grp->next) {
    struct panel *pan;
    for (pan = grp->panels; pan; pan = pan->next) {
      struct panel_data *d;
      for (d = pan->data; d; d = d->next) {
        double score;
        /* Yes, asu is more like proportional to dBm than to mW.  TBD whether
         * this is a fair compromise between not weighting at all, and the
         * hugely biased weighting that occurs from turning asu into a mW-like
         * scaling. */
        score = (double) d->max_asu;
        score *= score;
        total_weight += score;
        XX += score * (double) (d->I << 8);
        YY += score * (double) (d->J << 8);
      }
    }
  }

  *X = (int)(0.5 + XX / total_weight);
  *Y = (int)(0.5 + YY / total_weight);
}
/*}}}*/

static double get_bearing(int32_t ang)
{
  uint32_t ang2 = ang;
  uint64_t ang3 = ang2;
  uint64_t bearing1000 = (360000 * ang3) >> 32;
  return 0.001 * (double) bearing1000;
}

#define K1 (0.97239f)
#define K2 (0.19195f)
#define K3 ((float)((double)(1<<29) * 4.0 * K1 / M_PI))
#define K4 ((float)((double)(1<<29) * 4.0 * K2 / M_PI))

int32_t atan2i_approx2(int32_t y, int32_t x)
{
  int32_t u, v, s;
  uint32_t w;
  float f, f2, g;
  int32_t r;
  float fx, fy;
  float tt, bb;

  fx = (float) x;
  fy = (float) y;
  u = x + y;
  v = x - y;
  w = (u ^ v) >> 31;
  tt = (w) ? -fx : fy;
  bb = (w) ? fy : fx;
  f = tt / bb;
  f2 = f * f;
  g = f * (K3 - K4*f2);
  s = (((u >> 30) & 3) ^ ((v >> 31) & 1)) << 30;
  r = s + (int32_t) g;
  return r;
}

struct tp_score tp_score_group(const struct group *grp, int X28, int Y28)/*{{{*/
{
  struct panel *pan;
  int n_panels, i;
  int np, pp;
  struct point *points;
  struct final *finals;
  struct final *scratch;
  struct tp_score result;
  int C[6], best;
  int32_t move;
  int32_t spread;
  int32_t skew_per_panel, skew_so_far;
  for (pan = grp->panels, n_panels=0, np=0; pan; pan = pan->next, n_panels++) {
    if (pan->data) np += pan->data->n;
  }

  if (np == 0) {
    fprintf(stderr, "No readings to work from?\n");
    exit(2);
  }

  if (n_panels <= 1) {
    /* Omnidirectional cell.  If this is the only cell on the tower, the
     * initial (weighted centroid) estimate is the best we can do.  Otherwise,
     * rely on the other cell groups to provide the phase solution */
    result.score = 0.0;
    result.ok = 0;
    return result;
  }

  points = malloc(np * sizeof(struct point));
  finals = malloc(6 * np * sizeof(struct final));
  scratch = malloc(6 * np * sizeof(struct final));
  skew_so_far = 0x0;
  skew_per_panel = 0xffffffffU / n_panels;
  memset(C, 0, sizeof(C));
  for (i = 0, pan = grp->panels, pp = 0;
      i < n_panels;
      i++, pan = pan->next, skew_so_far += skew_per_panel) {
    if (pan->data) {
      double contrib = 1.0 / (double) pan->data->n;
      struct panel_data *d;
      for (d = pan->data; d; d = d->next) {
        int32_t dx, dy;
        int32_t ang32;
        uint32_t pa;
        int pa6;
        dx = (128 + (d->I << 8)) - X28;
        dy = (128 + (d->J << 8)) - Y28;
        ang32 = atan2i_approx2(dy, dx);
        points[pp].angle = ang32 - skew_so_far;
        pa = (uint32_t) (0x80000000 + ang32 - skew_so_far);
        pa6 = (int)((6 * (uint64_t) pa) >> 32);
        ++C[pa6];
        points[pp].contrib = contrib;
        pp++;
      }
    }
  }
  if (pp != np) {
    fprintf(stderr, "Some points were dropped\n");
  }

  best = C[0] + C[1];
  move = 0x55555555;
  if      (C[1] + C[2] > best) { best = C[1] + C[2]; move = 0x2AAAAAAA; }
  else if (C[2] + C[3] > best) { best = C[2] + C[3]; move = 0x00000000; }
  else if (C[3] + C[4] > best) { best = C[3] + C[4]; move = 0xD5555555; }
  else if (C[4] + C[5] > best) { best = C[4] + C[5]; move = 0xAAAAAAAA; }
  else if (C[5] + C[0] > best) { best = C[5] + C[0]; move = 0x80000000; }

  spread = 0x40000000 / n_panels;

  for (i=0; i<np; i++) {
    int32_t t = points[i].angle + move;
    t >>= 1;
    finals[6*i + 0].x = t + 0x20000000;
    finals[6*i + 1].x = t - 0x20000000;
    finals[6*i + 2].x = t + spread;
    finals[6*i + 3].x = t - spread;
    finals[6*i + 4].x = t + (spread >> 1);
    finals[6*i + 5].x = t - (spread >> 1);
    finals[6*i + 0].d = -points[i].contrib;
    finals[6*i + 1].d = +points[i].contrib;
    finals[6*i + 2].d = -points[i].contrib * 0.25f;
    finals[6*i + 3].d = +points[i].contrib * 0.25f;

    finals[6*i + 4].d = -points[i].contrib * 0.05f;
    finals[6*i + 5].d = +points[i].contrib * 0.05f;
  }

  sort_finals(6*np, finals, scratch);

  do {
    double run, best;
    int32_t first, last;
    int32_t ang;
    run = 0;
    best = 0;
    first = 0;
    last = 0;
    for (i=0; i<6*np; i++) {
      run += finals[i].d;
      if (run > best) {
        best = run;
        first = finals[i].x;
      }
      if (run >= best) {
        last = finals[i].x;
      }
    }
    ang = (first >> 1) + (last >> 1);
    ang <<= 1; /* restore to full circle from half-circle */
    ang -= move;
    ang += 0x40000000; /* from E-pointing to N-pointing zero */
    result.bearing = get_bearing(ang);
    /* unused elsewhere, it never worked well... */
    result.half_spread = 0;
    result.score = best;
  } while (0);

  free(points);
  free(finals);
  free(scratch);
  result.ok = 1;
  return result;
}
/*}}}*/
static int first_cid(struct group *grp)/*{{{*/
{
  struct panel *pan;
  for (pan = grp->panels; pan; pan = pan->next) {
    struct cell *c;
    for (c = pan->cells; c; c = c->next) {
      return c->cid;
    }
  }
  return 0;
}
/*}}}*/
struct tp_score tp_score(struct tower2 *tow, int X28, int Y28, int verbose)/*{{{*/
{
  /* compute a score for this tower assuming that position */
  struct group *grp;
  struct tp_score result;
  result.ok = 0;
  result.score = 0.0;
  for (grp = tow->groups; grp; grp = grp->next) {
    struct tp_score group_score;
    group_score = tp_score_group(grp, X28, Y28);
    if (group_score.ok) {
      grp->has_bearing = 1;
      grp->bearing = group_score.bearing;
      grp->half_spread = group_score.half_spread;
      result.ok = 1;
    }
    result.score += group_score.score;
    if (verbose) {
      printf("Group %d... : %9.5f", first_cid(grp), group_score.score);
      if (grp->has_bearing) {
        printf(" (%03d degrees)", (int)grp->bearing);
      }
      printf("\n");
    }
  }
  return result;
}
/*}}}*/
