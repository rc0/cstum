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

/* Estimation of tower positions */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"
#include "tpgen.h"


#define MAX_ITERS 32
static void optim(struct tower2 *tow)/*{{{*/
{
  int step = 4096;
  int range;
  int xx, yy;
  int n_iters = 0;
  struct M28 m28;
  double lat, lon;
  char *os;
  get_rough_tp_estimate(tow, &xx, &yy);
  printf("===================\n");
  printf("Tower : %s\n", tow->description ? tow->description : "(no description)");
  printf("Captions 2G:%s 3G:%s\n",
      tow->label_2g ? tow->label_2g : "-",
      tow->label_3g ? tow->label_3g : "-");
  m28.X = xx;
  m28.Y = yy;
  m28_to_wgs84(&m28, &lat, &lon);
  os = m28_to_osgb(&m28);
#if VERBOSE
  printf("Starting estimate : X=%10d, Y=%10d [%10.5f,%11.5f] [%s]\n",
      xx, yy, lat, lon, os ? os : "?");
#endif
  free(os);

  range = 4;
  do {
    int i, j;
    int bi, bj;
    double best;
    struct tp_score this;

    this = tp_score(tow, xx, yy, 0);
    if (this.ok == 0) {
      /* No multi-sector panel groups on the tower, so can't optimise position
       * better than rough estimate. */
      break;
    }
    best = this.score;
    bi = bj = 0;

    for (  i = -range; i <= +range; i++) {
      for (j = -range; j <= +range; j++) {
        if ((i != 0) || (j != 0)) {
          int candx = xx + step*i;
          int candy = yy + step*j;
          this = tp_score(tow, candx, candy, 0);
          if (this.score > best) {
            best = this.score;
            bi = i;
            bj = j;
          }
        }
      }
    }
    m28.X = xx;
    m28.Y = yy;
    m28_to_wgs84(&m28, &lat, &lon);
    os = m28_to_osgb(&m28);
#if VERBOSE
    printf("step=%5d bi=%2d bj=%2d xx=%10d yy=%10d [%10.5f,%11.5f] [%s] score=%9.5f\n",
        step, bi, bj, xx, yy,
        lat, lon,
        os ? os : "?",
        best);
#endif
    free(os);
    if ((bi == 0) && (bj == 0)) {
      step >>= 1;
    } else {
      xx += bi * step;
      yy += bj * step;
      if ((abs(bi) < range) && (abs(bj) < range)) {
        /* The best node wasn't at the edge - so there's no trying to walk
         * further with that step size. */
        step >>= 1;
      }
    }
    range = 2;
    n_iters++;
  } while ((n_iters < MAX_ITERS) && (step > 16));
  if (n_iters >= MAX_ITERS) {
    printf("ESTIMATION DID NOT CONVERGE");
  } else {
    if (tow->has_estimated) {
      double dx, dy;
      int xmetres, ymetres, metres;
      dx = (double)(xx - tow->estimated.X);
      dy = (double)(yy - tow->estimated.Y);
      xmetres = (int)(0.5 + 0.09 * dx);
      ymetres = (int)(0.5 + 0.09 * dy);
      metres = (int)(0.5 + 0.09 * sqrt(dx*dx + dy*dy));
      printf("Moved approx (%d,%d)->%d metres\n", xmetres, ymetres, metres);
    }
    tow->estimated.X = xx;
    tow->estimated.Y = yy;
    tow->has_estimated = 1;
  }
}
/*}}}*/
void tpe(struct node *top, struct tower_table *towers)/*{{{*/
{
  int i;
  struct lc_table *lct;
  lct = build_lc_table(towers);
  build_panels(lct, top);
  for (i=0; i<towers->n_towers; i++) {
    if (towers->towers[i]->active_flag) {
      optim(towers->towers[i]);
    }
  }
  free_lc_table(lct);
  return;
}
/*}}}*/
