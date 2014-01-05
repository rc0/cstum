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

/* List all the paths contained in the overlay file
 *
 * */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>

struct geo_limit {
  int active;
  double lat;
  double lon;
  double metres;
  double X0;
  double Y0;
  double X1;
  double Y1;
};

struct job {
  struct job *next;
  int z;
  int x;
  int y;
  uint32_t pos;
};

#define VERSION 2

static uint32_t read_ui32(FILE *in, off_t pos)
{
  uint8_t buffer[4];
  uint32_t result;
  fseek(in, pos, SEEK_SET);
  fread(buffer, 1, 4, in);
  /* Big-endian */
  result = (((uint32_t) buffer[0] << 24) |
      ((uint32_t) buffer[1] << 16) |
      ((uint32_t) buffer[2] << 8) |
      ((uint32_t) buffer[3]));
  return result;
}

static uint16_t read_ui16(FILE *in, off_t pos)
{
  uint8_t buffer[2];
  uint32_t result;
  fseek(in, pos, SEEK_SET);
  fread(buffer, 1, 2, in);
  /* Big-endian */
  result = (((uint16_t) buffer[0] << 8) |
      ((uint16_t) buffer[1]));
  return result;
}

static uint8_t read_ui8(FILE *in, off_t pos)
{
  uint8_t result;
  fseek(in, pos, SEEK_SET);
  fread(&result, 1, 1, in);
  return result;
}

static uint32_t expected_magic(void)
{
  uint32_t result;
  result = 0xfe000000;
  result |= (uint32_t) 'O' << 16;
  result |= (uint32_t) 'V' << 8;
  result |= VERSION;
  return result;
}

static int check_geo(int z, int x, int y, const struct geo_limit *geo)
{
  double scale, X0, Y0, X1, Y1;
  int xok, yok;
  if (!geo->active) return 1;
  scale = 1.0 / (double)(1<<z);
  X0 = (double)x * scale;
  X1 = X0 + scale; /* x+1 */
  Y0 = (double)y * scale;
  Y1 = Y0 + scale; /* x+1 */

  xok = ((X0 < geo->X1) && (geo->X0 < X1));
  yok = ((Y0 < geo->Y1) && (geo->Y0 < Y1));
  if (xok && yok) {
    return 1;
  } else {
    return 0;
  }
}

static void check_and_report(int g, int z, int x, int y, const struct geo_limit *geo)
{
  if (check_geo(z, x, y, geo)) {
    struct stat sb_orig, sb_gen;
    int orig_exists, gen_exists;
    char orig_buf[64], gen_buf[64];
    sprintf(orig_buf, "../osm/%d/%d/%d.png", z, x, y);
    orig_exists = (stat(orig_buf, &sb_orig) >= 0) ? 1 : 0;
    if (orig_exists) {
      sprintf(gen_buf, "tc/%dg/%d/%d/%d.png.tile", g, z, x, y);
      gen_exists = (stat(gen_buf, &sb_gen) >= 0) ? 1 : 0;
      if (!gen_exists) {
        printf("%dg_%d_%d_%d\n", g, z, x, y);
      }
      sprintf(gen_buf, "tc/m%dg/%d/%d/%d.png.tile", g, z, x, y);
      gen_exists = (stat(gen_buf, &sb_gen) >= 0) ? 1 : 0;
      if (!gen_exists) {
        printf("m%dg_%d_%d_%d\n", g, z, x, y);
      }
    }
  }
}

static void study(FILE *in, uint32_t pos0, int z0, int x0, int y0, const struct geo_limit *geo, int max_zoom)
{
  uint8_t flags;
  uint32_t offset;
  int wide;
  int count = 0;
  struct job *tail;
  struct job *nj, *j;
  static struct xytab {
    int flag;
    int xoff;
    int yoff;
  } xytab[4] = {
    {0x01, 0, 0},
    {0x02, 0, 1},
    {0x04, 1, 0},
    {0x08, 1, 1}
  };

  /* Nested function */
  void maybe_enqueue(uint32_t pos, int z, int x, int y) {
    struct job *nnj;
    if (z <= max_zoom) {
      nnj = malloc(sizeof(struct job));
      nnj->z = z;
      nnj->x = x;
      nnj->y = y;
      nnj->pos = pos;
      nnj->next = NULL;
      tail->next = nnj;
      tail = nnj;
    }
  }

  nj = malloc(sizeof(struct job));
  nj->z = nj->x = nj->y = 0;
  nj->pos = pos0;
  nj->next = NULL;
  j = tail = nj;

  /* Do scan breadth-first */
  do {
    int i;
    flags = read_ui8(in, j->pos);
    wide = (flags & 0x80) ? 1 : 0;
    count = 0;
    for (i=0; i<4; i++) {
      if (flags & xytab[i].flag) {
        offset = wide ? read_ui32(in, j->pos + 1 + (count<<2)) : read_ui16(in, j->pos + 1 + (count<<1));
        maybe_enqueue(j->pos - offset, j->z + 1, xytab[i].xoff + (j->x<<1), xytab[i].yoff + (j->y<<1));
        count++;
      }
    }
    if (flags & 0x10) {
      check_and_report(2, j->z, j->x, j->y, geo);
    }
    if (flags & 0x20) {
      check_and_report(3, j->z, j->x, j->y, geo);
    }

    nj = j->next;
    free(j);
    j = nj;
  } while (j);

}

static void scan_file(const char *filename, const struct geo_limit *geo, int max_zoom)
{
  FILE *in;
  uint32_t magic;
  struct stat sb;
  uint32_t length, pos, delta;

  in = fopen(filename, "rb");
  if (!in) {
    fprintf(stderr, "Could not open %s\n", filename);
    exit(1);
  }

  magic = read_ui32(in, 0);
  if (magic != expected_magic()) {
    fprintf(stderr, "%s does not have the expected format\n", filename);
    exit(1);
  }

  if (fstat(fileno(in), &sb) < 0) {
    fprintf(stderr, "Could not stat %s\n", filename);
    exit(1);
  }

  length = sb.st_size;
  pos = length - 4;
  delta = read_ui32(in, pos);
  pos -= delta;
  study(in, pos, 0, 0, 0, geo, max_zoom);

  fclose(in);
}

static void prepare_geo(struct geo_limit *geo)
{
  if (geo->active) {
    double t;
    double z;
    double X, Y;
    double distance;
    X = 0.5 + (geo->lon * (1.0/360.0));
    t = log(tan(0.5*geo->lat*(M_PI/180.0) + 0.25*M_PI));
    Y = 0.5 * (1.0 - t/M_PI);
    z = Y - 0.5;
    distance = geo->metres * (1.0 + 19.42*z*z + 74.22*z*z*z*z) / (2.0 * M_PI * 6378140.0);
    geo->X0 = X - distance;
    geo->X1 = X + distance;
    geo->Y0 = Y - distance;
    geo->Y1 = Y + distance;
  }
}

static void usage(char *argv0)
{
  fprintf(stderr, "Usage: %s [--near|-n <lat> <lon> <metres>] [--max|-m <maxzoom>] <overlay.db>\n",
      argv0);
}

int main (int argc, char **argv)
{
  char *filename = NULL;
  char *argv0 = *argv;
  struct geo_limit geo;
  int max_zoom = 16;

  memset(&geo, 0, sizeof(geo));

  /* This is only designed to be run from the directory for which
   * ../osm contains the base tiles, and
   * tc/... contains the 'already generated' tiles */

  ++argv;
  --argc;
  while (argc >= 1) {
    if (!strcmp(*argv, "-h") || !strcmp(*argv, "--help")) {
      usage(argv0);
      exit(0);
    }
    if (!strcmp(*argv, "-m") || !strcmp(*argv, "--max")) {
      if (argc < 2) {
        usage(argv0);
        exit(1);
      }
      max_zoom = atoi(argv[1]);
      argc -= 2;
      argv += 2;
    } else if (!strcmp(*argv, "-n") || !strcmp(*argv, "--near")) {
      geo.active = 1;
      if (argc < 4) {
        usage(argv0);
        exit(1);
      }
      geo.lat = atof(argv[1]);
      geo.lon = atof(argv[2]);
      geo.metres = atof(argv[3]);
      argc -= 4;
      argv += 4;
    } else if (argv[0][0] == '-') {
      fprintf(stderr, "Unrecognized option %s\n", *argv);
      usage(argv0);
      exit(1);
    } else {
      filename = *argv;
      break;
    }
  }

  if (!filename) {
    usage(argv0);
    exit(1);
  }

  prepare_geo(&geo);
  scan_file(filename, &geo, max_zoom);
  return 0;

}

/* vim:et:sts=2:sw=2:ht=2
 * */

