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

/* Resolve the lac/cid combinations on the command line (or just the CIDs if
 * --near was given) and mark their towers as active for processing on this
 * run. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include "tool.h"
#include "cli.h"

void resolve_towers(struct cli *cli, struct tower_table *table)
{
  int i;
  for (i=0; i<cli->n_cids; i++) {
    struct laccid *lc = &cli->laccids[i];
    struct tower2 *tow;
    if (cli->has_near) {
      tow = cid_to_tower_near(table, lc->cid, cli->near_lat, cli->near_lon);
      if (tow) {
        tow->active_flag = 1;
      } else {
        fprintf(stderr, "CID=%d is unknown in the table, giving up\n", lc->cid);
        exit(2);
      }
    } else {
      tow = cid_to_tower(table, lc->lac, lc->cid);
      if (tow) {
        tow->active_flag = 1;
      } else {
        fprintf(stderr, "(LAC,CID)=(%d,%d) is unknown in the table, giving up\n",
            lc->lac, lc->cid);
        exit(2);
      }
    }
  }
}

/* vim:et:sts=2:sw=2:ht=2
 * */

