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

/* Discover new LACs and update tower table */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"

static void add_lac_to_tower(struct tower2 *tow, int gen, int lac, struct tower_table *table)
{
  int *n;
  int *lacs;
  if (gen == 2){
    n = &tow->n_lacs_g;
    lacs = tow->lacs_g;
  } else if (gen == 3) {
    n = &tow->n_lacs_u;
    lacs = tow->lacs_u;
  } else {
    fprintf(stderr, "gen is not 2 or 3 in add_lac_cid\n");
    exit(2);
  }

  if (*n >= MAX_LACS) {
    fprintf(stderr, "Tower %s (%s,%s) is out of gen-%d LAC space\n",
        tow->description,
        tow->label_2g ?: "-",
        tow->label_3g ?: "-",
        gen);
    exit(2);
  }

  lacs[*n] = lac;
  ++*n;
  fprintf(stderr, "Tower %s (%s,%s) : adding LAC %d\n",
      tow->description,
      tow->label_2g ?: "-",
      tow->label_3g ?: "-",
      lac);
  fflush(stderr);

  insert_extra_lac_into_table(table, tow, gen, lac);
}

static void handle_percid(struct percid *p0, int gen, struct tower_table *table)
{
  struct percid *p;
  for (p = p0; p; p = p->next) {
    struct tower2 *tow;
    tow = cid_to_tower(table, p->lac, p->cid);
    if (!tow) {
      /* Try to find matching CIDs *AT THIS LOCATION* that do have a matching
       * tower.  So this only works if the new data overlaps previously-visited
       * locations.  Searching the locality would be more work... */
      struct percid *pp;
      for (pp = p0; pp; pp = pp->next) {
        if (pp->cid == p->cid) {
          tow = cid_to_tower(table, pp->lac, pp->cid);
          if (tow) {
            /* Matched.  Add a new CID/LAC entry to the tower */
            add_lac_to_tower(tow, gen, p->lac, table);
            break;
          }
        }
      }
    }
  }
}

static void inner_walk(int level, struct node *cursor, int I, int J, struct tower_table *table)/*{{{*/
{
  int i, j;
  for (i=0; i<2; i++) {
    for (j=0; j<2; j++) {
      if (cursor->c[i][j]) {
        inner_walk(level+1, cursor->c[i][j], (I<<1)+i, (J<<1)+j, table);
      }
    }
  }
  if (cursor->d) {
    if (cursor->d->cids2) {
      handle_percid(cursor->d->cids2, 2, table);
    }
    if (cursor->d->cids3) {
      handle_percid(cursor->d->cids3, 3, table);
    }
  }
}
/*}}}*/
void addlacs(struct node *cursor, struct tower_table *table)/*{{{*/
{
  inner_walk(0, cursor, 0, 0, table);
}
/*}}}*/
/* vim:et:sts=2:sw=2:ht=2
 * */
