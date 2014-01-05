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

static struct subcommand sub_grid = {/*{{{*/
  (OPT_UNTIL | OPT_SINCE | OPT_ZOOM | OPT_DATA | OPT_CIDS),
  "grid",
  "create ASCII-art graphic of these cells' sightings to stdout"
};
/*}}}*/
void usage_grid(void)/*{{{*/
{
  describe(&sub_grid);
}
/*}}}*/
struct range {/*{{{*/
  int x0;
  int x1;
  int y0;
  int y1;
};
/*}}}*/
static void inner_get_range(struct node *cursor,/*{{{*/
    int nc, struct laccid *laccids,
    int level, int I, int J,
    struct range *r)
{
  int i, j;
  if (level < 20) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_get_range(cursor->c[i][j], nc, laccids, level+1, (I<<1)+i, (J<<1)+j, r);
        }
      }
    }
  } else {
    for (i=0; i<nc; i++) {
      if ((lookup_cell(cursor->d, 2, laccids[i].lac, laccids[i].cid, 0)) ||
          (lookup_cell(cursor->d, 3, laccids[i].lac, laccids[i].cid, 0))) {
        if ((r->x0 < 0) || (r->x0 > I)) r->x0 = I;
        if ((r->x1 < 0) || (r->x1 < I)) r->x1 = I;
        if ((r->y0 < 0) || (r->y0 > J)) r->y0 = J;
        if ((r->y1 < 0) || (r->y1 < J)) r->y1 = J;
      }
    }
  }
}
/*}}}*/
static void get_range(struct node *top, int nc, struct laccid *laccids, struct range *r)/*{{{*/
{
  r->x0 = r->x1 = r->y0 = r->y1 = -1;
  inner_get_range(top, nc, laccids, 0, 0, 0, r);
}
/*}}}*/

struct perbox/*{{{*/
{
  unsigned char max_asu[MAX_CIDS];
};
/*}}}*/
struct grid/*{{{*/
{
  int tx0, ty0; /* tile coordinates */
  int ntx, nty; /* number of tiles */
  int bx0, by0; /* coordinates of tile at 4 zoom levels further in */
  int nbx, nby; /* number of single points */
  struct perbox *pb;
};
/*}}}*/
static struct perbox *perbox_addr(struct grid *g, int ii, int jj) {/*{{{*/
  ii -= g->bx0;
  jj -= g->by0;
  return &g->pb[ii*g->nby + jj];
}
/*}}}*/
static struct perbox *perbox_addr_2(struct grid *g, int ii, int jj) {/*{{{*/
  int z;
  int ti, tj;
  ti = ii - g->bx0;
  tj = jj - g->by0;
  z = ti*g->nby + tj;
#if 0
  fprintf(stderr, "ii=%d jj=%d ti=%d tj=%d z=%d\n", ii, jj, ti, tj, z);
  fflush(stderr);
#endif
  return &g->pb[z];
}
/*}}}*/
static void inner_maximise(struct node *cursor, int shift_amount, int nc, struct laccid *laccids, int level, int I, int J, struct grid *g)/*{{{*/
{
  int i, j;
  if (level < 20) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          inner_maximise(cursor->c[i][j], shift_amount, nc, laccids, level+1, (I<<1)+i, (J<<1)+j, g);
        }
      }
    }
  } else {
    struct perbox *pb = perbox_addr(g, I>>shift_amount, J>>shift_amount);
    for (i=0; i<nc; i++) {
      struct percid *pc;
      pc = lookup_cell(cursor->d, 2, laccids[i].lac, laccids[i].cid, 0);
      if (!pc) pc = lookup_cell(cursor->d, 3, laccids[i].lac, laccids[i].cid, 0);
      if (pc) {
        int high = 0;
        struct readings *r;
        for (r=pc->r; r; r=r->next) {
          int j;
          for (j=31; j>=0; j--) {
            if (r->asus[j] > 0) {
              if (j > high) high = j;
              break;
            }
          }
        }
        if (high > pb->max_asu[i]) pb->max_asu[i] = high;
      }
    }
  }
}
/*}}}*/
static struct grid *make_grid(struct node *top, int shift_amount, int nc, struct laccid *laccids, int tx0, int ty0, int ntx, int nty)/*{{{*/
{
  struct grid *g;
  int nb;
  g = malloc(sizeof(struct grid));
  g->tx0 = tx0;
  g->ntx = ntx;
  g->ty0 = ty0;
  g->nty = nty;
  g->nbx = ntx << 4;
  g->nby = nty << 4;
  g->bx0 = tx0 << 4;
  g->by0 = ty0 << 4;
  nb = g->nbx * g->nby;
  g->pb = malloc(nb * sizeof(struct perbox));
  memset(g->pb, 0, nb);
  inner_maximise(top, shift_amount, nc, laccids, 0, 0, 0, g);
  return g;
}
/*}}}*/
static void display_grid(FILE *out, struct grid *g, int nc)/*{{{*/
{
  int i, j, ii, jj;
  char *buf;
  int nx, ny;
  nx = g->ntx * 17 + 1;
  ny = g->nty * 17 + 1;
  buf = malloc(nx * ny);
  memset(buf, ' ', nx*ny);
  for (i=0; i<g->nty; i++) {
    for (ii=0; ii<16; ii++) {
      int I = i*16 + ii;
      int II = 1 + i*17 + ii;
      for (j=0; j<g->ntx; j++) {
        for (jj=0; jj<16; jj++) {
          char *addr;
          int J = j*16 + jj;
          int JJ = 1 + j*17 + jj;
          struct perbox *pb = perbox_addr_2(g, J+g->bx0, I+g->by0);
          int k;
          int hi_k=-1, hi_asu=0;
          for (k=0; k<nc; k++) {
            if (pb->max_asu[k] > hi_asu) {
              hi_k = k;
              hi_asu = pb->max_asu[k];
            }
          }
          addr = &buf[(nx * II) + JJ];
          if (hi_k >= 0) 
            *addr = 'A' + hi_k;
          else 
            *addr = ' ';
        }
      }
    }
  }

  /* Insert grid lines */
  for (i=0; i<g->nty; i++) {
    for (j=0; j<nx; j++) {
      buf[17*i*nx + j] = '-';
    } 
  }
  for (i=0; i<g->ntx; i++) {
    for (j=0; j<ny; j++) {
      buf[17*i + j*nx] = '|';
    } 
  }
  for (i=0; i<g->ntx; i++) {
    for (j=0; j<g->nty; j++) {
      buf[i*17 + j*17*nx] = '+';
    }
  }

  /* Insert numbers */


  for (i=0; i<ny; i++) {
    for (j=0; j<nx; j++) {
      char c = buf[i*nx + j];
      putchar(c);
    }
    putchar('\n');
  }

  free(buf);
}
/*}}}*/
int ui_grid(int argc, char **argv)/*{{{*/
{
  struct node *top;
  struct range range20;
  struct range rangez;
  int shift_amount;
  int ntx, nty;
  struct grid *result;
  struct cli *cli;

  cli = parse_args(argc, argv, &sub_grid);

  if ((cli->zoom < 7) || (cli->zoom > 16)) {
    fprintf(stderr, "Invalid zoom level %d\n", cli->zoom);
    return 2;
  }

  top = load_all_dbs(cli);

  if (cli->n_cids < 1) {
    fprintf(stderr, "No cids?\n");
    return 2;
  }

  if (cli->has_since || cli->has_until) {
    temporal_interval_filter(top, cli->has_since, cli->since_dd, cli->has_until, cli->until_dd);
  }

  /* Compute and check max range */
  get_range(top, cli->n_cids, cli->laccids, &range20);
  shift_amount = 20 - cli->zoom;
  rangez.x0 = range20.x0 >> shift_amount;
  rangez.x1 = range20.x1 >> shift_amount;
  rangez.y0 = range20.y0 >> shift_amount;
  rangez.y1 = range20.y1 >> shift_amount;
  ntx = 1 + rangez.x1 - rangez.x0;
  nty = 1 + rangez.y1 - rangez.y0;
  printf("Tile range... X:[%d,%d] Y:[%d,%d] (%d x %d total)\n",
      rangez.x0, rangez.x1, rangez.y0, rangez.y1, ntx, nty);

  /* Generate grid... */
  result = make_grid(top, shift_amount - 4, 
      cli->n_cids, cli->laccids,
      rangez.x0, rangez.y0, ntx, nty);
  display_grid(stdout, result, cli->n_cids);

  return 0;
}
/*}}}*/

/* vim:et:sts=2:sw=2:ht=2
 * */
