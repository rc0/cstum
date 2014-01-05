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
#include <assert.h>
#include "tool.h"

#define MAX_LCS 1024
#define MAX_DD 1024


struct work/*{{{*/
{
  int total;
};
/*}}}*/
static int lookup_lc(struct laccid *lc, int nlc, struct percid *pp)/*{{{*/
{
  int i;
  for (i=0; i<nlc; i++) {
    if ((lc[i].cid == pp->cid) && (lc[i].lac == pp->lac)) {
      return i;
    }
  }
  return -1;
}
/*}}}*/
static int lookup_dd(int *dd, int ndd, struct readings *r)/*{{{*/
{
  int i;
  for (i=0; i<ndd; i++) {
    if (dd[i] == r->timeframe) {
      return i;
    }
  }
  return -1;
}
/*}}}*/
static void do_network(int zoom, int I, int J, int gen, struct percid *p)/*{{{*/
{
  int nlc;
  int ndd;
  struct laccid lc[MAX_LCS];
  struct percid *pp;
  int dd[MAX_DD];
  struct work *work;
  int *lc_total, *dd_total;
  int overall_total;
  int ilc, idd;
  double stat;
  int dof;
  double normalised;

  nlc = 0;
  ndd = 0;
  for (pp=p; pp; pp=pp->next) {
    int matched;
    struct readings *r;
    matched = lookup_lc(lc, nlc, pp);
    if (matched < 0) {
        if (nlc >= MAX_LCS) {
          fprintf(stderr, "Too many CID,LAC pairs in (%dg,%d,%d,%d)\n",
              gen, zoom, I, J);
          exit(2);
        }
      lc[nlc].cid = pp->cid;
      lc[nlc].lac = pp->lac;
      nlc++;
    }
    for (r=pp->r; r; r=r->next) {
      int matched;
      matched = lookup_dd(dd, ndd, r);
      if (matched < 0) {
        if (ndd >= MAX_DD) {
          fprintf(stderr, "Too many timeframes in (%dg,%d,%d,%d)\n",
              gen, zoom, I, J);
          exit(2);
        }
        dd[ndd] = r->timeframe;
        ndd++;
      }
    }
  }

  if (nlc == 0) return;

  /* Allocate */
  work = calloc(ndd * nlc, sizeof(struct work));
  lc_total = calloc(nlc, sizeof(int));
  dd_total = calloc(ndd, sizeof(int));
  overall_total = 0;

#define WORK(LC, DD) work[(LC)*ndd + (DD)]

  for (pp=p; pp; pp=pp->next) {
    ilc = lookup_lc(lc, nlc, pp);
    assert(ilc >= 0);
    struct readings *r;
    for (r=pp->r; r; r=r->next) {
      int asu;
      idd = lookup_dd(dd, ndd, r);
      for (asu=0; asu<32; asu++) {
        int here = r->asus[asu];
        WORK(ilc, idd).total += here;
        lc_total[ilc] += here;
        dd_total[idd] += here;
        overall_total += here;
      }
    }
  }

  /* And now the stats ... */
  stat = 0.0;
  for (ilc=0; ilc<nlc; ilc++) {
    for (idd=0; idd<ndd; idd++) {
      double expected;
      int observed;
      expected = (double) dd_total[idd] * (double) lc_total[ilc] / (double) overall_total;
      observed = WORK(ilc,idd).total;
      if (observed > 0) {
        /* NOTE : then we KNOW that expected > 0 (!) */
        double contrib;
        double obs;
        obs = (double) observed;
        contrib = obs * log(obs/expected);
        stat += contrib;
      }
    }
  }

  dof = (nlc - 1) * (ndd - 1);
  if (dof > 0) {
    /* 2.0*stat is supposed to have approximately a chi-squared distribution
     * with 'dof' degress of freedom.  The mean of such a distribution is dof,
     * and the variance is 2*dof.  Let's normalised to get the number of sigmas
     * out that the statistic is.  Yes, this is all statistically unsound
     * because the distribution is non-symmetric yada yada....  but all we want
     * is some number that's comparable between tiles which is big for tiles
     * with a time-series disparity */
    normalised = (2.0 * stat - (double) dof) / sqrt(2.0 * (double) dof);
    printf("%dg %2d/%6d/%6d %3d %3d %4d %6d %12.5f\n",
        gen, zoom, I, J,
        nlc, ndd, dof,
        overall_total,
        normalised);
  }
  free(work);
  free(lc_total);
  free(dd_total);
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
void goodness(struct node *cursor) /*{{{*/
{
  inner(0, cursor, 0, 0);
}
/*}}}*/

