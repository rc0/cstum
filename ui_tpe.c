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

static struct subcommand sub_tpe = {/*{{{*/
  (OPT_NEAR | OPT_DATA | OPT_CIDS),
  "tpe",
  "estimate tower positions"
};
/*}}}*/

void usage_tpe(void)/*{{{*/
{
  describe(&sub_tpe);
}
/*}}}*/

int ui_tpe(int argc, char **argv)/*{{{*/
{
  struct tower_table *table;
  struct tower2 *tow;
  struct node *top;
  struct cli *cli;
  int i;

  cli = parse_args(argc, argv, &sub_tpe);
  if (cli->n_datafiles == 0) {
    usage_tpe();
    exit(2);
  }
  top = load_all_dbs(cli);
  table = read_towers2("towers.dat");
  resolve_towers(cli, table);

  /* Also make active any towers with no existing estimate, or no hexcode.  The
   * latter allows you to get a tower re-run by deleting the hexcode line from
   * the file, whilst still getting a report of the distance that the estimate
   * has moved by. */
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    if (!tow->has_estimated || !tow->hexcode) {
      tow->active_flag = 1;
    }
  }

  /* Do filtering */
  temporal_merge_2(top);
  tpe(top, table);
  write_towers2("new_towers.dat", table);
  free_tower_table(table);

  free(top);

  return 0;
}
/*}}}*/

/* vim:et:sts=2:sw=2:ht=2
 * */
