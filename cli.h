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

#ifndef _CLI_H
#define _CLI_H

struct subcommand/*{{{*/
{
  unsigned int options;
  char *name;
  char *description;
};
/*}}}*/

#define MAX_DATA_FILES 256
#define MAX_CIDS 256

struct cli/*{{{*/
{
  char *prune_file;
  int has_all;
  int has_verbose;
  int has_tower;
  int has_near;
  double near_lon;
  double near_lat;
  int has_range;
  int range;
  int zoom;
  char *template;
  int zoomlo;
  int zoomhi;
  char *output;
  char *svgout;
  int has_until;
  int until_dd;
  int has_since;
  int since_dd;
  char *basetiles;

  int n_datafiles;
  char *datafiles[MAX_DATA_FILES];

  int n_cids;
  struct laccid laccids[MAX_CIDS];

};
/*}}}*/

#define OPT_DATA      (1<<0)
#define OPT_CIDS      (1<<1)
#define OPT_ALL       (1<<2)
#define OPT_VERBOSE   (1<<3)
#define OPT_NEAR      (1<<4)
#define OPT_PRUNE     (1<<5)
#define OPT_TOWER     (1<<6)
#define OPT_RANGE     (1<<7)
#define OPT_SVGOUT    (1<<8)
#define OPT_TEMPLATE  (1<<9)
#define OPT_ZOOM      (1<<10)
#define OPT_ZOOMLOHI  (1<<11)
#define OPT_UNTIL     (1<<12)
#define OPT_SINCE     (1<<13)
#define OPT_OUTPUT    (1<<14)
#define OPT_BASETILES (1<<15)

extern struct cli *parse_args(int argc, char **argv, const struct subcommand *sub);
extern void describe(const struct subcommand *sub);
extern struct node *load_all_dbs(const struct cli *cli);

#endif /* _CLI_H */

/* vim:et:sts=2:sw=2:ht=2
 * */
