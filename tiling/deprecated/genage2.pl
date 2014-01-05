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

# Generate maps showin the age of the 3G data - to make it easier to pick
# places to revisit that are getting stale.

# In this version, bleach out map squares according to newness.

use strict;
use Digest::MD5 qw(md5_hex);

my $offline = 0;

my $rec = { };
my @all = ( );

my $zb = 28;
my $nb = (2**$zb);

sub error {
    die "perl genage2.pl <zoom> <[eo][eo]>";
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

my $cmdhex_name = "cmdhex/cmdhex_age3g2_".$eo.".dat";
my $tile_family = "mapnik";

#my $fillcol = "#c0c00040";
#my $linecol = "#60600080";
#my $fillcol = "#bb550040";
#my $linecol = "#60200080";

my $ddy; # year since 2000
my $ddd; # 1/3-months since start of year (0..35)

setup_ddd();

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
my %d3 = ();

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
my $logfile = "logs/age3g2_".$zoom."_".$eo.".dat";
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
        my ($g, $sx, $sy, $decaday, $asu, $primary, @rest) = split /\s+/;
        # a record to keep
        if ($g == 3) {
          if (defined $content->[$sx]->[$sy]) {
            if ($decaday > $content->[$sx]->[$sy]) {
              $content->[$sx]->[$sy] = $decaday;
            }
          } else {
            $content->[$sx]->[$sy] = $decaday;
          }
        }
    }

    my $base_tile;
    $base_tile = prepare_base_tile($tile_family, $zoom, $tx, $ty);
    if (defined $base_tile) {
        my $out_d = ensure_hier("out", "age3g2", $zoom, $tx);
        die unless (-d $out_d);
        my $out = $out_d."/".$ty.".png";
        &draw_tile($base_tile, $out, $content, $tx, $ty, $log);
    }
}

write_cmdhex($cmdhex_name, $cmdhex);
close $log;

exit 0;

sub draw_tile {
    my ($in, $out, $d3, $xi, $yi, $log) = @_;
    my @command = ();

    push @command, "convert", $in;
    push @command, "-strokewidth", "1";
    push @command, "-stroke", "none";

    # Index by stroke colour
    my %commands = ();
    for my $i (0..$nst1) {
        for my $j (0..$nst1) {
            if (defined $d3->[$i][$j]) {
                $d3->[$i][$j] =~ m{(\d\d)(\d\d)(\d)} or die "Junk date format?";
                my $y = $1;
                my $d = 3*($2 - 1) + $3;
                my $age = 36*($ddy - $y) + ($ddd - $d);
                my $xc = ($i << $st_scale) + $offset;
                my $yc = ($j << $st_scale) + $offset;
                my $colour = undef;
                my @cmds = ();
                if ($age < 12) {
                    $colour = "#000000c0";
                    @cmds = draw_gate($xc, $yc);
                } elsif ($age < 24) {
                    $colour = "#000000c0";
                    @cmds = draw_semi_gate_45($xc, $yc);
                } elsif ($age < 36) {
                    $colour = "#606060c0";
                    @cmds = draw_semi_gate_45($xc, $yc);
                } else {
                    $colour = "#f00000c0";
                    @cmds = draw_semi_gate_45($xc, $yc);
                }
                push @{$commands{$colour}}, @cmds;
            }
        }
    }
    push @command, "-fill", "none";
    for my $colour (keys %commands) {
        my $col_commands = $commands{$colour};
        push @command, "-stroke", $colour;
        for my $cc (@$col_commands) {
            push @command, "-draw", $cc;
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


############# CMD MD5 CACHE ##########

sub setup_ddd{
    my @lt = localtime;
    my $d = $lt[3] / 10;
    if ($d == 3) { $d = 2; }
    my $m = $lt[4];
    $ddy = $lt[5] - 100;
    $ddd = 3*$m + $d;
}

# vim:et:sts=4:ht=4:sw=4


