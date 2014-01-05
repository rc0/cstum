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

static void usage(const char *tool)
{
  fprintf(stderr,
      "Usage: %s <subcmd> [<options>] [<args>]\n"
      "Subcommands:\n" , tool);
  usage_cat();
  usage_grid();
  usage_tiles();
  usage_tilerange();
  usage_filter();
  usage_tpe();
  usage_tpr();
  usage_tcs();
  usage_tpcon();
  usage_goodness();
  usage_timecull();
  usage_cidlist();
  usage_overlay();
}

int main (int argc, char **argv)
{
  if (argc < 2) {
    usage(argv[0]);
    exit(0);
  }
  if (!strcmp(argv[1], "cat")) {
    exit(ui_cat(argc-2, argv+2));
  } else if (!strcmp(argv[1], "grid")) {
    exit(ui_grid(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tilerange")) {
    exit(ui_tilerange(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tiles")) {
    exit(ui_tiles(argc-2, argv+2));
  } else if (!strcmp(argv[1], "filter")) {
    exit(ui_filter(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tpe")) {
    exit(ui_tpe(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tpr")) {
    exit(ui_tpr(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tpcon")) {
    exit(ui_tpcon(argc-2, argv+2));
  } else if (!strcmp(argv[1], "tcs")) {
    exit(ui_tcs(argc-2, argv+2));
  } else if (!strcmp(argv[1], "goodness")) {
    exit(ui_goodness(argc-2, argv+2));
  } else if (!strcmp(argv[1], "timecull")) {
    exit(ui_timecull(argc-2, argv+2));
  } else if (!strcmp(argv[1], "cidlist")) {
    exit(ui_cidlist(argc-2, argv+2));
  } else if (!strcmp(argv[1], "overlay")) {
    exit(ui_overlay(argc-2, argv+2));
  } else {
    usage(argv[0]);
    exit(0);
  }
}


