/************************************************************************
 * Copyright (c) 2013 Richard P. Curnow
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


/* Implement spatial merging */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include "tool.h"

#define MAX_LCS 16

/* Change this value when making any incompatible change to the format of the
 * file */
#define VERSION 0x02

struct cfile {
  FILE *f;
  uint32_t n;
};

/* Information about what to show for each 'network' (2g or 3g) in each 1/256
 * of the tile */
struct network/*{{{*/
{
  int visited;
  int newest_timeframe; /* todo : could combine with visited! */
  int asu;
  int n_lcs;
  struct laccid lcs[MAX_LCS];
  unsigned short rel[MAX_LCS];
};
/*}}}*/
/* Information about each 1/256 of the tile */
struct micro/*{{{*/
{
  struct network g2;
  struct network g3;
};
/*}}}*/

struct tower_info/*{{{*/
{
  struct tower2 *tow;
  /* X, Y information is (tile<<4)+(subtile) */
  int X;
  int Y;
  int eno; /* 1=estimated, 0=observed */
  int multiple; /* 1=multiple towers appear at this position, 0=otherwise */
  int adjacent; /* 1=another tower is in an adjacent square, 0=otherwise (or multiple set) */
};
/*}}}*/
struct tower_info_table/*{{{*/
{
  struct tower_info *tinfo2[17];
  struct tower_info *tinfo3[17];
  int ti_size2[17];
  int ti_size3[17];
};
/*}}}*/
static int tower_info_compare(const struct tower_info *a, const struct tower_info *b)/*{{{*/
{
  if (a->X < b->X) return -1;
  else if (a->X > b->X) return +1;
  else if (a->Y < b->Y) return -1;
  else if (a->Y > b->Y) return +1;
  else return 0;
  /* We might get equalities on the coordinates, but we don't care about the
   * sort order or stability amongst entries at the same coordinates */
}
/*}}}*/
static struct tower_info *tower_info_search(struct tower_info *array, int N, int X, int Y)/*{{{*/
{
  int L, H, M;
  struct tower_info *tm;
  L = 0;
  H = N;
  while (H - L > 1) {
    M = (H + L) >> 1;
    tm = &array[M];
    if (X > tm->X) L = M;
    else if (X < tm->X) H = M;
    else if (Y > tm->Y) L = M;
    else if (Y < tm->Y) H = M;
    else return tm;
  }
  tm = &array[L];
  if ((X == tm->X) && (Y == tm->Y)) return tm;
  else return NULL;
}
/*}}}*/

static void emit(struct cfile *out, int c) {/*{{{*/
  fputc(c, out->f);
  ++out->n;
}
/*}}}*/
static void emit_ui32(struct cfile *out, uint32_t x)/*{{{*/
{
  /* Write the integer in big-endian order, to match Java's RandomAccessFile
   * read behaviour */
  fputc(((x >> 24) & 0xff), out->f);
  fputc(((x >> 16) & 0xff), out->f);
  fputc(((x >>  8) & 0xff), out->f);
  fputc(((x      ) & 0xff), out->f);
  out->n += 4;
}
/*}}}*/
static void emit_delta_16(struct cfile *out, uint32_t delta)/*{{{*/
{
  fputc(((delta >>  8) & 0xff), out->f);
  fputc(((delta      ) & 0xff), out->f);
  out->n += 2;
}
/*}}}*/
static void emit_delta_32(struct cfile *out, uint32_t delta)/*{{{*/
{
  fputc(((delta >> 24) & 0xff), out->f);
  fputc(((delta >> 16) & 0xff), out->f);
  fputc(((delta >>  8) & 0xff), out->f);
  fputc(((delta      ) & 0xff), out->f);
  out->n += 4;
}
/*}}}*/
static int reduce_displacement(struct cfile *out, int displacement)/*{{{*/
{
  /* Must return a value between 1 and 16. */
  if (displacement >= 33) {
    emit(out, 0xf0);
    emit(out, displacement - 1);
    return 1;
  } else if (displacement >= 17) {
    emit(out, 0xcf);
    return displacement - 16;
  } else { /* already 1..16 */
    return displacement;
  }
}
/*}}}*/
static void cancel_displacement(struct cfile *out, int displacement)/*{{{*/
{
  if (displacement > 17) {
    emit(out, 0xf0);
    emit(out, displacement - 1);
  } else if (displacement >= 2) {
    emit(out, 0xc0 | (displacement - 2));
  } else {
    /* displacement is 1 - NTD */
  }
}
/*}}}*/
static void emit_preamble(struct cfile *out)/*{{{*/
{
  uint32_t control_word;
  control_word = 0xfe << 24;
  control_word |= (uint32_t) 'O' << 16;
  control_word |= (uint32_t) 'V' << 8;
  control_word |= VERSION;
  emit_ui32(out, control_word);
}
/*}}}*/
static void emit_circle(struct cfile *out, int size, int colour_index, int displacement)/*{{{*/
{
  displacement = reduce_displacement(out, displacement);
  emit(out, 0x60 | (displacement - 1));
  /* Ok up to 16 colours only... */
  emit(out, (size<<4) | colour_index);
}
/*}}}*/
static void emit_sector(struct cfile *out, int size, int colour_index, double bearing, int displacement)/*{{{*/
{
  int b;
  /* Encode into 5 bits.  Maybe revise ...? */
  b = (int) (0.5 + bearing * (32.0 / 360.0)) & 0x1f;
  cancel_displacement(out, displacement);
  emit(out, 0x40 | b);
  /* Ok up to 16 colours only... */
  emit(out, (size<<4) | colour_index);
}
/*}}}*/
static void emit_null(struct cfile *out, int displacement)/*{{{*/
{
  displacement = reduce_displacement(out, displacement);
  emit(out, 0xb0 | (displacement - 1));
}
/*}}}*/
static void emit_fat_circle(struct cfile *out, int size, int colour_index, int displacement)/*{{{*/
{
  displacement = reduce_displacement(out, displacement);
  emit(out, 0x70 | (displacement - 1));
  /* Ok up to 16 colours only... */
  emit(out, (size<<4) | colour_index);
}
/*}}}*/
static void emit_long_vane(struct cfile *out, int32_t ang)/*{{{*/
{
  int ang5;
  ang5 = ang >> (32 - 6);
  ang5 += 1;
  ang5 >>= 1;
  ang5 &= 0x1f;
  emit(out, ang5); /* opcode=000 */
}
/*}}}*/
static void emit_short_vane(struct cfile *out, int32_t ang)/*{{{*/
{
  int ang5;
  ang5 = ang >> (32 - 6);
  ang5 += 1;
  ang5 >>= 1;
  ang5 &= 0x1f;
  emit(out, 0x20 | ang5); /* opcode=001x */
}
/*}}}*/
static void emit_eot(struct cfile *out)/*{{{*/
{
  emit(out, 0xff);
}
/*}}}*/
static void emit_regular_tower(struct cfile *out, int colour_index,/*{{{*/
    int eno,
    const char *caption,
    int displacement)
{
  int len;
  int i;
  len = caption ? strlen(caption) : 0;
  if (len > 7) len = 7;
  displacement = reduce_displacement(out, displacement);
  emit(out, 0x90 | (displacement - 1));
  emit(out, ((eno & 1) << 7) | (len << 4) | (colour_index & 0xf));
  for (i=0; i<len; i++) {
    emit(out, caption[i]);
  }
}
/*}}}*/
static void emit_adjacent_tower(struct cfile *out, int colour_index, int eno, int displacement)/*{{{*/
{
  displacement = reduce_displacement(out, displacement);
  emit(out, 0x80 | (displacement - 1));
  emit(out, ((eno & 1) << 7) | (colour_index & 0xf));
}
/*}}}*/
static void emit_multiple_tower(struct cfile *out, int displacement)/*{{{*/
{
  displacement = reduce_displacement(out, displacement);
  emit(out, 0xa0 | (displacement - 1));
}
/*}}}*/
#define NT 4
static void find_tower_choices(struct network *n, struct tower_table *table, struct tower2 **t)/*{{{*/
{
  /* Report up to 4 towers */
  int nt = 0;
  int i;
  int count[NT];
  memset(t, 0, NT * sizeof(struct tower2 *));
  memset(count, 0, NT * sizeof(int));
  for (i = 0; i < n->n_lcs; i++) {
    struct tower2 *tow;
    int j;
    tow = cid_to_tower(table, n->lcs[i].lac, n->lcs[i].cid);
    if (tow) {
      for (j = 0; j < nt; j++) {
        if (t[j] == tow) {
          count[j] += n->rel[i];
          goto done;
        }
      }
      if (nt < NT) {
        t[nt] = tow;
        count[nt] = n->rel[i];
        nt++;
      }
done:
      (void) 0;
    }
  }
  /* Annul any tower whose sample count is below a quarter of the highest one */
  for (i = 1; i < nt; i++) {
    if (((count[i] << 2) / count[0]) < 1) {
      continue;
      t[i] = NULL;
    }
  }
}
/*}}}*/
static void do_micro_tile(struct cfile *out, struct network *n, struct tower_table *table, struct M28 *micro_centre, int displacement)/*{{{*/
{
  int size;
  struct tower2 *tower_choices[NT];
  int i;

  size = (int)sqrt(1.0 + 2.5*n->asu);
  find_tower_choices(n, table, tower_choices);

  if (n->lcs[0].cid > 0) {
    struct tower2 *tow;
#if 0
    struct M28 *towpos;
#endif
    tow = cid_to_tower(table, n->lcs[0].lac, n->lcs[0].cid);
    if (tow) {
      int ci = tow->colour_index;
      struct panel *pan;
      if (ci <= 0) {
        fprintf(stderr, "Bad colour index for tower (%s,%s) %s\n",
            tow->label_2g ?: "-",
            tow->label_3g ?: "-",
            tow->description);
        exit(2);
      }
      pan = cid_to_panel(table, n->lcs[0].lac, n->lcs[0].cid);
      if (pan->group->has_bearing) {
        emit_sector(out, size, ci, pan->bearing, displacement);
      } else {
        emit_circle(out, size, ci, displacement);
      }

#if 0
      if (tow->has_observed) {
        towpos = &tow->observed;
      } else if (tow->has_estimated) {
        towpos = &tow->estimated;
      } else {
        towpos = NULL;
      }

      if (towpos) {
        /* Do 'vanes' */
        int32_t dx, dy, ang;
        dx = towpos->X - micro_centre->X;
        dy = towpos->Y - micro_centre->Y;
        ang = atan2i_approx2(dy, dx);
        emit_long_vane(out, ang);
      }
#endif

    } else {
      emit_fat_circle(out, size, 0, displacement);
    }
  } else if (n->lcs[0].lac == 0) {
    /* 'Emergency calls only' */
    emit_circle(out, size, 0, displacement);
  } else {
    /* Completely null coverage */
    emit_null(out, displacement);
  }

  for (i = 0; i < NT; i++) {
    struct tower2 *tow;
    tow = tower_choices[i];
    if (tow) {
      struct M28 *towpos;
      if (tow->has_observed) {
        towpos = &tow->observed;
      } else if (tow->has_estimated) {
        towpos = &tow->estimated;
      } else {
        towpos = NULL;
      }
      if (towpos) {
        /* Do 'vanes' */
        int32_t dx, dy, ang;
        dx = towpos->X - micro_centre->X;
        dy = towpos->Y - micro_centre->Y;
        ang = atan2i_approx2(dy, dx);
        if (i == 0) {
          emit_long_vane(out, ang);
        } else {
          emit_short_vane(out, ang);
        }
      }
    }
  }
}
/*}}}*/

static struct M28 m28_position(int zoom, int I, int J, int i, int j)/*{{{*/
{
  struct M28 result;
  /* Mid tile position - hence the extra '1' term */
  result.X = (I << (28 - zoom)) + (i << (28 - zoom - 4)) + (1 << (28 - zoom - 5));
  result.Y = (J << (28 - zoom)) + (j << (28 - zoom - 4)) + (1 << (28 - zoom - 5));
  return result;
}
/*}}}*/
static int do_net3(int zoom, int I, int J, int gen,/*{{{*/
    struct micro sub[16][16],
    struct tower_table *table,
    struct tower_info *ti, int nti,
    struct cfile *out)
{
  int i;
  int j;
  int pos, last_pos, displacement;
  int any;

  any = 0;
  last_pos = -1;
  for (i=0; i<16; i++) {
    for (j=0; j<16; j++) {
      struct micro *m = &sub[i][j];
      struct network *n = (gen == 2) ? &(m->g2) : &(m->g3);
      struct M28 micro_centre;
      struct tower_info *display_tower = tower_info_search(ti, nti, (I<<4)+i, (J<<4)+j);
      pos = (i<<4) | j;
      displacement = pos - last_pos;
      if (display_tower) {
        struct tower2 *tow = display_tower->tow;
        last_pos = pos;
        if (display_tower->multiple) {
          emit_multiple_tower(out, displacement);
        } else if (display_tower->adjacent) {
          emit_adjacent_tower(out, tow->colour_index, display_tower->eno, displacement);
        } else {
          /* Regular tower */
          emit_regular_tower(out, tow->colour_index,
              display_tower->eno,
              (gen == 2) ? tow->label_2g : tow->label_3g,
              displacement);
        }
        any = 1;

      } else if (n->visited) {
        last_pos = pos;
        micro_centre = m28_position(zoom, I, J, i, j);
        do_micro_tile(out, n, table, &micro_centre, displacement);
        any = 1;
      }
    }
  }
  if (any) {
    emit_eot(out);
  }
  return any;
}
/*}}}*/
static int render(int zoom, int I, int J, struct node *cursor,/*{{{*/
    struct micro sub[16][16],
    struct tower_table *table,
    struct tower_info_table *ti_table,
    struct cfile *out, uint32_t *pos)
{
  int result = 0;
  pos[0] = out->n;
  if (do_net3(zoom, I, J, 2, sub, table, ti_table->tinfo2[zoom], ti_table->ti_size2[zoom], out)) {
    result |= 0x01;
  }
  pos[1] = out->n;
  if (do_net3(zoom, I, J, 3, sub, table, ti_table->tinfo3[zoom], ti_table->ti_size3[zoom],out)) {
    result |= 0x02;
  }
  return result;
}
/*}}}*/
static void do_network(struct data *d, int gen, struct network *net)/*{{{*/
{
  struct odata *od;
  od = compute_odata(d, gen);
  if (od) {
    int n_lcs, i;
    net->visited = 1;
    net->newest_timeframe = od->newest_timeframe;
    net->asu = od->asu;
    n_lcs = od->n_lcs;
    if (n_lcs > MAX_LCS) {
      n_lcs = MAX_LCS;
    }
    net->n_lcs = n_lcs;
    for (i=0; i<n_lcs; i++) {
      net->lcs[i] = od->lcs[i];
      net->rel[i] = od->rel[i];
    }
    free_odata(od);
  } else {
    net->visited = 0;
  }
}
/*}}}*/
static void do_fragment(struct data *d, struct micro *frag)/*{{{*/
{
  do_network(d, 2, &frag->g2);
  do_network(d, 3, &frag->g3);
}
/*}}}*/
static void gather(int to_go, int I, int J, struct node *cursor, struct micro sub[16][16])/*{{{*/
{
  int i;
  int j;
  if (to_go > 0) {
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          gather(to_go - 1, (I<<1)+i, (J<<1)+j, cursor->c[i][j], sub);
        }
      }
    }
  } else {
    /* Process this 1/16 x 1/16 block */
    if (cursor->d) {
      /* could be null at other than deepest zoom in tiles that have no
       * coverage data? */
      do_fragment(cursor->d, &sub[I][J]);
    }
  }
}
/*}}}*/
static void clear_sub(struct micro sub[16][16])/*{{{*/
{
  int i, j;
  for (i=0; i<16; i++) {
    for (j=0; j<16; j++) {
      sub[i][j].g2.visited = 0;
      sub[i][j].g3.visited = 0;
    }
  }
}
/*}}}*/
static int do_tile(int zoom, int I, int J,/*{{{*/
    struct node *cursor,
    struct tower_table *table,
    struct tower_info_table *ti_table,
    struct cfile *out, uint32_t *pos)
{
  struct micro sub[16][16];
  int result;
  /* Clear the array */
  clear_sub(sub);

  gather(4, 0, 0, cursor, sub);
  result = render(zoom, I, J, cursor, sub, table, ti_table, out, pos);
  return result;
}
/*}}}*/
static uint32_t inner(int zoomlo, int zoomhi,/*{{{*/
    int level,
    struct node *cursor,
    int I, int J,
    struct tower_table *table,
    struct tower_info_table *ti_table,
    struct cfile *out) {
  int i, j;
  uint32_t pos[6];
  uint32_t delta[6];
  int is_wide;
  uint32_t result;
  unsigned char present = 0;
  memset(pos, 0, sizeof(pos));
  if (level < zoomhi) {
    /* Don't bother traversing to deeper tiles if we're already at deepest zoom level */
    for (i=0; i<2; i++) {
      for (j=0; j<2; j++) {
        if (cursor->c[i][j]) {
          int idx = i + i + j;
          pos[idx] = inner(zoomlo, zoomhi, level+1, cursor->c[i][j], (I<<1)+i, (J<<1)+j,
              table, ti_table, out);
          present |= (1<<idx);
        }
      }
    }
  }
  if (level >= zoomlo) {
    int tiles_present;
    tiles_present = do_tile(level, I, J, cursor, table, ti_table, out, pos+4);
    present |= (tiles_present<<4);
  }

  result = out->n;
  is_wide = 0;
  for (i=0; i<6; i++) {
    if (present & (1<<i)) {
      delta[i] = result - pos[i];
      if (delta[i] > 65535) is_wide = 1;
    }
  }

  if (is_wide) present |= 0x80;
  emit(out, present);
  for (i=0; i<6; i++) {
    if (present & (1<<i)) {
      if (is_wide) {
        emit_delta_32(out, delta[i]);
      } else {
        emit_delta_16(out, delta[i]);
      }
    }
  }
  return result;
}
/*}}}*/

#define SORT_FUNCTION_NAME tower_info_sort
#define TYPE struct tower_info
#define SORT_COMPARE_NAME tower_info_compare
#include "sort/mergesort_template.c"

static struct tower_info *build_tower_info(int zoom, struct tower_table *table, int gen, int *N)/*{{{*/
{
  int pass;
  struct tower_info *result = NULL;
  struct tower_info *scratch = NULL;
  int shift_amount = 28 - (zoom + 4);
  int count;
  int i;
  for (pass=0; pass<2; pass++) {
    count = 0;
    for (i=0; i<table->n_towers; i++) {
      struct tower2 *tow = table->towers[i];
      if (((gen == 2) && !tow->label_2g) ||
          ((gen == 3) && !tow->label_3g)) continue;
      if (tow->has_observed) {
        if (tow->has_estimated) {
          int xe, ye;
          int xo, yo;
          int dx, dy;
          xo = tow->observed.X >> shift_amount;
          yo = tow->observed.Y >> shift_amount;
          xe = tow->estimated.X >> shift_amount;
          ye = tow->estimated.Y >> shift_amount;
          /* Check for adjacency */
          dx = abs(xo - xe);
          dy = abs(yo - ye);
          if ((dx > 1) || (dy > 1)) {
            /* widely separated */
            if (pass == 0) {
              count += 2;
            } else {
              result[count].X = xo;
              result[count].Y = yo;
              result[count].eno = 0;
              result[count].tow = tow;
              count++;
              result[count].X = xe;
              result[count].Y = ye;
              result[count].eno = 1;
              result[count].tow = tow;
              count++;
            }
          } else {
            /* If observed and estimated positions are close together at this
             * zoom, only consider the observed position */
            if (pass == 0) {
              count++;
            } else {
#if 0
              printf("z=%d obs=%d,%d est=%d,%d %s too close\n",
                  zoom, xo, yo, xe, ye,
                  (gen == 2) ? tow->label_2g : tow->label_3g);
#endif

              result[count].X = xo;
              result[count].Y = yo;
              result[count].eno = 0;
              result[count].tow = tow;
              count++;
            }
          }
        } else {
          /* observed position but no estimated position (unlikely, but anyway..) */
          if (pass == 0) {
            count++;
          } else {
            result[count].X = tow->observed.X >> shift_amount;
            result[count].Y = tow->observed.Y >> shift_amount;
            result[count].eno = 0;
            result[count].tow = tow;
            count++;
          }
        }
      } else if (tow->has_estimated) {
          if (pass == 0) {
            count++;
          } else {
            result[count].X = tow->estimated.X >> shift_amount;
            result[count].Y = tow->estimated.Y >> shift_amount;
            result[count].eno = 1;
            result[count].tow = tow;
            count++;
          }
      }
    }
    if (!pass) {
      *N = count;
      result = malloc(count * sizeof(struct tower_info));
      for (i=0; i<count; i++) {
        result[i].multiple = result[i].adjacent = 0;
      }
    }
  }

  /* Now sort by coordinate */
  scratch = malloc(count * sizeof(struct tower_info));
  tower_info_sort(*N, result, scratch);
  free(scratch);

  /* Identify grid squares with > 1 tower in them. */
  for (i=1; i<*N; i++) {
    if ((result[i-1].X == result[i].X) &&
        (result[i-1].Y == result[i].Y)) {
      result[i-1].multiple = result[i].multiple = 1;
    }
  }

  /* Identify places where a tower is next to another tower */
  for (i=0; i<*N; i++) {
    int dx, X, dy, Y;
    struct tower_info *neighbour;
    if (result[i].adjacent || result[i].multiple) continue;
    for (dx=-1; dx<=1; dx++) {
      for (dy=-1; dy<=1; dy++) {
        if ((dx == 0) && (dy == 0)) continue;
        X = result[i].X + dx;
        Y = result[i].Y + dy;
        neighbour = tower_info_search(result, *N, X, Y);
        if (neighbour) {
          neighbour->adjacent = 1;
          result[i].adjacent = 1;
          goto next_record;
        }
      }
    }
next_record:
    (void) 0;
  }

#if 0
  for (i=0; i<*N; i++) {
    struct tower_info *q = &result[i];
    struct tower2 *qt = q->tow;
    printf("i=%3d g=%1d : %2d:%6d,%6d -> %5d,%5d %2d,%2d %s %s %s %s\n",
        i, gen, zoom, q->X, q->Y,
        q->X >> 4, q->Y >> 4, q->X & 15, q->Y & 15,
        q->multiple ? "MULTI" : "     ",
        q->adjacent ? "ADJ" : "   ",
        q->eno ? "EST" : "OBS",
        (gen == 2) ? qt->label_2g : qt->label_3g);
  }
#endif

  return result;
}
/*}}}*/
static void add_towers_locations_to_tree(struct tower_table *table, struct node *top)/*{{{*/
{
  /* If we don't do this, nothing is written to the overlay file for tiles that
   * contain only tower locations but no coverage. */
  int i;
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    if (tow->has_observed) {
      (void) lookup_spatial(top, tow->observed.X >> 8, tow->observed.Y >> 8);
    }
    if (tow->has_estimated) {
      (void) lookup_spatial(top, tow->estimated.X >> 8, tow->estimated.Y >> 8);
    }
  }
}
/*}}}*/
void overlay(int zoomlo, int zoomhi,/*{{{*/
    struct tower_table *table,
    struct node *top,
    FILE *out) 
{
  struct cfile cf;
  uint32_t offset, end4;
  struct tower_info_table ti_table;
  int i;

  add_towers_locations_to_tree(table, top);

  for (i=7; i<=16; i++) {
    ti_table.tinfo2[i] = build_tower_info(i, table, 2, &ti_table.ti_size2[i]);
    ti_table.tinfo3[i] = build_tower_info(i, table, 3, &ti_table.ti_size3[i]);
  }

  cf.f = out;
  cf.n = 0;
  emit_preamble(&cf);
  offset = inner(zoomlo, zoomhi, 0, top, 0, 0, table, &ti_table, &cf);
  end4 = cf.n;
  /* To let the reader navigate in */
  emit_ui32(&cf, end4 - offset);
  return;
}
/*}}}*/
/* vim:et:sts=2:sw=2:ht=2
 * */
