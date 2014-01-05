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

static struct subcommand sub_tcs = {
  (OPT_DATA | OPT_ALL | OPT_VERBOSE),
  "tcs",
  "select tower colours"
};


void usage_tcs(void)/*{{{*/
{
  describe(&sub_tcs);
}
/*}}}*/

int ui_tcs(int argc, char **argv)/*{{{*/
{
  int i;
  struct tower_table *table;
  struct node *top;
  struct cli *cli;

  cli = parse_args(argc, argv, &sub_tcs);

  top = load_all_dbs(cli);
  spatial_merge_2(top);
  temporal_merge_2(top);
  table = read_towers2("towers.dat");
  /* Set activity flags here so that we can do an --all option later. */
  for (i = 0; i < table->n_towers; i++) {
    if (table->towers[i]->colour_index < 0) {
      table->towers[i]->active_flag = 1;
    } else {
      table->towers[i]->active_flag = 0;
    }
  }

  tcs(top, table, cli->has_all, cli->has_verbose);
  write_towers2("new_towers.dat", table);
  free_tower_table(table);

  return 0;
}
/*}}}*/
