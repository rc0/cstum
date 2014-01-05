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

static struct subcommand sub_filter = {/*{{{*/
  (OPT_DATA | OPT_CIDS),
  "filter",
  "select readings from a set of CIDs"
};
/*}}}*/


void usage_filter(void)/*{{{*/
{
  describe(&sub_filter);
}
/*}}}*/

int ui_filter(int argc, char **argv)/*{{{*/
{
  int i;
  int *cids;
  struct cli *cli;
  struct node *top;

  cli = parse_args(argc, argv, &sub_filter);
  top = load_all_dbs(cli);

  if (cli->n_cids < 1) {
    fprintf(stderr, "No cids?\n");
    return 2;
  }

  cids = malloc(MAX_CIDS * sizeof(int));
  for(i=0; i<cli->n_cids; i++) {
    /* Poor : no account taken of LAC */
    cids[i] = cli->laccids[i].cid;
  }

  /* Do filtering */
  cid_filter(top, cli->n_cids, cids);

  write_db(stdout, top);

  free(top);

  return 0;
}
/*}}}*/

