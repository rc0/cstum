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

#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "tool.h"

/* 36 periods per annum, 5 years */
#define MAX_AGO (36*5)

struct dd {
  int year;
  int month;
  int d10;
};

static int compute_ago(int timeframe, const struct dd *now_dd)
{
  int ago;
  int dd, m, y;
  y = timeframe / 1000;
  m = ((timeframe % 1000) / 10) - 1;
  dd = timeframe % 10;
  ago = (now_dd->year - y) * 36 + (now_dd->month - m) * 3 + (now_dd->d10 - dd);
  if (ago < 0) {
    fprintf(stderr, "ago should not be < 0\n");
    exit(2);
  }
  return ago;
}

static void make_weights(int upto, const int *tally, int *weights)
{
  int i;
  int count;
  int scaling = 1024;
  for (i=0, count=0; i<=upto; i++) {
    weights[i] = scaling;
    count += tally[i];
    count += 2; /* Extra per time period */
    while ((scaling > 1) && (count >= 16)) {
      scaling = ((scaling >> 1) + (scaling >> 2)) + 1;
      count -= 16;
    }
  }
}

static void aggregate_readings(const struct dd *now_dd, struct data *node, int gen)/*{{{*/
{
  int tally[MAX_AGO];
  int weights[MAX_AGO];
  int upto;
  struct percid *p;
  struct percid **head;

  memset(tally, 0, sizeof(tally));
  upto = 0;
  /* Count total number of samples in each historical period */

  head = pergen(node, gen);
  for (p=*head; p; p=p->next) {
    struct readings *r;
    for (r=p->r; r; r=r->next) {
      if (r->timeframe > 0) {
        int ago = compute_ago(r->timeframe, now_dd);
        if (ago >= MAX_AGO) {
          fprintf(stderr, "Some data is too old (%d periods, max %d periods)\n",
              ago, MAX_AGO);
          exit(2);
        }
        if (upto < ago) {
          upto = ago;
        }
        ++tally[ago];
      } else {
        fprintf(stderr, "There should not be any zero timeframes at this stage??\n");
        exit(2);
      }
    }
  }
  make_weights(upto, tally, weights);

  for (p=*head; p; p=p->next) {
    struct readings *r;
    struct readings *sr;
    sr = lookup_readings(p, 0); /* Aggregate into '0' (universal) timeframe */
    for (r=p->r; r; r=r->next) {
      int i;
      int ago;
      int scaling;
      if (r->timeframe > 0) {
        /* Avoid already aggregated timeframes */
        ago = compute_ago(r->timeframe, now_dd);
        if (r->timeframe > sr->newest) {
          sr->newest = r->timeframe;
        }
        scaling = weights[ago];
        for (i=0; i<32; i++) {
          sr->asus[i] += scaling * r->asus[i];
        }
      }
    }
  }
}
/*}}}*/
static void inner_merge(const struct dd *now_dd, int level, struct node *cursor) {/*{{{*/
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_merge(now_dd, level+1, cursor->c[i][j]);
      }
    }
  }
  if (level >= 10) {
    /* Could do this right up to level 0, but then every CID and every
     * timeframe would be present in the dataset. The searches during the build
     * would cost something. */
    if (!cursor->d) {
      cursor->d = malloc(sizeof(struct data));
      cursor->d->cids2 = NULL;
      cursor->d->cids3 = NULL;
    }
    aggregate_readings(now_dd, cursor->d, 2);
    aggregate_readings(now_dd, cursor->d, 3);
  }
}
/*}}}*/
void temporal_merge_2(struct node *cursor) /*{{{*/
{
  time_t now;
  struct tm now_tm;
  struct dd now_dd;
  int dd;

  now = time(NULL);
  now_tm = *gmtime(&now);
  dd = now_tm.tm_mday / 10;
  if (dd > 2) dd = 2;
  now_dd.d10 = dd;
  now_dd.month = now_tm.tm_mon;
  now_dd.year = now_tm.tm_year - 100;
  inner_merge(&now_dd, 0, cursor);
}
/*}}}*/

struct interval
{
  int has_since;
  int since_dd;
  int has_until;
  int until_dd;
};

static struct readings *filter_readings(struct readings *rr, const struct interval *ival)
{
  struct readings *result = NULL;
  struct readings *r, *nr;
  for (r = rr; r; r = nr) {
    int drop;
    nr = r->next;
    drop = 0;
    if (ival->has_since && (r->timeframe < ival->since_dd)) drop = 1;
    if (ival->has_until && (r->timeframe > ival->until_dd)) drop = 1;
    if (!drop) {
      r->next = result;
      result = r;
    } else {
      free(r);
    }
  }
  return result;
}

static struct percid *filter_percid(struct percid *pc, const struct interval *ival)
{
  struct percid *result = NULL;
  struct percid *p, *np;
  for (p = pc; p; p = np) {
    struct readings *rr;
    np = p->next;
    rr = filter_readings(p->r, ival);
    if (rr) {
      p->r = rr;
      p->next = result;
      result = p;
    } else {
      free(p);
    }
  }
  return result;
}

static void inner_filter(int level, struct node *cursor, const struct interval *ival)
{
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_filter(level+1, cursor->c[i][j], ival);
      }
    }
  }
  if (cursor->d) {
    cursor->d->cids2 = filter_percid(cursor->d->cids2, ival);
    cursor->d->cids3 = filter_percid(cursor->d->cids3, ival);
  }
}

void temporal_interval_filter(struct node *top, int has_since, int since_dd, int has_until, int until_dd)/*{{{*/
{
  struct interval ival;
  ival.has_since = has_since;
  ival.since_dd = since_dd;
  ival.has_until = has_until;
  ival.until_dd = until_dd;
  fprintf(stderr, "Filtering %d,%d,%d,%d\n", has_since, since_dd, has_until, until_dd);
  inner_filter(0, top, &ival);
}
/*}}}*/
/* vim:et:sts=2:sw=2:ht=2
 * */
