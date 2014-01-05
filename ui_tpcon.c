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
#include "tool.h"
#include "cli.h"

static struct subcommand sub_tpcon = {/*{{{*/
  (OPT_RANGE | OPT_TOWER | OPT_NEAR | OPT_SVGOUT | OPT_ZOOM | OPT_DATA | OPT_CIDS | OPT_BASETILES),
  "tpcon",
  "generate contours of tower position 'likelihood function'"
};
/*}}}*/


void usage_tpcon(void)/*{{{*/
{
  describe(&sub_tpcon);
}
/*}}}*/

#define MAX_LC 6
int ui_tpcon(int argc, char **argv)/*{{{*/
{
  int range = 1;
  int i;
  struct tower_table *table;
  struct node *top;
  struct tower2 *tow[MAX_LC];
  struct group *grp[MAX_LC];
  struct cli *cli;

  cli = parse_args(argc, argv, &sub_tpcon);
  top = load_all_dbs(cli);
  table = read_towers2("towers.dat");

  if (cli->has_range) range = cli->range;

  i = 0;
  if (cli->n_cids > MAX_LC) {
    fprintf(stderr, "Too many CID args\n");
    exit(2);
  }

  for (i=0; i<cli->n_cids; i++) {
    struct laccid *lc = &cli->laccids[i];
    if (cli->has_tower) {
      if (cli->has_near) {
        tow[i] = cid_to_tower_near(table, lc->cid, cli->near_lat, cli->near_lon);
        if (!tow[i]) {
          fprintf(stderr, "CID=%d is unknown in the table, giving up\n", lc->cid);
          exit(2);
        }
      } else {
        tow[i] = cid_to_tower(table, lc->lac, lc->cid);
        if (!tow[i]) {
          fprintf(stderr, "(LAC,CID)=(%d,%d) is unknown in the table, giving up\n",
              lc->lac, lc->cid);
          exit(2);
        }
      }
    } else {
      if (cli->has_near) {
        grp[i] = cid_to_group_near(table, lc->cid, cli->near_lat, cli->near_lon);
        if (!grp[i]) {
          fprintf(stderr, "CID=%d is unknown in the table, giving up\n",
              lc->cid);
          exit(2);
        }
      } else {
        grp[i] = cid_to_group(table, lc->lac, lc->cid);
        if (!grp[i]) {
          fprintf(stderr, "(LAC,CID)=(%d,%d) is unknown in the table, giving up\n",
              lc->lac, lc->cid);
          exit(2);
        }
      }
    }
  }

  temporal_merge_2(top);
  if (cli->has_tower) {
    tpcon_towers(top, table, cli->n_cids, tow, cli->basetiles, cli->svgout, cli->zoom, range);
  } else {
    tpcon_groups(top, table, cli->n_cids, grp, cli->basetiles, cli->svgout, cli->zoom, range);
  }

  free_tower_table(table);
  free(top);

  return 0;
}
/*}}}*/


