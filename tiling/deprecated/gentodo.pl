#!/usr/bin/perl

########################################################################
# Copyright (c) 2012, 2013 Richard P. Curnow
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

#

use strict;
use Digest::MD5 qw(md5_hex);

my $rec = { };

my $zb = 28;
my $nb = (2**$zb);

sub error {
    die "perl gentodo.pl <zoom> <[eo][eo]>";
}

my $zoom = shift or error();
my $eo = shift or error();

my $xodd = undef;
my $yodd = undef;
if    ($eo =~ /^ee$/) { $xodd = 0; $yodd = 0; }
elsif ($eo =~ /^eo$/) { $xodd = 0; $yodd = 1; }
elsif ($eo =~ /^oe$/) { $xodd = 1; $yodd = 0; }
elsif ($eo =~ /^oo$/) { $xodd = 1; $yodd = 1; }
else {
    die "Even/odd parameter must be ee, eo, oe or oo";
}

my $file = "level_".$zoom.".dat";
unless (-f $file) {
    die "$file does not exist : run 'cstum tiles' first";
}

my $cmdhex_name = "cmdhex/cmdhex_todo_".$eo.".dat";
my $tile_family = "mapnik";

my $bothcol    = "#ff000040";
my $only2gcol  = "#0000ff40";
my $only3gcol  = "#c0c00040";

my $bothcol2    = "#80000080";
my $only2gcol2  = "#00008080";
my $only3gcol2  = "#60600080";

############## LOAD UTILITIES #############

my $path = $0;
my $prefix = $path;
$prefix =~ s{[^/]+$}{};

do $prefix.'common.pl';
do $prefix.'drawing.pl';

############# PROCESS DATA ##########
#

my $xmin = $nb;
my $xmax = 0;
my $ymin = $nb;
my $ymax = 0;

# Flags for 2G, 3G and everything

my $ts = ($zb - $zoom);
my $grain = 4; # 16x16 subtiles
my $sts = ($zb - ($zoom + $grain));
my $nst = (1<<$grain);
my $nst1 = $nst - 1;
my $st_scale = 8 - $grain;
my $offset = 1 << (7 - $grain);
my $sub_mask = $nst - 1;
my $order = 0;

my $cmdhex = read_cmdhex($cmdhex_name);
my $limit_file = "./limits.txt";
my $limits = read_limits($limit_file);

unless (-d "logs") {
    mkdir "logs";
}
my $logfile = "logs/age3g_".$zoom."_".$eo.".dat";
open my $log, ">", $logfile;

open my $in, "<", $file;
file_loop:
while (<$in>) {
    last file_loop unless (m{^\<\<\s*(\d+)\s+(\d+)});
    my $tx = $1;
    my $ty = $2;
    my $line = undef;
    my $do_skip = 0;
    if (($xodd != ($tx & 1)) || ($yodd != ($ty & 1))) {
        $do_skip = 1;
    } else {
        my $ok = check_limits($limits, $zoom, $tx, $ty);
        if (!$ok) {
            $do_skip = 1;
        }
    }

    if ($do_skip) {
        while (1) {
            $line = <$in> or last file_loop;
            next file_loop if ($line =~ m{^\>\>});
        }
    }

    my $content = [ ];
    while (<$in>) {
        chomp;
        last if (m{^\>\>});
        next if (m{^\*[23]}); # scoring for whether to expand tile
        my ($g, $sx, $sy, $decaday, $asu, $rank, $primary, @rest) = split /\s+/;
        # a record to keep
        $content->[$sx]->[$sy] |= ($g == 3) ? 2 : 1;
    }

    my $base_tile;
    $base_tile = prepare_base_tile($tile_family, $zoom, $tx, $ty);
    if (defined $base_tile) {
        my $out_d = ensure_hier("out", "todo", $zoom, $tx);
        die unless (-d $out_d);
        my $out = $out_d."/".$ty.".png";
        &draw_tile($base_tile, $out, $content, $tx, $ty, $log);
    }
}

# Generate tiles

write_cmdhex($cmdhex_name, $cmdhex);
close $log;
exit 0;

sub draw_tile {
    my ($in, $out, $flags, $xi, $yi, $log) = @_;
    my @command = ();

    for my $i (0..$nst1) {
        for my $j (0..$nst1) {
            # Bit 0 : has 2D
            # Bit 1 : has 3D
            # Bit 2 : draw size is 2 boxen
            if (defined $flags->[$i][$j]) {
                $flags->[$i][$j] |= (1<<2);
            }
        }
    }

    # Grouping
    for (my $i=0; $i<$nst; $i+=2) {
        for (my $j=0; $j<$nst; $j+=2) {
            if (($flags->[$i+1][$j]   == $flags->[$i][$j]) &&
                ($flags->[$i]  [$j+1] == $flags->[$i][$j]) &&
                ($flags->[$i+1][$j+1] == $flags->[$i][$j])) {
                $flags->[$i][$j] &= 3;
                $flags->[$i][$j] |= (2<<2);
                $flags->[$i+1][$j] = 0;
                $flags->[$i][$j+1] = 0;
                $flags->[$i+1][$j+1] = 0;
            }
        }
    }

    for (my $i=0; $i<$nst; $i+=4) {
        for (my $j=0; $j<$nst; $j+=4) {
            if ((($flags->[$i][$j] & (2<<2)) != 0) &&
                ($flags->[$i+2][$j]   == $flags->[$i][$j]) &&
                ($flags->[$i]  [$j+2] == $flags->[$i][$j]) &&
                ($flags->[$i+2][$j+2] == $flags->[$i][$j])) {
                $flags->[$i][$j] &= 3;
                $flags->[$i][$j] |= (4<<2);
                $flags->[$i+2][$j] = 0;
                $flags->[$i][$j+2] = 0;
                $flags->[$i+2][$j+2] = 0;
            }
        }
    }

    for (my $i=0; $i<$nst; $i+=8) {
        for (my $j=0; $j<$nst; $j+=8) {
            if ((($flags->[$i][$j] & (4<<2)) != 0) &&
                ($flags->[$i+4][$j]   == $flags->[$i][$j]) &&
                ($flags->[$i]  [$j+4] == $flags->[$i][$j]) &&
                ($flags->[$i+4][$j+4] == $flags->[$i][$j])) {
                $flags->[$i][$j] &= 3;
                $flags->[$i][$j] |= (8<<2);
                $flags->[$i+4][$j] = 0;
                $flags->[$i][$j+4] = 0;
                $flags->[$i+4][$j+4] = 0;
            }
        }
    }

    push @command, "convert", $in;
    push @command, "-strokewidth", "1";
    for my $i (0..$nst1) {
        for my $j (0..$nst1) {
            my $f = $flags->[$i][$j];

            my $size = 8 * ($f >> 2);
            my $xc = ($i << $st_scale) + $size;
            my $yc = ($j << $st_scale) + $size;
            $size--;

            if (($f & 3) == 3) { # both
                push @command, "-fill", $bothcol; # red
                push @command, "-stroke", $bothcol2;
                push @command, "-draw", draw_square($xc, $yc, $size);
            } elsif (($f & 3) == 1) { # no 3g, only 2g
                push @command, "-fill", $only2gcol; # blue
                push @command, "-stroke", $only2gcol2;
                push @command, "-draw", draw_square($xc, $yc, $size);
            } elsif (($f & 3) == 2) { # no 2g, only 3g
                push @command, "-fill", $only3gcol; # blue
                push @command, "-stroke", $only3gcol2;
                push @command, "-draw", draw_square($xc, $yc, $size);
            } else { # neither
            }
        }
    }

    push @command, "-font", "helvetica", "-pointsize", "13";
    push @command, "-fill", "white";
    push @command, "-undercolor", "#00008040";
    push @command, "-stroke", "none";
    push @command, "-gravity", "south";
    push @command, "-annotate", '0x0+0+0', $xi;
    push @command, "-gravity", "east";
    push @command, "-annotate", '270x270+10+0', $yi;

    ### push @command, "png8:".$out.".tile";
    push @command, "png:".$out.".tile";

    ### for my $i (0..$#command) {
    ###     printf STDERR "%s\n", $command[$i];
    ### }
    if (!check_cmdhex($cmdhex, $in, $out, \@command)) {
        print $log (join ' ', @command);
        print $log "\n";
        system @command;
    }
}

# vim:et:sts=4:ht=4:sw=4

