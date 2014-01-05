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
#include "tool.h"
#include "cli.h"

#define CHECK(FLAG) ((sub->options & OPT_##FLAG) != 0)
#define GAP "         "

void describe(const struct subcommand *sub)/*{{{*/
{
  fprintf(stderr, "%s : %s\n", sub->name, sub->description);
  if (CHECK(PRUNE))     fprintf(stderr, GAP " [--prune <file>] : use <file> to define time ranges to dump\n");
  if (CHECK(RANGE))     fprintf(stderr, GAP " [--range <ntiles>] : draw +/- <ntiles> around first listed tower\n");
  if (CHECK(NEAR))      fprintf(stderr, GAP " [--near lat,lon] : localise CID arguments without LACs\n");
  if (CHECK(ALL))       fprintf(stderr, GAP " [--all]          : optimise for all towers\n");
  if (CHECK(VERBOSE))   fprintf(stderr, GAP " [--verbose|-v]   : show verbose output (to stdout)\n");
  if (CHECK(UNTIL))     fprintf(stderr, GAP " [--until <yymmd>] : exclude readings after this date\n");
  if (CHECK(SINCE))     fprintf(stderr, GAP " [--since <yymmd>] : exclude readings before this date\n");
  if (CHECK(BASETILES)) fprintf(stderr, GAP " [--base|-B <dirpath>] : path to base map tile directory\n");
  if (CHECK(OUTPUT))    fprintf(stderr, GAP " [--out|-o <filename>] : path to write the output to\n");
  if (CHECK(SVGOUT))    fprintf(stderr, GAP "   <out.svg>  : path to the output file\n");
  if (CHECK(ZOOM))      fprintf(stderr, GAP "   <zoom>  : zoom level\n");
  if (CHECK(TEMPLATE))  fprintf(stderr, GAP "   <template> : printf template for filenames (%%d for zoom)\n");
  if (CHECK(ZOOMLOHI))  fprintf(stderr, GAP "   <zoomlo>  : low zoom level\n");
  if (CHECK(ZOOMLOHI))  fprintf(stderr, GAP "   <zoomhi>  : high zoom level\n");
  if (CHECK(DATA))      fprintf(stderr, GAP "   <in1> [<in2>...]  (data files)\n");
  if (CHECK(CIDS))      fprintf(stderr, GAP "   -- [<lac1>/]<cid1> [[<lac2>/]<cid2>...]  (CIDs)\n");
}
/*}}}*/
static void bail(const struct subcommand *sub)/*{{{*/
{
  describe(sub);
  exit(1);
}
/*}}}*/
static int bogus_dd(int dd)/*{{{*/
{
  int m;
  if (dd < 11010) return 1;
  if (dd > 13122) return 1;
  if ((dd % 10) > 2) return 1;
  m = (dd / 10) % 100;
  if ((m < 1) || (m > 12)) return 1;
  return 0;
}
/*}}}*/
struct cli *parse_args(int argc, char **argv, const struct subcommand *sub)/*{{{*/
{
  struct cli *result;
  int i;
  result = calloc(1, sizeof(struct cli));

  i = 0;
  while ((i < argc) && (argv[i][0] == '-')) {
    if ((sub->options & OPT_PRUNE) && !strcmp(argv[i], "--prune")) {
      i++;
      if (!argv[i]) bail(sub);
      result->prune_file = argv[i];
      i++;
    } else if ((sub->options & OPT_OUTPUT) && 
        (!strcmp(argv[i], "--out") ||
         !strcmp(argv[i], "-o"))) {
      i++;
      if (!argv[i]) bail(sub);
      result->output = argv[i];
      i++;
    } else if ((sub->options & OPT_BASETILES) && 
        (!strcmp(argv[i], "--base") ||
         !strcmp(argv[i], "-B"))) {
      i++;
      if (!argv[i]) bail(sub);
      result->basetiles = argv[i];
      i++;
    } else if ((sub->options & OPT_NEAR) && !strcmp(argv[i], "--near")) {
      i++;
      if (!argv[i]) bail(sub);
      if (sscanf(argv[i], "%lf,%lf", &result->near_lat, &result->near_lon) != 2) {
        fprintf(stderr, "Misparsed --near argument\n");
        bail(sub);
      }
      result->has_near = 1;
      i++;
    } else if ((sub->options & OPT_RANGE) && !strcmp(argv[i], "--range")) {
      i++;
      if (!argv[i]) bail(sub);
      if (sscanf(argv[i], "%d", &result->range) != 1) {
        fprintf(stderr, "Misparsed --range argument\n");
        bail(sub);
      }
      result->has_range = 1;
      i++;
    } else if ((sub->options & OPT_UNTIL) && !strcmp(argv[i], "--until")) {
      i++;
      if (!argv[i]) bail(sub);
      if (sscanf(argv[i], "%d", &result->until_dd) != 1) {
        fprintf(stderr, "Misparsed --until argument\n");
        bail(sub);
      }
      if (bogus_dd(result->until_dd)) {
        fprintf(stderr, "--until date %d is bogus\n", result->until_dd);
      }
      result->has_until = 1;
      i++;
    } else if ((sub->options & OPT_SINCE) && !strcmp(argv[i], "--since")) {
      i++;
      if (!argv[i]) bail(sub);
      if (sscanf(argv[i], "%d", &result->since_dd) != 1) {
        fprintf(stderr, "Misparsed --since argument\n");
        bail(sub);
      }
      if (bogus_dd(result->since_dd)) {
        fprintf(stderr, "--since date %d is bogus\n", result->since_dd);
      }
      result->has_since = 1;
      i++;
    } else if ((sub->options & OPT_ALL) && !strcmp(argv[i], "--all")) {
      result->has_all = 1;
      i++;
    } else if ((sub->options & OPT_TOWER) && !strcmp(argv[i], "--tower")) {
      result->has_tower = 1;
      i++;
    } else if ((sub->options & OPT_VERBOSE) &&
                (!strcmp(argv[i], "--verbose") ||
                 !strcmp(argv[i], "-v"))) {
      result->has_verbose = 1;
      i++;
    } else {
      fprintf(stderr, "Unparsed option %s\n", argv[i]);
      bail(sub);
    }
  }

  if (sub->options & OPT_SVGOUT) {
    if (!argv[i]) bail(sub);
    result->svgout = argv[i++];
  }

  if (sub->options & OPT_ZOOM) {
    if (!argv[i]) bail(sub);
    result->zoom = atoi(argv[i++]);
  }

  if (sub->options & OPT_TEMPLATE) {
    if (!argv[i]) bail(sub);
    result->template = argv[i++];
  }

  if (sub->options & OPT_ZOOMLOHI) {
    if (!argv[i]) bail(sub);
    result->zoomlo = atoi(argv[i++]);
    if (!argv[i]) bail(sub);
    result->zoomhi = atoi(argv[i++]);
  }

  if (sub->options & OPT_DATA) {
    while (i < argc) {
      if (!strcmp(argv[i], "--")) {
        i++;
        break;
      }
      if (result->n_datafiles == MAX_DATA_FILES) {
        fprintf(stderr, "Too many data files\n");
        exit(2);
      }
      result->datafiles[result->n_datafiles++] = argv[i++];
    }
  }

  if (sub->options & OPT_CIDS) {
    while (i < argc) {
      if (result->n_cids == MAX_CIDS) {
        fprintf(stderr, "Too many CIDs given\n");
        exit(2);
      }
      if (result->has_near) {
        if (sscanf(argv[i], "%d",
              &result->laccids[result->n_cids].cid) == 1) {
          i++;
          result->n_cids++;
        } else {
          fprintf(stderr, "Misparsed cid = %s\n", argv[i]);
          bail(sub);
        }
      } else {
        if (sscanf(argv[i], "%d/%d",
              &result->laccids[result->n_cids].lac,
              &result->laccids[result->n_cids].cid) == 2) {
          i++;
          result->n_cids++;
        } else {
          fprintf(stderr, "Misparsed lac/cid = %s\n", argv[i]);
          bail(sub);
        }
      }
    }
  }

  if (i < argc) {
    fprintf(stderr, "There were unprocessed arguments\n");
    bail(sub);
  }

  return result;
}
/*}}}*/
struct node *load_all_dbs(const struct cli *cli)/*{{{*/
{
  struct node *result;
  int i;
  result = new_node();
  for (i=0; i<cli->n_datafiles; i++) {
    read_db(cli->datafiles[i], result, NULL);
  }
  return result;
}
/*}}}*/
/* vim:et:sts=2:sw=2:ht=2
 * */

