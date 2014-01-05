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
#include <assert.h>
#include "tool.h"

struct prune_data *load_prune_data(const char *filename)/*{{{*/
{
  struct prune_data *result = NULL;
  FILE *in;
  char line[256];
  int lac, cid, upto;
  struct prune_data *p;

  in = fopen(filename, "r");
  if (!in) {
    fprintf(stderr, "Could not open %s for reading\n", filename);
    exit(2);
  }

  while (fgets(line, sizeof(line), in)) {
    if (sscanf(line, "%d%d%d", &lac, &cid, &upto) == 3) {
      p = malloc(sizeof(struct prune_data));
      p->lac = lac;
      p->cid = cid;
      p->upto = upto;
      p->count = 0;
      p->next = result;
      result = p;
    } else {
      fprintf(stderr, "Bad line in %s\n", filename);
      fprintf(stderr, "Each line should be <lac> <cid> <upto>\n");
      exit(2);
    }
  }
  return result;
}
/*}}}*/
static int prune_this(struct prune_data *p, int lac, int cid, int timeframe)/*{{{*/
{
  while (p) {
    if ((p->lac == lac) && (p->cid == cid) && (timeframe <= p->upto)){
      ++p->count;
      return 1;
    }
    p = p->next;
  }
  return 0;
}
/*}}}*/
void report_pruning(const struct prune_data *p)/*{{{*/
{
  if (p) {
    fprintf(stderr, "Readings were pruned as follows:\n");
    fprintf(stderr, "<LAC> <<<CID>>>> <UPTO> <<COUNT>\n");
    while (p) {
      fprintf(stderr, "%5d %10d %6d %8d\n",
          p->lac, p->cid, p->upto, p->count);
      p = p->next;
    }
  }
}
/*}}}*/
static void read_multi(const char *dbname, struct node *top, struct prune_data *prunings)/*{{{*/
{
  FILE *in;
  char line[256], *p;
  in = fopen(dbname, "r");
  if (!in) {
    fprintf(stderr, "Could not open %s\n", dbname);
    exit(2);
  }
  while (fgets(line, sizeof(line), in)) {
    if (line[0] == '#') continue;
    p = line + strlen(line);
    assert(*p == 0);
    p--;
    while((p >= line) && isspace(*p)) {
      *p = 0;
      p--;
    }
    if (p > line) {
      read_db(line, top, prunings);
    }
  }
  fclose(in);
}
/*}}}*/
void read_db(const char *dbname, struct node *top, struct prune_data *prunings) /*{{{*/
{
  FILE *in;
  char buffer[256];
  int lineno;
  struct data *cursor = NULL;
  int count_pos;
  int count_meas;

  if (dbname[0] == '+') {
    read_multi(dbname+1, top, prunings);
    return;
  }

  /* ordinary case, read a single file */

  in = fopen(dbname, "r");
  if (!in) {
    fprintf(stderr, "Could not open %s\n", dbname);
    exit(2);
  }

  lineno = 0;
  count_pos = 0;
  count_meas = 0;
  while (fgets(buffer, sizeof(buffer), in)) {
    ++lineno;
    if (buffer[0] == ':') {
      int xi, yi;
      if (sscanf(buffer+1, "%d %d", &xi, &yi) != 2) {
        fprintf(stderr, "Line %d corrupt\n", lineno);
        exit(2);
      }
      cursor = lookup_spatial(top, xi, yi);
      count_pos++;
    } else if (!is_blank(buffer)) {
      int n, gen, lac, cid, timeframe;
      char *p;
      int i;
      int total;
      int asus[32];

      if (!cursor) {
        fprintf(stderr, "Data line with no earlier location, bailing at line %d\n", lineno);
        exit (2);
      }
      if (sscanf(buffer, "%d%d%d%d%n", &gen, &timeframe, &lac, &cid, &n) != 4) {
        fprintf(stderr, "Parse error at line %d\n", lineno);
        exit(2);
      }
      if ((gen < 2) || (gen > 3)) {
        fprintf(stderr, "gen (%d) can only be 2 or 3 at line %d\n", gen, lineno);
        exit(2);
      }
      p = buffer + n;
      while (isspace(*p)) p++;
      memset(asus, 0, sizeof(asus));
      i = 0;
      total = 0;
#define FREEZE do { asus[i++] = total; count_meas += total; total = 0; } while (0)
#define FREEZE2 do { if (total > 0) { asus[i++] = total; count_meas += total; } total = 0; } while (0)
      while (*p) {
        switch (*p) {
          case 'a': FREEZE2; i +=  0; break;
          case 'b': FREEZE2; i +=  1; break;
          case 'c': FREEZE2; i +=  2; break;
          case 'd': FREEZE2; i +=  3; break;
          case 'e': FREEZE2; i +=  4; break;
          case 'f': FREEZE2; i +=  5; break;
          case 'g': FREEZE2; i +=  6; break;
          case 'h': FREEZE2; i +=  7; break;
          case 'i': FREEZE2; i +=  8; break;
          case 'j': FREEZE2; i +=  9; break;
          case 'k': FREEZE2; i += 10; break;
          case 'l': FREEZE2; i += 11; break;
          case 'm': FREEZE2; i += 12; break;
          case 'n': FREEZE2; i += 13; break;
          case 'o': FREEZE2; i += 14; break;
          case 'p': FREEZE2; i += 15; break;
          case 'q': FREEZE2; i += 16; break;
          case 'r': FREEZE2; i += 17; break;
          case 's': FREEZE2; i += 18; break;
          case 't': FREEZE2; i += 19; break;
          case 'u': FREEZE2; i += 20; break;
          case 'v': FREEZE2; i += 21; break;
          case 'w': FREEZE2; i += 22; break;
          case 'x': FREEZE2; i += 23; break;
          case 'y': FREEZE2; i += 24; break;
          case 'z': FREEZE2; i += 25; break;
          case 'A': FREEZE2; i += 26; break;
          case 'B': FREEZE2; i += 27; break;
          case 'C': FREEZE2; i += 28; break;
          case 'D': FREEZE2; i += 29; break;
          case 'E': FREEZE2; i += 30; break;
          case 'F': FREEZE2; i += 31; break;

          case ',':
          case '\n':
            FREEZE;
            break;
          case '0':
          case '1':
          case '2':
          case '3':
          case '4':
          case '5':
          case '6':
          case '7':
          case '8':
          case '9':
            total = (10*total) + (*p - '0');
            break;
          default:
            fprintf(stderr, "Parse error at line %d, char code is 0x%02x\n", lineno, (int) *p);
        }
        p++;
      }
      if (!prunings || !prune_this(prunings, lac, cid, timeframe)) {
        insert_asus(cursor, gen, lac, cid, timeframe, asus);
      }
    }

  }

  fclose(in);
  fprintf(stderr, "Read in data for %d positions, %d measurements\n",
      count_pos, count_meas);
}
/*}}}*/

/* vim:et:sts=2:sw=2:ht=2
 * */
