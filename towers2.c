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

/* Reading and writing the tower database file. */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include "tool.h"

static struct tower2 *add_tower(struct tower2 *list, struct tower2 *tow)/*{{{*/
{
  tow->next = list;
  return tow;
}
/*}}}*/
static void add_group(struct tower2 *tow, struct group *grp)/*{{{*/
{
  grp->next = tow->groups;
  grp->tower = tow;
  tow->groups = grp;
}
/*}}}*/
static void add_panel(struct group *grp, struct panel *pan)/*{{{*/
{
  pan->next = grp->panels;
  pan->group = grp;
  grp->panels = pan;
}
/*}}}*/
static void add_cell(struct panel *pan, struct cell *sc)/*{{{*/
{
  sc->next = pan->cells;
  pan->cells = sc;
}
/*}}}*/
static char *deblank(char *in)/*{{{*/
{
  char *x;
  for (x=in; *x && isspace(*x); x++) {}
  return x;
}
/*}}}*/
static const char *skip_space(const char *q)/*{{{*/
{
  while (*q && isspace(*q)) q++;
  return q;
}
/*}}}*/
static const char *parse_panel(int lineno, const char *p, struct panel *pan)/*{{{*/
{
  int n;
  char generation;
  int cid;
  struct cell *sc;
  while (*p) {
    if (isspace(*p)) {
      p++;
      continue;
    }
    if (*p == ']') {
      return p+1;
    }
    generation = *p++;
    if (sscanf(p, "%d%n", &cid, &n) == 1) {
      p += n;
      sc = malloc(sizeof(struct cell));
      switch (generation) {
        case 'G': sc->gen = CELL_G; break;
        case 'U': sc->gen = CELL_U; break;
        default:
          fprintf(stderr, "Can't understand cell generation %c at line %d\n", generation, lineno);
          return p;
      }
      sc->cid = cid;
      sc->panel = pan;
      add_cell(pan, sc);
    } else {
      fprintf(stderr, "Can't parse reset of line <%s> at line %d\n",
          p, lineno);
      return p;
    }
  }
  return p;
}
/*}}}*/
static void parse_group(int lineno, const char *p, struct group *grp)/*{{{*/
{
  while (*p) {
    if (isspace(*p)) {
      p++;
    } else if (*p == '[') {
      struct panel *panel;
      panel = malloc(sizeof (struct panel));
      memset(panel, 0, sizeof(struct panel));
      p = parse_panel(lineno, p+1, panel);
      add_panel(grp, panel);
    } else if (*p == '%') {
      int bearing;
      int n;
      p++;
      if (sscanf(p, "%d%n", &bearing, &n) != 1) {
        fprintf(stderr, "Unparsed bearing <%s> at line %d\n", p, lineno);
        return;
      }
      p += n;
      grp->has_bearing = 1;
      grp->bearing = (double) bearing;
    } else if (*p == '/') {
      return; /* trailing bearing information */
    } else {
      fprintf(stderr, "Unparsed group <%s> at line %d\n", p, lineno);
      return;
    }
  }
}
/*}}}*/
static void chomp(char *x)/*{{{*/
{
  int len;
  char *p;
  len = strlen(x);
  if (len > 0) {
    p = x + len - 1;
    while (p >= x) {
      if (isspace(*p)) {
        *p = 0;
        p--;
      } else {
        break;
      }
    }
  }
}
/*}}}*/
static void add_comment(struct tower2 *tow, const char *text)/*{{{*/
{
  struct comment *com;
  com = malloc(sizeof(struct comment));
  com->text = strdup(text);
  com->next = tow->comments;
  tow->comments = com;
}
/*}}}*/
static void parse_lacs(const char *p, struct tower2 *tow, int lineno, const char *filename)/*{{{*/
{
  const char *q = p;
  int lac, nc;
  /* Should be looking 1 char beyond the > */
  q = skip_space(q);
  while (*q) {
    switch (*q) {
      case 'G':
        if (sscanf(q+1, "%d%n", &lac, &nc) >= 1) {
          if (tow->n_lacs_g >= MAX_LACS) {
            goto excess_lacs;
          }
          tow->lacs_g[tow->n_lacs_g++] = lac;
          q += nc+1;
        } else {
          goto bad_lac;
        }
        break;
      case 'U':
        if (sscanf(q+1, "%d%n", &lac, &nc) >= 1) {
          if (tow->n_lacs_u >= MAX_LACS) {
            goto excess_lacs;
          }
          tow->lacs_u[tow->n_lacs_u++] = lac;
          q += nc+1;
        } else {
          goto bad_lac;
        }
        break;
      default:
        fprintf(stderr, "Unrecognized tag '%c' in LAC line %d in %s\n", *q, lineno, filename);
        return;
    }
    q = skip_space(q);
  }
  return;

excess_lacs:
  fprintf(stderr, "Too many LACs at line %d in %s\n", lineno, filename);
  return;

bad_lac:
  fprintf(stderr, "Unparsed LAC number line %d in %s\n", lineno, filename);
  return;
}
/*}}}*/
#if 0
static int parse_lac(const char *p, struct lac *lac)/*{{{*/
{
  int n = 0;
  int width;
  const char *q = p;
  if (sscanf(p, "%d%n", &lac->lac, &width) == 1) {
    p += width;
    if (*p != '>') {
      fprintf(stderr, "Bad LAC line %s\n", q);
      exit(2);
    }
    p++;
    do {
      int cid;
      if (sscanf(p, "%d%n", &cid, &width) == 1) {
        p += width;
        lac->cids[n] = cid;
        n++;
      } else {
        break;
      }
    } while (1);
    lac->n_cids = n;
    return 1;
  } else {
    fprintf(stderr, "Bad LAC line %s\n", q);
    exit(1);
  }
}
/*}}}*/
#endif
static struct tower2 *read_towers_from_file(const char *filename)/*{{{*/
{
  FILE *in;
  int lineno;
  char buffer[1024];
  char *p;
  struct tower2 *tow;
  struct group *grp;
  struct tower2 *result = NULL;
  in = fopen(filename, "r");
  if (!in) {
    fprintf(stderr, "Could not open %s for input\n", filename);
    exit(2);
  }
  lineno = 0;
  tow = NULL;
  while (fgets(buffer, sizeof(buffer), in)) {
    lineno++;
    if (is_blank(buffer)) continue;
    chomp(buffer);
    switch (buffer[0]) {
      case '#':
        break;
      case '=':
        /* Start new tower */
        tow = malloc(sizeof(struct tower2));
        memset(tow, 0, sizeof(struct tower2));
        p = deblank(buffer+1);
        tow->description = strdup(p);
        tow->colour_index = -1;
        result = add_tower(result, tow);
        break;
      case '^':
        if (buffer[1] == '2') {
          tow->label_2g = strdup(deblank(buffer+2));
        } else if (buffer[1] == '3') {
          tow->label_3g = strdup(deblank(buffer+2));
        } else {
          fprintf(stderr, "Meaningless caption line at %d in %s\n", lineno, filename);
        }
        break;
      case '%':
        p = deblank(buffer + 1);
        chomp(p);
        add_comment(tow, p);
        break;

      case '!':
        tow->colour_index = atoi(buffer+1);
        break;
      case '+':
        /* ignore hexcode and recompute later */
        tow->hexcode = strdup(deblank(buffer+1));
        break;
      case '@':
        if (buffer[1] == '@') {
          if (sscanf(buffer+2, "%d%d", &tow->observed.X, &tow->observed.Y) == 2) {
            tow->has_observed = 1;
          } else {
            fprintf(stderr, "Meaningless location line at %d in %s\n", lineno, filename);
          }
        } else if (buffer[1] == '?') {
          if (sscanf(buffer+2, "%d%d", &tow->estimated.X, &tow->estimated.Y) == 2) {
            tow->has_estimated = 1;
          } else {
            fprintf(stderr, "Meaningless location line at %d in %s\n", lineno, filename);
          }
        } else {
          fprintf(stderr, "Meaningless location line at %d in %s\n", lineno, filename);
        }
        break;
      case '*':
        p = buffer + 1;
        grp = malloc(sizeof(struct group));
        memset(grp, 0, sizeof(struct group));
        add_group(tow, grp);
        if (*p == '?') {
          p++;
          grp->no_order = 1;
        }
        parse_group(lineno, p, grp);
        break;
      case '<':
        if (buffer[1] == '>') {
          parse_lacs(buffer+2, tow, lineno, filename);
        } else {
          fprintf(stderr, "Meaningless LAC line at %d in %s\n", lineno, filename);
        }
        break;
      default:
        fprintf(stderr, "Unparsed line %d in %s\n", lineno, filename);
        break;
    }
  }

  fclose(in);
  return result;
}
/*}}}*/
struct list {/*{{{*/
  struct list *next;
};
/*}}}*/
static void *reverse(void *x)/*{{{*/
{
  struct list *a, *na, *aa;
  aa = NULL;
  for (a = (struct list *)x; a; a = na) {
    na = a->next;
    a->next = aa;
    aa = a;
  }
  return (void *)aa;
}
/*}}}*/
static struct tower2 *reverse_lists(struct tower2 *list)/*{{{*/
{
  struct tower2 *tow, *ntow;
  struct tower2 *result = NULL;
  for (tow = list; tow; tow = ntow) {
    struct group *g, *ng, *gg;
    ntow = tow->next;
    gg = NULL;
    for (g = tow->groups; g; g=ng) {
      struct panel *p, *np, *pp;
      ng = g->next;
      g->next = gg;
      gg = g;

      pp = NULL;
      for (p = g->panels; p; p = np) {
        struct cell *c, *nc, *cc;
        np = p->next;
        p->next = pp;
        pp = p;

        /* flip cid order */
        cc = NULL;
        for (c = p->cells; c; c = nc) {
          nc = c->next;
          c->next = cc;
          cc = c;
        }
        p->cells = cc;
      }
      g->panels = pp;
    }
    tow->groups = gg;
    tow->comments = reverse(tow->comments);
    result = add_tower(result, tow);
  }
  return result;
}
/*}}}*/
static inline double degof(double rad)/*{{{*/
{
  return rad * (180.0/M_PI);
}
/*}}}*/
static inline double radof(double deg)/*{{{*/
{
  return deg * (M_PI / 180.0);
}
/*}}}*/
void m28_to_wgs84(const struct M28 *m28, double *lat, double *lon)/*{{{*/
{
  double t;
  const double scale = 1.0 / (double) (1<<28);
  *lon = degof(M_PI * (((2.0 * scale) * (double)m28->X)-1.0));
  t = M_PI * (1.0 - ((2.0 * scale) * (double)m28->Y));
  t = 2.0 * atan(exp(t)) - (0.5*M_PI);
  *lat = degof(t);
}
/*}}}*/
void wgs84_to_m28(double lat, double lon, int *X, int *Y)/*{{{*/
{
  double latr, lonr;
  const double scale = 1.0 / (double) (1<<28);
  double t, xx, yy;
  latr = radof(lat);
  lonr = radof(lon);
  t = log(tan(0.5*latr) + 0.25*M_PI);
  xx = 0.5 * (1.0 + lonr / M_PI);
  yy = 0.5 * (1.0 - t / M_PI);
  *Y = (int) floor(yy * scale);
  *X = (int) floor(xx * scale);
}
/*}}}*/
static char *m28_to_osgb_internal(const struct M28 *m28, const char *format)/*{{{*/
{
  static const char letters1[] = "VQLFAWRMGBXSNHCYTOJDZUPKE";
  static const char letters0[] = "SNHTOJ";
  const double scale = 1.0 / (double) (1<<28);
  double x = scale * (double)m28->X;
  double y = scale * (double)m28->Y;
  double alpha, beta;
  double t90, t91, t92, t93, t94, t95, t96, t97;
  double t98, t99, t100;
  double t101, t102, t103, t104, t105, t106, t107, t108;
  double t109;
  double alpha2;
  double beta2;
  double beta4;
  double E, N;
  char buffer[13];
  int e, n, e0, e1, n0, n1;
  char c0, c1;
  alpha = 61.000 * (x - 0.4944400930);
  beta = 36.000 * (y - 0.3126638550);
  if ((alpha < -1.0) || (alpha > 1.0) || (beta < -1.0) || (beta > 1.0)) {
    return NULL;
  }
  alpha2 = alpha * alpha;
  beta2 = beta * beta;
  beta4 = beta2 * beta2;
  t90 = (400001.47) + (-17.07)*beta;
  t91 = (370523.38) + (53326.92)*beta;
  t92 = (2025.68) + (-241.27)*beta;
  t93 = t91 + t92*beta2;
  t94 = t93 + (-41.77)*beta4;
  t95 = t90 + t94*alpha;
  t96 = (-11.21)*beta;
  t97 = t96 + (14.84)*beta2;
  t98 = (-237.68) + (82.89)*beta;
  t99 = t98 + (41.21)*beta2;
  t100 = t97 + t99*alpha;
  E = t95 + t100*alpha2;
  t101 = (649998.33) + (-13.90)*alpha;
  t102 = t101 + (15782.38)*alpha2;
  t103 = (-626496.42) + (1220.67)*alpha2;
  t104 = t102 + t103*beta;
  t105 = (-44898.11) + (10.01)*alpha;
  t106 = t105 + (-217.21)*alpha2;
  t107 = (-1088.27) + (-49.59)*alpha2;
  t108 = t106 + t107*beta;
  t109 = t104 + t108*beta2;
  N = t109 + (107.47)*beta4;
  if ((E < 0.0) || (E >= 700000.0) || (N < 0.0) || (N >= 1300000)) {
    return NULL;
  }
  e = (int)(0.5 + E) / 10;
  n = (int)(0.5 + N) / 10;
  e0 = e / 10000;
  e1 = e % 10000;
  n0 = n / 10000;
  n1 = n % 10000;

  c0 = letters0[3*(e0/5) + (n0/5)];
  c1 = letters1[5*(e0%5) + (n0%5)];
  sprintf(buffer, format, c0, c1, e1, n1);
  return strdup(buffer);
}
/*}}}*/
char *m28_to_osgb(const struct M28 *m28)/*{{{*/
{
  return m28_to_osgb_internal(m28, "%1c%1c %04d %04d");
}
/*}}}*/
char *m28_to_osgb_nospace(const struct M28 *m28)/*{{{*/
{
  return m28_to_osgb_internal(m28, "%1c%1c%04d%04d");
}
/*}}}*/
static void write_towers_to_file(const char *filename, struct tower_table *table)/*{{{*/
{
  FILE *out;
  int i, j;
  out = fopen(filename, "w");
  if (!out) {
    fprintf(stderr, "Could not open output file\n");
    exit (2);
  }
  for (i = 0; i < table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    struct group *grp;
    struct comment *com;
    int first;
    if (tow->description) {
      fprintf(out, "= %s\n", tow->description);
    } else {
      fprintf(stderr, "Bogus : no description for tower??\n");
      exit(2);
    }
    for (com = tow->comments; com; com = com->next) {
      fprintf(out, "%% %s\n", com->text);
    }
    if (tow->colour_index >= 0) { fprintf(out, "!%d\n", tow->colour_index); }
    if (tow->label_2g) { fprintf(out, "^2 %s\n", tow->label_2g); }
    if (tow->label_3g) { fprintf(out, "^3 %s\n", tow->label_3g); }
    if (tow->has_observed) {
      double lat, lon;
      char *os;
      m28_to_wgs84(&tow->observed, &lat, &lon);
      os = m28_to_osgb(&tow->observed);
      fprintf(out, "@@ %d %d // %.5f %.5f // %s\n",
          tow->observed.X, tow->observed.Y,
          lat, lon,
          os ? os : "<unknown>");
      free(os);
    }
    if (tow->has_estimated) {
      double lat, lon;
      char *os;
      m28_to_wgs84(&tow->estimated, &lat, &lon);
      os = m28_to_osgb(&tow->estimated);
      fprintf(out, "@? %d %d // %.5f %.5f // %s\n",
          tow->estimated.X, tow->estimated.Y,
          lat, lon,
          os ? os : "<unknown>");
      free(os);
    }
    if (tow->hexcode) {
      fprintf(out, "+ %s\n", tow->hexcode);
    }
    fprintf(out, "<>");
    for (j=0; j<tow->n_lacs_g; j++) {
      fprintf(out, " G%d", tow->lacs_g[j]);
    }
    for (j=0; j<tow->n_lacs_u; j++) {
      fprintf(out, " U%d", tow->lacs_u[j]);
    }
    fprintf(out, "\n");
    for (grp = tow->groups; grp; grp = grp->next) {
      struct panel *pan;
      int n_panels;
      fputc('*', out);
      if (grp->no_order) fputc('?', out);
      if (grp->has_bearing) {
        fprintf(out, " %%%03d", (int)grp->bearing);
      }
      for (pan = grp->panels, n_panels = 0; pan; pan = pan->next, n_panels++) {
        struct cell *sc;
        first = 1;
        fputc(' ', out);
        fputc('[', out);
        for (sc = pan->cells; sc; sc = sc->next) {
          if (!first) fputc(' ', out);
          first = 0;
          switch (sc->gen) {
            case CELL_G:
              fprintf(out, "G%d", sc->cid);
              break;
            case CELL_U:
              fprintf(out, "U%d", sc->cid);
              break;
          }
        }
        fputc(']', out);
      }
      if (grp->has_bearing) {
        int k;
        double step;
        step = 360.0 / (double) n_panels;
        for (k=0; k<n_panels; k++) {
          if (k == 0) {
            fprintf(out, " // %d", (int) grp->bearing);
          } else {
            fprintf(out, ",%d", (int)(grp->bearing + k * step) % 360);
          }
        }
      }
      fputc('\n', out);
    }
    fputc('\n', out);
  }
  fclose(out);
}
/*}}}*/
static struct tower_table *build_towers_table(struct tower2 *list)/*{{{*/
{
  struct tower2 *tow;
  int i;
  int n;
  struct tower_table *result;
  n = 0;
  for (tow = list; tow; tow = tow->next) {
    n++;
  }
  result = malloc(sizeof(struct tower_table));
  result->n_towers = n;
  result->towers = (struct tower2 **) malloc(n * sizeof(struct tower2 *));
  for (tow = list, i=0; tow; i++, tow = tow->next) {
    result->towers[i] = tow;
    tow->index = i;
  }
  return result;
}
/*}}}*/
char *compute_hexcode(int X, int Y)/*{{{*/
{
  char buffer[15];
  static const char lookup[17] = "0123456789ABCDEF";
  int i;
  for (i=0; i<14; i++) {
    int j = 13 - i;
    int j2 = j+j;
    int xx1 = (X >> (j2+1)) & 1;
    int yy1 = (Y >> (j2+1)) & 1;
    int xx0 = (X >> (j2)) & 1;
    int yy0 = (Y >> (j2)) & 1;
    buffer[i] = lookup[(xx1<<3) | (yy1<<2) | (xx0<<1) | yy0];
  }
  buffer[14] = 0;
  return strdup(buffer);
}
/*}}}*/
static void build_hexcodes(struct tower_table *table)/*{{{*/
{
  int i;
  for (i = 0; i < table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    if (tow->hexcode) {
      free(tow->hexcode);
    }
    if (tow->has_observed) {
      tow->hexcode = compute_hexcode(tow->observed.X, tow->observed.Y);
    } else if (tow->has_estimated) {
      tow->hexcode = compute_hexcode(tow->estimated.X, tow->estimated.Y);
    }
  }
}
/*}}}*/
static int compare_towers(const void *a, const void *b)/*{{{*/
{
  const struct tower2 *aa = *(const struct tower2 **)a;
  const struct tower2 *bb = *(const struct tower2 **)b;
  /* Sort towers with no location information to the bottom of the file */
  if (aa->hexcode && bb->hexcode) {
    return strcmp(aa->hexcode, bb->hexcode);
  } else if (aa->hexcode) {
    return -1;
  } else if (bb->hexcode) {
    return +1;
  } else {
    /* Crude but what else? */
    return strcmp(aa->description, bb->description);
  }
}
/*}}}*/
static void sort_towers(struct tower_table *table)/*{{{*/
{
  qsort(table->towers, table->n_towers, sizeof(struct tower2 *), compare_towers);
}
/*}}}*/

#define CHAIN_SHIFT 16
#define N_CHAINS (1<<CHAIN_SHIFT)
#define CHAIN_MASK ((N_CHAINS) - 1)
struct lc_chain {/*{{{*/
  struct lc_chain *next;
  int lac;
  int cid;
  struct cell *cell;
};
/*}}}*/
struct lc_table {/*{{{*/
  struct lc_chain *chains[N_CHAINS];
};
/*}}}*/
static int hash(int cid)/*{{{*/
{
  int y;
  y = (cid & CHAIN_MASK) ^ ((cid >> CHAIN_SHIFT) & CHAIN_MASK);
  return y;
}
/*}}}*/
static void insert_lc_into_table(struct lc_table *table, int lac, int cid, struct cell *cell)/*{{{*/
{
  int h;
  struct lc_chain *cc;
  h = hash(cid);
  /* ABORT ON DUPLICATE CID! */
  for (cc = table->chains[h]; cc; cc = cc->next) {
    if ((cc->cid == cid) && (cc->lac == lac)) {
      fprintf(stderr, "(lac,cid)=(%d,%d) is duplicated in the tower table\n",
          lac, cid);
      exit(2);
    }
  }
  cc = malloc(sizeof(struct lc_chain));
  cc->lac = lac;
  cc->cid = cid;
  cc->cell = cell;
  cc->next = table->chains[h];
  table->chains[h] = cc;
}
/*}}}*/
void insert_extra_lac_into_table(struct tower_table *table, struct tower2 *tow, int gen, int lac)
{
  struct group *grp;
  for (grp = tow->groups; grp; grp = grp->next) {
    struct panel *pan;
    for (pan = grp->panels; pan; pan = pan->next) {
      struct cell *c;
      for (c = pan->cells; c; c = c->next) {
        int j;
        switch (c->gen) {
          case CELL_G:
            if (gen == 2) {
              insert_lc_into_table(table->lcs, lac, c->cid, c);
            }
            break;
          case CELL_U:
            if (gen == 3) {
              insert_lc_into_table(table->lcs, lac, c->cid, c);
            }
            break;
        }
      }
    }
  }
}

static void insert_lc_table(struct tower_table *table)/*{{{*/
{
  int i;
  table->lcs = malloc(sizeof(struct lc_table));
  memset(table->lcs, 0, sizeof(struct lc_table));
  for (i=0; i<table->n_towers; i++) {
    struct tower2 *tow = table->towers[i];
    struct group *grp;
    for (grp = tow->groups; grp; grp = grp->next) {
      struct panel *pan;
      for (pan = grp->panels; pan; pan = pan->next) {
        struct cell *c;
        for (c = pan->cells; c; c = c->next) {
          int j;
          switch (c->gen) {
            case CELL_G:
              for (j=0; j<tow->n_lacs_g; j++) {
                insert_lc_into_table(table->lcs, tow->lacs_g[j], c->cid, c);
              }
              break;
            case CELL_U:
              for (j=0; j<tow->n_lacs_u; j++) {
                insert_lc_into_table(table->lcs, tow->lacs_u[j], c->cid, c);
              }
              break;
          }
        }
      }
    }
  }
}
/*}}}*/
static void compute_panel_bearings(struct tower2 *tower_list)/*{{{*/
{
  struct tower2 *tow;
  for (tow = tower_list; tow; tow = tow->next) {
    struct group *g;
    for (g = tow->groups; g; g = g->next) {
      if (g->has_bearing) {
        struct panel *pan;
        int n_panels;
        int i;
        for (pan = g->panels, n_panels=0; pan; pan = pan->next) n_panels++;
        double incr = 360.0 / (double) n_panels;
        for (pan = g->panels, i = 0;
             pan;
             pan = pan->next, i++) {
          double b = g->bearing + (double) i * incr;
          if (b > 360.0) b -= 360.0;
          pan->bearing = b;
        }
      }
    }
  }
}
/*}}}*/
struct tower_table *read_towers2(const char *filename)/*{{{*/
{
  struct tower2 *tower_list;
  struct tower_table *result;
  tower_list = read_towers_from_file(filename);
  tower_list = reverse_lists(tower_list);
  compute_panel_bearings(tower_list);
  result = build_towers_table(tower_list);
  insert_lc_table(result);
  fprintf(stderr, "Read in %d towers\n", result->n_towers);
  fflush(stderr);
  return result;
}
/*}}}*/
struct cell *cid_to_cell(struct tower_table *table, int lac, int cid)/*{{{*/
{
  int h;
  struct lc_chain *cc;
  h = hash(cid);
  for (cc = table->lcs->chains[h]; cc; cc = cc->next) {
    if ((cc->lac == lac) && (cc->cid == cid)) {
      return cc->cell;
    }
  }
  return NULL;
}
/*}}}*/
struct cell *cid_to_cell_near(struct tower_table *table, int cid, double lon, double lat)/*{{{*/
{
  int h;
  int X, Y;
  int dx, dy, d, best_d=0;
  struct cell *best_cell = NULL;
  struct lc_chain *cc;
  h = hash(cid);
  wgs84_to_m28(lat, lon, &X, &Y);
  for (cc = table->lcs->chains[h]; cc; cc = cc->next) {
    if (cc->cid == cid) {
      struct cell *cl = cc->cell;
      struct group *g = cl->panel->group;
      struct M28 *m28;
      m28 = g->tower->has_observed ? &g->tower->observed :
            g->tower->has_estimated ? &g->tower->estimated : NULL;
      if (m28) {
        dx = m28->X - X;
        dy = m28->Y - Y;
        /* Crude Manhattan distance, will do for this! */
        d = abs(dx) + abs(dy);
        if (!best_cell || (d < best_d)) {
          best_d = d;
          best_cell = cl;
        }
      }
    }
  }
  return best_cell;
}
/*}}}*/
struct panel *cid_to_panel(struct tower_table *table, int lac, int cid)/*{{{*/
{
  struct cell *cell;
  cell = cid_to_cell(table, lac, cid);
  return cell ? cell->panel : NULL;
}
/*}}}*/
struct panel *cid_to_panel_near(struct tower_table *table, int cid, double lon, double lat)/*{{{*/
{
  struct cell *cell;
  cell = cid_to_cell_near(table, cid, lon, lat);
  return cell ? cell->panel : NULL;
}
/*}}}*/
struct group *cid_to_group(struct tower_table *table, int lac, int cid)/*{{{*/
{
  struct panel *panel;
  panel = cid_to_panel(table, lac, cid);
  return panel ? panel->group : NULL;
}
/*}}}*/
struct group *cid_to_group_near(struct tower_table *table, int cid, double lon, double lat)/*{{{*/
{
  struct panel *panel;
  panel = cid_to_panel_near(table, cid, lon, lat);
  return panel ? panel->group : NULL;
}
/*}}}*/
struct tower2 *cid_to_tower(struct tower_table *table, int lac, int cid)/*{{{*/
{
  struct group *group;
  group = cid_to_group(table, lac, cid);
  return group ? group->tower : NULL;
}
/*}}}*/
struct tower2 *cid_to_tower_near(struct tower_table *table, int cid, double lon, double lat)/*{{{*/
{
  struct group *group;
  group = cid_to_group_near(table, cid, lon, lat);
  return group ? group->tower : NULL;
}
/*}}}*/
void write_towers2(const char *filename, struct tower_table *table)/*{{{*/
{
  build_hexcodes(table);
  sort_towers(table);
  write_towers_to_file(filename, table);
}
/*}}}*/
static void free_tower(struct tower2 *tow)/*{{{*/
{
  void free_comments(struct comment *c) {
    struct comment *nc;
    for (; c; c=nc) {
      nc = c->next;
      free(c->text);
      free(c);
    }
  }
  void free_groups(struct group *g) {
    struct group *ng;
    struct panel *p, *np;
    struct cell *c, *nc;
    for (; g; g = ng) {
      ng = g->next;
      for (p = g->panels; p; p = np) {
        np = p->next;
        for (c = p->cells; c; c = nc) {
          nc = c->next;
          free(c);
        }
        free(p);
      }
      free(g);
    }
  }

  if (tow->description) free(tow->description);
  if (tow->hexcode) free(tow->hexcode);
  if (tow->label_2g) free(tow->label_2g);
  if (tow->label_3g) free(tow->label_3g);
  free_comments(tow->comments);
  free_groups(tow->groups);
  free(tow);
}
/*}}}*/
static void free_lc_table(struct lc_table *table)/*{{{*/
{
  if (table) {
    int h;
    for (h=0; h<N_CHAINS; h++) {
      struct lc_chain *cc, *ncc;
      for (cc = table->chains[h]; cc; cc = ncc) {
        ncc = cc->next;
        free(cc);
      }
    }
  }
  free(table);
}
/*}}}*/
void free_tower_table(struct tower_table *table)/*{{{*/
{
  int i;
  for (i=0; i<table->n_towers; i++) {
    /* TODO : the rest of it! */
    free_tower(table->towers[i]);
  }
  free_lc_table(table->lcs);
  free(table->towers);
  free(table);
}
/*}}}*/
#ifdef TEST
int main (int argc, char **argv)/*{{{*/
{
  struct tower_table *table;
  table = read_towers2("./towers.dat");
  write_towers2("./new_towers.dat", table);
  free_tower_table(table);
  return 0;
}
/*}}}*/
#endif /* TEST */

/* vim:et:sts=2:sw=2:ht=2
 * */
