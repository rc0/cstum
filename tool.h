/************************************************************************
 * Copyright (c) 2012, 2013, 2014 Richard P. Curnow
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

#ifndef TOOL_H
#define TOOL_H

#include <stdio.h>
#include <stdint.h>

/* CELL READINGS */
struct readings {/*{{{*/
  struct readings *next;
  int timeframe;
  int newest; /* timeframe of newest data in the combo, if timeframe=0 */
  int asus[32];
};
/*}}}*/
struct percid {/*{{{*/
  struct percid *next;
  int lac;
  int cid;
  struct readings *r;
};
/*}}}*/
struct laccid/*{{{*/
{
  int lac;
  int cid;
};
/*}}}*/
/* Most-frequent + also-ran results */
struct odata {/*{{{*/
  int asu;
  int n_lcs;
  struct laccid *lcs;
  /* Assuming the 'primary' cid is weighted 1<<16,
   * this array is the relative weights of the other CIDs */
  unsigned short *rel;
  int newest_timeframe;
};
/*}}}*/
struct data {/*{{{*/
  struct percid *cids2;
  struct percid *cids3;

  /* The towers that are shown in this tile for 2G and 3G : used in tower
   * colour select */
  struct tower2 *t2;
  struct tower2 *t3;
};
/*}}}*/

struct scoring {
  int n_active_subtiles;
  /* Highest value amongst descendents */
  int n_highest_act_subs;
};

struct node {/*{{{*/
  struct node *c[2][2];
  struct data *d;

  /* These are used for manipulating the tree later, to make traversal easier */
  struct node *s; /* 'string' */
  /* coordinates */
  int X, Y;

#if 0
  /* STUFF TO DO WITH AUTO_LIMIT HANDLING */
  /* Scores for just the box itself */
  int my_score2, my_score3;

  /* Scores for entire tile, i.e. the sum of the scores for the 256 (16x16) little boxes inside it */
  int tile_score2, tile_score3;

  /* The highest tile_score across this tile and all its descendents.  This
   * will be used to achieve the property that if we render a tile, we also
   * render all zoom-out levels above it. */
  int max_score2, max_score3;
#endif

  struct scoring s2, s3;
};
/*}}}*/

/* TOWERS */

#if 0
struct tower/*{{{*/
{
  int colour_index;
  int pos_known;
  int X28;
  int Y28;
  char *label_2g;
  char *label_3g;
};
/*}}}*/
#endif

struct M28 {/*{{{*/
  int X;
  int Y;
};
/*}}}*/
struct cell {/*{{{*/
  /* A CID with implied LAC, associated with one panel (hence one group hence one tower) */
  struct cell *next;
  struct panel *panel;
  enum {CELL_G, CELL_U} gen;
  int cid;
};
/*}}}*/
struct panel {/*{{{*/
  struct panel *next;
  struct group *group;
  struct cell *cells;
  double bearing;
  struct panel_data *data; /* for algorithm data */
};
/*}}}*/
struct group/*{{{*/
{
  struct group *next;
  struct tower2 *tower;
  int no_order;
  int has_bearing;
  double bearing;
  double half_spread;
  struct panel *panels;
};
/*}}}*/
struct comment/*{{{*/
{
  struct comment *next;
  char *text;
};
/*}}}*/

#define MAX_LACS 6
#define MAX_CIDS_PER_LAC 12

#if 0
struct lac {/*{{{*/
  int lac;
  int n_cids;
  int cids[MAX_CIDS_PER_LAC];
};
/*}}}*/
#endif
struct tower2 /*{{{*/
{
  struct tower2 *next;
  char *description;
  struct comment *comments;
  int colour_index;
  int has_observed;
  struct M28 observed;
  int has_estimated;
  struct M28 estimated;
  int n_lacs_g;
  int lacs_g[MAX_LACS];
  int n_lacs_u;
  int lacs_u[MAX_LACS];
  char *hexcode;
  char *label_2g;
  char *label_3g;
  struct group *groups;
  /* For tracking towers to be worked on according to command line arguments */
  int active_flag;
  /* This tower's position in the tower table */
  int index;
  /* Count number of subtiles displayed as this tower */
  int display_count;
};
/*}}}*/
struct tower_table/*{{{*/
{
  int n_towers;
  struct tower2 **towers;
  /* LAC,CID -> GROUP mapping */
  struct lc_table *lcs;
};
/*}}}*/
struct prune_data {/*{{{*/
  struct prune_data *next;
  int lac;
  int cid;
  int upto; /* Most recent timeframe to drop */
  int count;
};
/*}}}*/

struct cli;

/* PROTOS */

extern void read_db(const char *dbname, struct node *top, struct prune_data *prunings);
extern struct prune_data *load_prune_data(const char *filename);
extern void report_pruning(const struct prune_data *p);
extern int is_blank(const char *x);
extern void insert_asus(struct data *cursor, int gen, int lac, int cid, int timeframe, int *asus);
extern struct percid *lookup_cell(struct data *d, int gen, int lac, int cid, int create);
extern struct node *new_node(void);
extern struct data *lookup_spatial(struct node *parent, int xi, int yi);
extern struct readings *lookup_readings(struct percid *c, int timeframe);
extern struct node *lookup_node(struct node *top, int level, int ii, int jj);
extern struct odata *compute_odata(struct data *d, int gen);
extern void free_odata(struct odata *x);
extern void spatial_merge(struct node *cursor);
extern void temporal_filter(struct node *top);
extern void write_db(FILE *out, struct node *top);
extern void spatial_merge_2(struct node *cursor);
extern void temporal_merge_2(struct node *cursor);
extern void temporal_interval_filter(struct node *top, int has_since, int since_dd, int has_until, int until_dd);
extern void tile_walk(int zoom, struct tower_table *table, struct node *cursor, FILE *out);
extern void overlay(int zoomlo, int zoomhi, struct tower_table *table, struct node *cursor, FILE *out);
extern void correlate(struct node *top);

struct tower_table *read_towers2(const char *filename);
#if 0
struct tower *lookup_cid(int cid);
#endif
void cid_filter(struct node *top, int n_cids, int *cids);

/* towers2 */
extern struct tower_table *read_towers2(const char *filename);
extern struct cell *cid_to_cell(struct tower_table *table, int lac, int cid);
extern struct cell *cid_to_cell_near(struct tower_table *table, int cid, double lat, double lon);
extern struct panel *cid_to_panel(struct tower_table *table, int lac, int cid);
extern struct panel *cid_to_panel_near(struct tower_table *table, int cid, double lat, double lon);
extern struct group *cid_to_group(struct tower_table *table, int lac, int cid);
extern struct group *cid_to_group_near(struct tower_table *table, int cid, double lat, double lon);
extern struct tower2 *cid_to_tower(struct tower_table *table, int lac, int cid);
extern struct tower2 *cid_to_tower_near(struct tower_table *table, int cid, double lat, double lon);
extern void insert_extra_lac_into_table(struct tower_table *table, struct tower2 *tow, int gen, int lac);
extern void write_towers2(const char *filename, struct tower_table *table);
extern void free_tower_table(struct tower_table *table);
extern void m28_to_wgs84(const struct M28 *m28, double *lat, double *lon);
extern void wgs84_to_m28(double lat, double lon, int *X, int *Y);
extern char *m28_to_osgb(const struct M28 *m28);
extern char *m28_to_osgb_nospace(const struct M28 *m28);
extern char *compute_hexcode(int X, int Y);
extern void resolve_towers(struct cli *cli, struct tower_table *table);
extern int32_t atan2i_approx2(int32_t y, int32_t x);

extern void tpe(struct node *top, struct tower_table *table);
extern void tpr(struct node *top, struct tower_table *table);
extern void tcs(struct node *top, struct tower_table *table, int all, int verbose);
extern void tpcon_towers(struct node *top, struct tower_table *table,
    int n_towers, struct tower2 **tow,
    const char *basetiles,
    const char *svg_name, int zoom, int range);
extern void tpcon_groups(struct node *top, struct tower_table *table,
    int n_groups, struct group **grp,
    const char *basetiles,
    const char *svg_name, int zoom, int range);

extern void goodness(struct node *cursor);
extern void timecull(struct node *cursor);
extern void cidlist(struct node *cursor);
extern void addlacs(struct node *cursor, struct tower_table *table);

extern int ui_cat(int argc, char **argv);
extern void usage_cat(void);
extern int ui_grid(int argc, char **argv);
extern void usage_grid(void);
extern int ui_tiles(int argc, char **argv);
extern void usage_tiles(void);
extern int ui_tilerange(int argc, char **argv);
extern void usage_tilerange(void);
extern int ui_filter(int argc, char **argv);
extern void usage_filter(void);
extern int ui_cover(int argc, char **argv);
extern void usage_cover(void);
extern int ui_tpe(int argc, char **argv);
extern void usage_tpr(void);
extern int ui_tpr(int argc, char **argv);
extern void usage_tpe(void);
extern int ui_tcs(int argc, char **argv);
extern void usage_tcs(void);
extern int ui_tpcon(int argc, char **argv);
extern void usage_tpcon(void);
extern int ui_goodness(int argc, char **argv);
extern void usage_goodness(void);
extern int ui_timecull(int argc, char **argv);
extern void usage_timecull(void);
extern int ui_cidlist(int argc, char **argv);
extern void usage_cidlist(void);
extern int ui_overlay(int argc, char **argv);
extern void usage_overlay(void);
extern int ui_addlacs(int argc, char **argv);
extern void usage_addlacs(void);

/* INLINE */

static inline struct percid **pergen(struct data *d, int gen) /*{{{*/
{
  if (gen == 3) {
    return &d->cids3;
  } else {
    return &d->cids2;
  }
}
/*}}}*/
#endif /* TOOL_H */

/* vim:et:sts=2:sw=2:ht=2
 * */
