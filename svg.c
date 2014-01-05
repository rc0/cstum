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

/*
* Copyright (c) 2012, Richard P. Curnow
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*     * Redistributions of source code must retain the above copyright
*       notice, this list of conditions and the following disclaimer.
*     * Redistributions in binary form must reproduce the above copyright
*       notice, this list of conditions and the following disclaimer in the
*       documentation and/or other materials provided with the distribution.
*     * Neither the name of the <organization> nor the
*       names of its contributors may be used to endorse or promote products
*       derived from this software without specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
* ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
* WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
* DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
* DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
* (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
* LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
* ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
* (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
* SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
* */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>

#include "contour.h"

#define DEFAULT_BASETILES "../osm"

static FILE *out;
/* pngs are loaded into inkscape at 1 pixel per point and each tile is 256
 * pixels in the png */
static double scale = 256.0 ;
static double stroke_width = 1.0;

void start_svg(const char *basetiles, const char *name, int zoom, int xt0, int yt0, int nx, int ny)
{
  char temp[256];
  int i, j;
  out = fopen(name, "w");

  fprintf(out, "<?xml version=\"1.0\"?>\n");
  fprintf(out, "<svg\n");

  fprintf(out, "xmlns:svg=\"http://www.w3.org/2000/svg\"\n"
         "xmlns=\"http://www.w3.org/2000/svg\"\n"
         "xmlns:xlink=\"http://www.w3.org/1999/xlink\"\n"
         "id=\"svg2\"\n"
         "height=\"%d\"\n"
         "width=\"%d\"\n"
         "y=\"0.0000000\"\n"
         "x=\"0.0000000\"\n"
         "version=\"1.0\">\n", ny*256, nx*256);
  fprintf(out, "<defs\n"
         "id=\"defs3\" />\n");

#if 0
  fprintf(out, "<image x=\"0\" y=\"0\" xlink:href=\"file:///mnt/flash/optima/wgs/osm/uk12.png\" height=\"1024\" width=\"768\" />\n");
#endif
  for(i=0; i<nx; i++) {
    for (j=0; j<ny; j++) {
      struct stat sb;
      sprintf(temp, "%s/%d/%d/%d.png",
          basetiles ? basetiles : DEFAULT_BASETILES,
          zoom, xt0+i, yt0+j);
      if (stat(temp, &sb) < 0) {
        strcpy(temp, "./blank256.png");
      }
      fprintf(out, "<image x=\"%d\" y=\"%d\" \n"
          "xlink:href=\"%s\" height=\"256\" width=\"256\" />\n",
          256*i, 256*j, temp);
    }
  }

  fprintf(out, "<g id=\"layer1\">\n");
}

void emit_svg(double level, const char *colour, double width_scale, struct cline *lines)
{
  struct cline *line;

  for (line=lines; line; line=line->next) {
    struct cpoint *p;
    struct cpoint *fp;
    double minx, miny, maxx, maxy;
    double mhd;
    int first;
    fprintf(out, "<path style=\"fill:none;stroke:%s;stroke-width:%f;stroke-linecap:square;stroke-linejoin:miter;stroke-miterlimit:4.0;stroke-opacity:1.0\"\n",
        colour,
        stroke_width * width_scale);

    first = 1;
    fp = line->points;
    for (p=line->points; p; p=p->next) {
      double x = p->x;
      double y = p->y;
      if (first) {
        fprintf(out, "d=\"M %f,%f",
            scale * x, scale * y);
        first = 0;
        minx = maxx = x;
        miny = maxy = y;
      } else {
        fprintf(out, " L %f,%f",
            scale * x,
            scale * y);
        if (x < minx) minx = x;
        if (x > maxx) maxx = x;
        if (y < miny) miny = y;
        if (y > maxy) maxy = y;
      }
    }
    fprintf(out, "\" />\n");

    mhd = (maxx - minx) + (maxy - miny);
    if (0 /* mhd > 0.05 */) {
      double xc, yc;
      xc = fp->x;
      yc = fp->y;
      /* Draw caption on the line */
      fprintf(out, "<text style=\"font-size:10;font-style:normal;font-variant:normal;"
               "font-weight:bold;fill:%s;fill-opacity:1.0;stroke:none;"
               "font-family:Luxi Sans;text-anchor:left;writing-mode:lr-tb\"\n",
               colour);
      if (level < 1.0) {
        fprintf(out, "x=\"%f\" y=\"%f\">%.2f</text>\n",
            scale*xc,
            scale*yc, level);
      } else if (level < 10.0) {
        fprintf(out, "x=\"%f\" y=\"%f\">%.1f</text>\n",
            scale*xc,
            scale*yc, level);
      } else {
        fprintf(out, "x=\"%f\" y=\"%f\">%.0f</text>\n",
            scale*xc,
            scale*yc, level);
      }
    }

  }


  return;
}

void emit_reg_marks(double xoff)
{
  int i, j;
  double x, y;
  double half;
  half = scale / 32.0;
  for (i=0; i<=3; i++) {
    for (j=0; j<=4; j++) {
      x = xoff + scale * (double) i;
      y = scale * (double) j;
      fprintf(out, "<path style=\"fill:none;stroke:%s;stroke-width:%f;stroke-linecap:square;stroke-linejoin:miter;stroke-miterlimit:4.0;stroke-opacity:1.0\"\n",
        "#000",
        0.5 * stroke_width);
      fprintf(out, "d=\"M %f,%f L %f,%f M %f,%f L %f,%f\" />\n",
          x - half, y,
          x + half, y,
          x, y - half,
          x, y + half
          );
    }
  }
}

void svg_cross(const char *colour, double width_scale, double xx, double yy)
{
  double x0, x1, y0, y1;
  x0 = xx - 0.05;
  x1 = xx + 0.05;
  y0 = yy - 0.05;
  y1 = yy + 0.05;

  fprintf(out, "<path style=\"fill:none;stroke:%s;stroke-width:%f;stroke-linecap:square;stroke-linejoin:miter;stroke-miterlimit:4.0;stroke-opacity:1.0\"\n",
      colour,
      stroke_width * width_scale);
  fprintf(out, "d=\"M %f,%f", scale * x0, scale * yy);
  fprintf(out, " L %f,%f", scale * x1, scale * yy);
  fprintf(out, " M %f,%f", scale * xx, scale * y0);
  fprintf(out, " L %f,%f", scale * xx, scale * y1);
  fprintf(out, "\" />\n");
}

void finish_svg(void)
{
  fprintf(out, "</g>\n");
  fprintf(out, "</svg>\n");

}

/* vim:et:sts=2:sw=2:ht=2
 * */
