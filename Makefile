
########################################################################
# Copyright (c) 2012, 2013, 2014 Richard P. Curnow
# All rights reserved.
#
# Redistribution and use in source and binary forms, with or without
# modification, are permitted provided that the following conditions are
# met:
#     * Redistributions of source code must retain the above copyright
#       notice, this list of conditions and the following disclaimer.
#     * Redistributions in binary form must reproduce the above copyright
#       notice, this list of conditions and the following disclaimer in
#       the documentation and/or other materials provided with the
#       distribution.
#     * Neither the name of the <organization> nor the
#       names of its contributors may be used to endorse or promote
#       products derived from this software without specific prior written
#       permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
# IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
# TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
# PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
# BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
# SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
# INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
# CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
# ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
# THE POSSIBILITY OF SUCH DAMAGE.
########################################################################

CC := gcc
CFLAGS := -O2 -ffast-math -msse3 -mrecip -g -Wall
#CFLAGS := -g -Wall
LDFLAGS := -pg

OBJ := cstum.o
OBJ += read_db.o write_db.o util.o db.o
OBJ += ui_cat.o
OBJ += ui_grid.o
OBJ += ui_tiles.o
OBJ += ui_tilerange.o
OBJ += ui_filter.o
OBJ += ui_tpe.o
OBJ += ui_tpr.o
OBJ += ui_tcs.o
OBJ += ui_tpcon.o
OBJ += ui_goodness.o
OBJ += ui_timecull.o
OBJ += ui_cidlist.o
OBJ += ui_overlay.o
OBJ += ui_addlacs.o

OBJ += spatial2.o
OBJ += temporal3.o
OBJ += tile_walk.o
OBJ += summary.o
OBJ += towers2.o
OBJ += do_filter.o
OBJ += tpe.o
OBJ += tpr.o
#OBJ += tcs2.o
#OBJ += tcs3.o
OBJ += tcs4.o
OBJ += tpcon.o
OBJ += tpgen.o
OBJ += sort_finals.o
OBJ += resolve_towers.o
OBJ += chart.o
OBJ += contour.o
OBJ += svg.o
OBJ += goodness.o
OBJ += timecull.o
OBJ += cidlist.o
OBJ += cli.o
OBJ += correlate2.o
OBJ += overlay.o
OBJ += addlacs.o

all : cstum listover

cstum : $(OBJ)
	$(CC) $(CFLAGS) $(LDFLAGS) -o $@ $(OBJ) -lm

%.o : %.c tool.h cli.h
	$(CC) $(CFLAGS) -c $< -o $@

tp*.o : tpgen.h

tpgen.o sort_finals.o : tp_type.h

listover : listover.o
	$(CC) $(CFLAGS) -o $@ $^ -lm

listover.o : listover.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	-rm -f cstum *.o

# vim:noet:ht=8:sts=8:sw=8:ts=8
