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
#include <stdint.h>
#include <assert.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"
#include "tpgen.h"

/* Deal with 2, 3, 4, 6 panel arrangements */
#define NSECT 12
#define PERSECT 3
#define NBINS (NSECT*PERSECT)

static void header(void)
{
  printf("           "
      "N           "
      "E           "
      "S           "
      "W           "
      "N           "
      "E           "
      "S\n");
}

static void display(struct tower2 *tow)
{
  struct group *grp;
  int X28, Y28;
  if (tow->has_observed) {
    X28 = tow->observed.X;
    Y28 = tow->observed.Y;
  } else if (tow->has_estimated) {
    X28 = tow->estimated.X;
    Y28 = tow->estimated.Y;
  } else {
    printf("Cannot process %s (%s:%s), no known position\n",
      tow->description ? tow->description : "(no description)",
      tow->label_2g ? tow->label_2g : "-",
      tow->label_3g ? tow->label_3g : "-");
    return;
  }

  printf("Tower %s (%s:%s):\n",
    tow->description ? tow->description : "(no description)",
    tow->label_2g ? tow->label_2g : "-",
    tow->label_3g ? tow->label_3g : "-");
  header();
  for (grp = tow->groups; grp; grp = grp->next) {
    struct panel *pan;
    for (pan = grp->panels; pan; pan = pan->next) {
      if (pan->cells) {
        printf("%10d :", pan->cells->cid);
      } else {
        printf("%10s :", "[-]");
      }
      if (pan->data) {
        int count[NBINS];
        int total;
        int i, ii;
        double av, t0, t1;
        struct panel_data *d;
        memset(count, 0, sizeof(count));
        total = 0;
        for (d = pan->data; d; d = d->next) {
          int32_t dx, dy;
          int32_t ang32;
          int bin;
          dx = (128 + (d->I << 8)) - X28;
          dy = (128 + (d->J << 8)) - Y28;
          ang32 = atan2i_approx2(dy, dx) + 0x40000000;
          bin = (int)(((uint64_t)NBINS * (uint64_t) (uint32_t) ang32) >> 32);
          assert(bin >= 0);
          assert(bin < NBINS);
          ++count[bin];
          ++total;
        }
        av = (double) total / (double) NBINS;
        t0 = 1.0*av;
        t1 = 2.0*av;
        for (i=0, ii=0; i<NBINS+(NBINS>>1); i++) {
          double x = (double) count[i%NBINS];
          char c;
#if 0
          c = (x > t1) ? '#' :
            (x > t0) ? '*' :
            (x > 0.0) ? ' ' : ' ';
#else
          c = (x > t0) ? '*' : ' ';
#endif
          putchar(c);
          ++ii;
          if (ii == PERSECT) {
            putchar('|');
            ii = 0;
          }
        }
      }
      if (pan->cells) {
        printf(" %d", pan->cells->cid);
      } else {
        printf("[-]");
      }
      printf("\n");
    }
  }
  header();

}

void tpr(struct node *top, struct tower_table *towers)/*{{{*/
{
  int i;
  struct lc_table *lct;
  lct = build_lc_table(towers);
  build_panels(lct, top);
  for (i=0; i<towers->n_towers; i++) {
    if (towers->towers[i]->active_flag) {
      display(towers->towers[i]);
    }
  }
  free_lc_table(lct);
  return;
}
/*}}}*/

/* vim:et:sts=2:sw=2:ht=2
 * */

