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

#ifndef TPGEN_H
#define TPGEN_H
struct tpe_lc {/*{{{*/
  struct tpe_lc *next;
  int lac;
  int cid;
  struct panel *panel;
};
/*}}}*/

struct lc_table {
  int n_lcs;
  struct tpe_lc **lcs;
};

struct panel_data {/*{{{*/
  struct panel_data *next;
  int I, J; /* coordinates (m20) */
  int max_asu;
  int n; /* number of points in the linked list after + including this one */
};
/*}}}*/

extern struct lc_table *build_lc_table(struct tower_table *towers);
extern void free_lc_table(struct lc_table *lct);
extern int compare_lc(int lac0, int cid0, int lac1, int cid1);
extern struct panel *lookup_panel(struct lc_table *lct, int lac, int cid);
extern void build_panels(struct lc_table *lct, struct node *top);
extern void get_rough_tp_estimate(const struct tower2 *tow, int *X, int *Y);

struct tp_score {/*{{{*/
  double score;
  double bearing;
  double half_spread;
  int ok;
};

extern struct tp_score tp_score(struct tower2 *tow, int X28, int Y28, int verbose);
extern struct tp_score tp_score_group(const struct group *grp, int X28, int Y28);

/*}}}*/
#endif

