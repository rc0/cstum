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

/* Summarise the results in a node into what we'll display on the map */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "tool.h"

struct odata_1 {/*{{{*/
  int lac;
  int cid;
  int n_readings;
  int median;
};
/*}}}*/
static int compute_median(int x[32], int nr) {/*{{{*/
  int i;
  int nr2;
  int f;
  nr2 = (nr+1)>>1;
  f = 0;
  for (i=0; i<32; i++) {
    f += x[i];
    if (f >= nr2) {
      return i;
    }
  }
  fprintf(stderr, "I cannot get here in compute_median!\n");
  exit(2);
}
/*}}}*/
static int compare_odata_1(const void *aa, const void *bb) {/*{{{*/
  const struct odata_1 *a = aa;
  const struct odata_1 *b = bb;
  if      (a->n_readings < b->n_readings) return 1;
  else if (a->n_readings > b->n_readings) return -1;
  else if (a->median < b->median) return 1;
  else if (a->median > b->median) return -1;
  else if (a->cid < b->cid) return -1;
  else if (a->cid > b->cid) return 1;
  else if (a->lac < b->lac) return -1;
  else if (a->lac > b->lac) return 1;
  else return 0;
}
/*}}}*/
static int ignore(int cid)
{
#if 1
  if (cid == 50594049) return 1;
#endif
  return 0;
}

struct odata *compute_odata(struct data *d, int gen)/*{{{*/
{
  int n_lcs;
  struct percid *p;
  struct odata_1 *od1;
  struct odata *result;
  double readings_scale;
  int i;
  int newest_timeframe = 0;
  struct percid **head = pergen(d, gen);
  int asus[32], tnr;
  int kept;
  int n_ok;
  for (p=*head, n_lcs=0; p; p=p->next) {
    n_lcs++;
  }

  memset(asus, 0, sizeof(asus));
  tnr = 0;

  if (n_lcs > 0) {
    od1 = malloc(n_lcs * sizeof(struct odata_1));
    for (p=*head, i=0; p; p=p->next) {
      struct readings *r;
      int j, nr;

      od1[i].lac = p->lac;
      od1[i].cid = p->cid;
      r = lookup_readings(p, 0);
      if (r->newest > newest_timeframe) {
        newest_timeframe = r->newest;
      }
      if (!r) {
        fprintf(stderr, "r cannot be null!\n");
        exit(2);
      }
      for (j=0, nr=0; j<32; j++) {
        nr += r->asus[j];
        asus[j] += r->asus[j];
        tnr += r->asus[j];
      }
      od1[i].n_readings = nr;
      od1[i].median = compute_median(r->asus, nr);
      i++;
    }

    /* now sort */
    qsort(od1, n_lcs, sizeof(struct odata_1), compare_odata_1);

    result = malloc(sizeof(struct odata));
    result->asu = compute_median(asus, tnr);

    /* This is OK even if [0] is an 'ignore' CID */
    readings_scale = 65535.0 / (double) od1[0].n_readings;

    for (i=n_ok=0; i<n_lcs; i++) {
      if (!ignore(od1[i].cid)) n_ok++;
    }

    if (n_ok == 0) {
      result->lcs = malloc(1 * sizeof(struct laccid));
      result->rel = malloc(1 * sizeof(unsigned short));
      result->lcs[0].lac = od1[0].lac;
      result->lcs[0].cid = od1[0].cid;
      result->rel[0] = (int)((double) od1[0].n_readings * readings_scale);
      result->n_lcs = 1;
    } else {
      result->lcs = malloc(n_ok * sizeof(struct laccid));
      result->rel = malloc(n_ok * sizeof(unsigned short));
      kept = 0;
      for (i=0; i<n_lcs; i++) {
        if (ignore(od1[i].cid)) continue;
        result->lcs[kept].lac = od1[i].lac;
        result->lcs[kept].cid = od1[i].cid;
        result->rel[kept] = (int)((double) od1[i].n_readings * readings_scale);
        kept++;
      }
      result->n_lcs = kept;
    }
    result->newest_timeframe = newest_timeframe;
    free(od1);
  } else {
    result = NULL;
  }
  return result;
}
/*}}}*/
void free_odata(struct odata *x)/*{{{*/
{
  if (x->lcs) free(x->lcs);
  if (x->rel) free(x->rel);
  free(x);
}
/*}}}*/
