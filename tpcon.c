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

/* Generating contour plots of the metric used for estimating tower positions
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"
#include "tpgen.h"
#include "contour.h"

static void remap_clines(struct cline *lines, int pertile)/*{{{*/
{
  struct cline *l;
  for (l=lines; l; l=l->next) {
    struct cpoint *p;
    for (p=l->points; p; p=p->next) {
      p->x = (1.0 / (double) pertile) * p->x;
      p->y = (1.0 / (double) pertile) * p->y;
    }
  }
}
/*}}}*/

struct level_info {/*{{{*/
  double level;
  double width;
};
/*}}}*/
#define NL 2
static struct level_info levels[NL] = {/*{{{*/
  {0.995, 2.5},
  {0.95, 5.0}
};
/*}}}*/
static char *colours[6] = {/*{{{*/
  "#ff0000",
  "#00c000",
  "#0000ff",
  "#e000e0",
  "#00e0e0",
  "#c0c000"
};
/*}}}*/

struct params {
  int x0, y0;
  int ni, nj;
  int zoom;
  int step;
  int pertile;
};

static void setup(int xx, int yy, const char *basetiles, const char *svg_name, int zoom, int range, struct params *params)
{
  int xtile, ytile;
  int xtile0, ytile0;
  int nx, ny;

  params->pertile = 16;
  params->zoom = zoom;
  xtile = xx >> (28 - zoom);
  ytile = yy >> (28 - zoom);
  xtile0 = xtile - range;
  ytile0 = ytile - range;
  params->step = (1 << (28 - zoom)) / params->pertile;
  nx = 2*range + 1;
  ny = 2*range + 1;

  params->nj = nx * params->pertile;
  params->ni = ny * params->pertile;
  params->x0 = xtile0 << (28 - zoom);
  params->y0 = ytile0 << (28 - zoom);
  start_svg(basetiles, svg_name, zoom, xtile0, ytile0, nx, ny);
}

static struct cdata *get_cdata_tower(struct tower2 *tow, const struct params *params, double *max_score)/*{{{*/
{
  int i, j;
  struct cdata *cdata;
  struct crow *crow;

  *max_score = 0.0;
  cdata = new_cdata();
  for (i=0; i<params->ni; i++) {
    int X, Y;
    struct tp_score score;
    Y = params->y0 + i * params->step;
    crow = next_crow(cdata);
    for (j=0; j<params->nj; j++) {
      X = params->x0 + j * params->step;
      score = tp_score(tow, X, Y, 0);
      if (score.score > *max_score) {
        *max_score = score.score;
      }
      add_cnode(crow, score.score);
    }
  }

  return cdata;
}
/*}}}*/

static struct cdata *get_cdata_group(struct group *grp, const struct params *params, double *max_score)/*{{{*/
{
  int i, j;
  struct cdata *cdata;
  struct crow *crow;

  *max_score = 0.0;
  cdata = new_cdata();
  for (i=0; i<params->ni; i++) {
    int X, Y;
    struct tp_score score;
    Y = params->y0 + i * params->step;
    crow = next_crow(cdata);
    for (j=0; j<params->nj; j++) {
      X = params->x0 + j * params->step;
      score = tp_score_group(grp, X, Y);
      if (score.score > *max_score) {
        *max_score = score.score;
      }
      add_cnode(crow, score.score);
    }
  }

  return cdata;
}
/*}}}*/

static void contour_cdata(int index, struct cdata *cdata, const struct params *params, double max_score)
{
  int i;
  double level;
  struct cline *lines;
  for (i=0; i<NL; i++) {
    level = levels[i].level;
    lines = generate_isolines(cdata, level * max_score);
    remap_clines(lines, params->pertile);
    emit_svg(level, colours[index] , levels[i].width, lines);
    free_clines(lines);
  }
}

static void do_crosshair(int index, const struct tower2 *tow, const struct params *params)
{
  const struct M28 *m28;
  double xx, yy;
  double scale;
  int dx, dy;
  m28 = tow->has_observed ? &tow->observed :
    tow->has_estimated ? &tow->estimated : NULL;
  if (!m28) return;
  dx = m28->X - params->x0;
  dy = m28->Y - params->y0;
  scale = (double)(1 << (28 - params->zoom));
  xx = (double) dx / scale;
  yy = (double) dy / scale;
  svg_cross(colours[index], 3.0, xx, yy);
}

static void add_cog(struct tower2 *tow, int *xx, int *yy)/*{{{*/
{
  int x, y;
  if (tow->has_observed) {
    x = tow->observed.X;
    y = tow->observed.Y;
  } else if (tow->has_estimated) {
    x = tow->estimated.X;
    y = tow->estimated.Y;
  } else {
    get_rough_tp_estimate(tow, &x, &y);
  }
  *xx += x;
  *yy += y;
}
/*}}}*/
static void get_tower_cog(int n_towers, struct tower2 **towers, int *X, int *Y)/*{{{*/
{
  int xx, yy;
  int i;
  xx = yy = 0;
  for (i=0; i<n_towers; i++) {
    struct tower2 *tow = towers[i];
    add_cog(tow, &xx, &yy);
  }
  *X = xx / n_towers;
  *Y = yy / n_towers;
}
/*}}}*/
static void get_group_cog(int n_groups, struct group **groups, int *X, int *Y)/*{{{*/
{
  int xx, yy;
  int i;
  xx = yy = 0;
  for (i=0; i<n_groups; i++) {
    struct tower2 *tow = groups[i]->tower;
    add_cog(tow, &xx, &yy);
  }
  *X = xx / n_groups;
  *Y = yy / n_groups;
}
/*}}}*/
void tpcon_towers(struct node *top, struct tower_table *table,/*{{{*/
    int n_towers, struct tower2 **tow,
    const char *basetiles,
    const char *svg_name, int zoom, int range)
{
  int i;
  struct lc_table *lct;
  struct params params;
  int X, Y;
  for (i=0; i<n_towers; i++) {
    tow[i]->active_flag = 1;
  }
  lct = build_lc_table(table);
  build_panels(lct, top);
  get_tower_cog(n_towers, tow, &X, &Y);
  setup(X, Y, basetiles, svg_name, zoom, range, &params);
  for (i=0; i<n_towers; i++) {
    struct cdata *cdata;
    double max_score;
    cdata = get_cdata_tower(tow[i], &params, &max_score);
    contour_cdata(i, cdata, &params, max_score);
    do_crosshair(i, tow[i], &params);
    free_cdata(cdata);
  }
  finish_svg();
  return;
}
/*}}}*/

void tpcon_groups(struct node *top, struct tower_table *table,/*{{{*/
    int n_groups, struct group **grp,
    const char *basetiles,
    const char *svg_name, int zoom, int range)
{
  int i;
  struct lc_table *lct;
  struct params params;
  int X, Y;
  for (i=0; i<table->n_towers; i++) {
    table->towers[i]->active_flag = 1;
  }
  lct = build_lc_table(table);
  build_panels(lct, top);
  get_group_cog(n_groups, grp, &X, &Y);
  setup(X, Y, basetiles, svg_name, zoom, range, &params);
  for (i=0; i<n_groups; i++) {
    struct cdata *cdata;
    double max_score;
    cdata = get_cdata_group(grp[i], &params, &max_score);
    contour_cdata(i, cdata, &params, max_score);
    free_cdata(cdata);
  }
  finish_svg();
  return;
}
/*}}}*/
