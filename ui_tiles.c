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

static struct subcommand sub_tiles = {/*{{{*/
  (OPT_ZOOM | OPT_DATA | OPT_NEAR | OPT_CIDS),
  "tiles",
  "create tile generator recipes\n"
  "        (with CIDs, limit to bounding box)"
};
/*}}}*/


void usage_tiles(void)
{
  describe(&sub_tiles);
}

int ui_tiles(int argc, char **argv)
{
  struct node *top;
  struct cli *cli;
  struct tower_table *table;

  cli = parse_args(argc, argv, &sub_tiles);
  top = load_all_dbs(cli);
  table = read_towers2("towers.dat");
  resolve_towers(cli, table);

  /* Perhaps here, do filtering based on dead towers or something like that.
   * For the future */

  /* Temporal filtering : based on 'since' ? 
   * Cheaper to do this before the spatial merge */

  /* Do spatial merge (preserving timeframes) */
  spatial_merge_2(top);

  /* Temporal merging : collapse remaining timeframes into a single combo
   * timeframe */
  temporal_merge_2(top);

  /* Now walk the tree of what we're left with */
  tile_walk(cli->zoom, table, top, stdout);

  return 0;
}

/* vim:et:sts=2:sw=2:ht=2
 * */
