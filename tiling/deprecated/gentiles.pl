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

my $offline = 0;

my $rec = { };
my @all = ( );

my $zb = 28;
my $nb = (2**$zb);

sub error {
    die "perl gentiles.pl zoom <[eo][eo]> <2g|3g>";
}

my $zoom = shift or error();
my $eo = shift or error();
my $gen  = shift or error();

unless ($gen =~ /^([23])g$/) {
    die "Generation parameter must be 2g / 3g";
} else {
    $gen = $1; # just the number
}

my $file = "level_".$zoom.".dat";
unless (-f $file) {
    die "$file does not exist : run 'cstum tiles' first";
}

my $xodd = undef;
my $yodd = undef;
if    ($eo =~ /^ee$/) { $xodd = 0; $yodd = 0; }
elsif ($eo =~ /^eo$/) { $xodd = 0; $yodd = 1; }
elsif ($eo =~ /^oe$/) { $xodd = 1; $yodd = 0; }
elsif ($eo =~ /^oo$/) { $xodd = 1; $yodd = 1; }
else {
    die "Even/odd parameter must be ee, eo, oe or oo";
}

my $cmdhex_name = "cmdhex/cmdhex_".$gen."g_".$eo.".dat";

my $tile_family = "tonerlite";

sub min_zoom_to_show_tower_direction  { return 11; }
sub min_zoom_to_show_cell_shapes      { return 13; }

sub min_zoom_to_show_tower_colours    { return 0; }
sub min_zoom_to_show_tower_positions  { return 0; }

# The next two need to be the same really?
sub min_zoom_to_show_unmapped_regions { return 0; }

############## LOAD UTILITIES #############

my $path = $0;
my $prefix = $path;
$prefix =~ s{[^/]+$}{};

do $prefix.'common.pl';
do $prefix.'drawing.pl';

############# PARSE TOWERS FILE ##########

# TODO : eventually ./
my $towers_file = "towers.dat";

my %cidcol = ();
my %towercol = ();
my $anon_colour = 0;
my $ambig_colour = -1;
my %cidshape = ();
my %cidtower = ();
my %angles = ();
my %tower_x = ();
my %tower_y = ();
my %tower_confirmed = ();
# For secondary display (i.e. estimated positions of known towers)
my %tower_x2 = ();
my %tower_y2 = ();

my @towers = read_towers($towers_file);
my %tower_by_name = ();

# assume a tower may have up to 6 IDs - 2 distinct sets of 3.
# don't understand why they are allocated like this, though!
my @shapes = ("penta_up", "hexagon", "penta_down", "penta_up", "hexagon", "penta_down");
for my $tower (@towers) {
    my $name = $tower->{name};
    $tower_by_name{$name} = $tower;

    my $col = $tower->{colour};
    if ($zoom < min_zoom_to_show_tower_colours) {
        $col = $ambig_colour;
    }

    die "Colour undefined for tower $name" unless (defined $col);
    $towercol{$name} = $col;
    ### my @lacs = sort keys %{$tower->{lacs}};
    my @cids = @{$tower->{cids}};
    for my $xcid (@cids) {
        $xcid =~ m{^([A-Z])(\d+)$};
        my $network = $1;
        my $cid = $2;
        unless (defined $tower->{lacs}{$network}) {
            my $cap = $tower->{tag2}."/".$tower->{tag3};
            die "No match for network $network on CID $cid : $cap";
        }
        my @lacs = @{$tower->{lacs}{$network}};

        for my $lac (@lacs) {
            my $lc = $lac."+".$cid;
            $cidcol{$lc} = $col;
            $cidshape{$lc} = "circle";
            $cidtower{$lc} = $name;
            if (defined $tower->{angles}{$xcid}) {
                $angles{$lc} = $tower->{angles}{$xcid};
            }
        }
    }

    my $posn = $tower->{observed};
    if (defined $posn) {
        ($tower_x{$name}, $tower_y{$name}) = &validated_posn($posn);
        $tower_confirmed{$name} = 1;

        # Generate secondary position - the estimated one
        $posn = $tower->{estimated};
        if (defined $posn) {
            ($tower_x2{$name}, $tower_y2{$name}) = &validated_posn($posn);

        }

    } else {
        $posn = $tower->{estimated};
        if (defined $posn) {
            ($tower_x{$name}, $tower_y{$name}) = &validated_posn($posn);
            $tower_confirmed{$name} = 0;
        }
    }

}

############# PROCESS DATA ##########
#

my %maxpow = ();
my $xmin = $nb;
my $xmax = 0;
my $ymin = $nb;
my $ymax = 0;

my %tiles = ();

# Table of tiles to render : all tiles containing data plus neighbouring ones
my %all2_tiles = ();

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

# index by tilex->tiley
my %tower_marks = ();

my $mark_offset = 1 << $sts;
for my $name (sort {$a cmp $b} keys %tower_x) {
    # Force caption onto the line below the tower position.  Doing it here
    # means it's easy to shift it onto the tile below, if that's where it needs
    # to be.
    my $ty = $tower_y{$name} + $mark_offset;
    my $tile_x = $tower_x{$name} >> $ts;
    my $tile_y = $ty >> $ts;
    my $sub_x = ($tower_x{$name} >> $sts) & $sub_mask;
    my $sub_y = ($ty >> $sts) & $sub_mask;
    my $rec = { sub_x => $sub_x, sub_y => $sub_y, name => $name };
    #$tile_x, $tile_y, $sub_x, $sub_y, $name;
    push @{$tower_marks{$tile_x}->{$tile_y}}, $rec;
}

my %tile_towers = ();
my %tile_towers2 = (); # 2ary estimated position
my %tower_prox = ();

if ($zoom >= min_zoom_to_show_tower_positions) {
    for my $name (sort {$a cmp $b} keys %tower_x) {
        my $tile_x = $tower_x{$name} >> $ts;
        my $tile_y = $tower_y{$name} >> $ts;
        my $sub_x = ($tower_x{$name} >> $sts) & $sub_mask;
        my $sub_y = ($tower_y{$name} >> $sts) & $sub_mask;
        my $tag = ($gen == 2) ? $tower_by_name{$name}->{tag2} : $tower_by_name{$name}->{tag3};
        next unless ($tag);

        if (defined $tile_towers{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y]) {
            $tile_towers{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y] = "MULTI";
        } else {
            $tile_towers{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y] = $name;
        }
        add_quad($tower_x{$name}, $tower_y{$name});
        add_prox($tile_x, $tile_y, $sub_x, $sub_y);
    }

    # Process 2ndary positions (estimates)
    for my $name (sort {$a cmp $b} keys %tower_x2) {
        my $tile_x = $tower_x2{$name} >> $ts;
        my $tile_y = $tower_y2{$name} >> $ts;
        my $sub_x = ($tower_x2{$name} >> $sts) & $sub_mask;
        my $sub_y = ($tower_y2{$name} >> $sts) & $sub_mask;
        my $tag = ($gen == 2) ? $tower_by_name{$name}->{tag2} : $tower_by_name{$name}->{tag3};
        next unless ($tag);

        if (defined $tile_towers2{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y]) {
            $tile_towers2{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y] = "MULTI";
        } else {
            $tile_towers2{$tile_x}->{$tile_y}->[$sub_x]->[$sub_y] = $name;
        }
    }
}


unless (-d "logs") {
    mkdir "logs";
}
my $logfile = "logs/".$gen."g_".$zoom."_".$eo.".dat";
open my $log, ">", $logfile;
# TODO : deal later with how to force the tiles surrounding the active ones

my @included = ();
my @excluded = ();

open my $in, "<", $file;
file_loop:
while (<$in>) {
    last file_loop unless (m{^\<\<\s*(\d+)\s+(\d+)});
    my $tx = $1;
    my $ty = $2;

    my $line = <$in>;
    $line =~ m{^\*2 +(\d+) +(\d+)} or die "Unmatched *2 line";
    my ($tile_score2, $max_score2) = ($1, $2);
    $line = <$in>;
    $line =~ m{^\*3 +(\d+) +(\d+)} or die "Unmatched *3 line";
    my ($tile_score3, $max_score3) = ($1, $2);

    my ($tile_score, $max_score);
    if ($gen == 2) {
        ($tile_score, $max_score) = ($tile_score2, $max_score2);
    } else {
        ($tile_score, $max_score) = ($tile_score3, $max_score3);
    }

    my $line = undef;
    my $do_skip = 0;
    if (($xodd != ($tx & 1)) || ($yodd != ($ty & 1))) {
        $do_skip = 1;
    } else {
        my $ok = check_limits($limits, $zoom, $tx, $ty);
        if (!$ok) {
            push @excluded, [$tile_score, $max_score, $tx, $ty];
            $do_skip = 1;
        } else {
            push @included, [$tile_score, $max_score, $tx, $ty];
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
        my ($g, $sx, $sy, $decaday, $asu, $rank, $primary, @rest) = split /\s+/;
        next unless ($g == $gen);
        # a record to keep
        $primary =~ m{(\d+)/(\d+):(\d+)};
        my $primary_lac = $1;
        my $primary_cid = $2;
        $content->[$sx]->[$sy]->{lacs} = [ $primary_lac ];
        $content->[$sx]->[$sy]->{cids} = [ $primary_cid ];
        $content->[$sx]->[$sy]->{lengths} = [ 8.0 ];
        $content->[$sx]->[$sy]->{asu} = $asu;
        for my $other (@rest) {
            $other =~ m{(\d+)/(\d+):(\d+)};
            my $other_lac = $1;
            my $other_cid = $2;
            my $other_occ = $3;
            if ($other_occ > 66) {
                push @{$content->[$sx]->[$sy]->{lacs}}, $other_lac;
                push @{$content->[$sx]->[$sy]->{cids}}, $other_cid;
                push @{$content->[$sx]->[$sy]->{lengths}}, 8.0;
            } elsif ($other_occ > 33) {
                push @{$content->[$sx]->[$sy]->{lacs}}, $other_lac;
                push @{$content->[$sx]->[$sy]->{cids}}, $other_cid;
                push @{$content->[$sx]->[$sy]->{lengths}}, 5.0;
            } elsif ($other_occ > 16) {
                push @{$content->[$sx]->[$sy]->{lacs}}, $other_lac;
                push @{$content->[$sx]->[$sy]->{cids}}, $other_cid;
                push @{$content->[$sx]->[$sy]->{lengths}}, 3.0;
            }
        }
    }

    my $base_tile;
    $base_tile = prepare_base_tile($tile_family, $zoom, $tx, $ty);
    if (defined $base_tile) {
        my $out_d = ensure_hier("out", $gen."g", $zoom, $tx);
        die unless (-d $out_d);
        my $out = $out_d."/".$ty.".png";
        &draw_tile($base_tile, $out, $content, $tx, $ty, $log);
    }
    delete $all2_tiles{$tx}->{$ty};
}

# Hoover up the tiles that only have towers in them.  Eventually can extend to
# pick up tiles bordering the ones we actually rendered. 
for my $tx (keys %all2_tiles) {
    for my $ty (keys %{$all2_tiles{$tx}}) {
        if (($xodd == ($tx & 1)) && ($yodd == ($ty & 1))) {
            if (check_limits($limits, $zoom, $tx, $ty)) {
                my $base_tile;
                $base_tile = prepare_base_tile($tile_family, $zoom, $tx, $ty);
                if (defined $base_tile) {
                    my $out_d = ensure_hier("out", $gen."g", $zoom, $tx);
                    die unless (-d $out_d);
                    my $out = $out_d."/".$ty.".png";
                    &draw_tile($base_tile, $out, [ ], $tx, $ty, $log);
                }
            }
        }
    }
}

write_cmdhex($cmdhex_name, $cmdhex);
close $log;

open my $excluded, ">", "logs/excluded_".$gen."g_".$zoom."_".$eo.".dat";
@excluded = sort {$b->[0] <=> $a->[0] or $b->[1] <=> $a->[1]} @excluded;
my $count = 0;
for my $exc (@excluded) {
    &out_incexc($excluded, $exc);
    $count++;
}
close $excluded;

open my $included, ">", "logs/included_".$gen."g_".$zoom."_".$eo.".dat";
@included = sort {$a->[0] <=> $b->[0] or $a->[1] <=> $b->[1]} @included;
my $count = 0;
for my $inc (@included) {
    &out_incexc($included, $inc);
    $count++;
}
close $included;

exit 0;

sub validated_posn {
    my $posn = $_[0];
    if (($posn->{x} =~ /^\d+$/) && ($posn->{y} =~ /^\d+/)) {
        if ($posn->{x} == 0) {
            die "Tower has zeroX";
        }
        if ($posn->{y} == 0) {
            die "Tower has zeroY";
        }
        return ($posn->{x}, $posn->{y});
    } else {
        die "Tower has non-integer X or Y";
    }
}

sub out_incexc {
    my ($fh, $d) = @_;
    printf $fh "%2d %6d %6d : %6d %6d", $zoom, $d->[0], $d->[1], $d->[2], $d->[3];
    my $z = $zoom;
    my $x = $d->[2];
    my $y = $d->[3];
    while ($z > 12) {
        $z--;
        my $x0 = ($x & 1);
        my $y0 = ($y & 1);
        my $qq = qw/NW NE SW SE/ [$x0 + $y0 + $y0];
        $x >>= 1;
        $y >>= 1;
        my $flag = " ";
        if (-f "out/".$gen."g/".$z."/".$x."/".$y.".png.tile") {
            $flag = "*";
        }
        printf $fh "  %d:%d,%d%s%s", $z, $x, $y, $qq, $flag;
    }
    print $fh "\n";
}

sub add_quad {
    my ($x, $y) = @_;
    # Pick up the tile containing the point and the 3 tiles around it,
    # bordering the sides and corner closest to the point within its own tile
    my $x0 = (($x >> ($ts - 1)) - 1) >> 1;
    my $y0 = (($y >> ($ts - 1)) - 1) >> 1;
    my $x1 = $x0 + 1;
    my $y1 = $y0 + 1;
    $all2_tiles{$x0}->{$y0} = 1;
    $all2_tiles{$x0}->{$y1} = 1;
    $all2_tiles{$x1}->{$y0} = 1;
    $all2_tiles{$x1}->{$y1} = 1;
}

sub add_prox {
    my ($tx, $ty, $sx, $sy) = @_;
    my $x = ($tx << 4) + $sx;
    my $y = ($ty << 4) + $sy;
    for my $dx (-1,0,1) {
        my $X = $x + $dx;
        for my $dy (-1,0,1) {
            unless (($dx | $dy) == 0) {
                my $Y = $y + $dy;
                my $tx0 = $X >> 4;
                my $sx0 = $X & 15;
                my $ty0 = $Y >> 4;
                my $sy0 = $Y & 15;
                $tower_prox{$tx0}->{$ty0}->[$sx0]->[$sy0] = 1;
            }
        }
    }
}

sub draw_tile {
    my ($in, $out, $tr, $xi, $yi, $log) = @_;
    my @command = ();

    # $phase[n]{fill}{stroke}{strokewidth} is an array of draw commands
    my @phase = ();

    # map of which subtiles have no data for them
    my @missing = ();

    push @command, "convert", $in;
    for my $i (0..$nst1) {
        for my $j (0..$nst1) {
            if (defined $tr->[$i][$j]) {
                my $str = $tr->[$i][$j];
                my $xc = ($i << $st_scale) + $offset;
                my $yc = ($j << $st_scale) + $offset;
                my $xtile = ($xi << $ts);
                my $xraw = $xtile + ($i << $sts) + (1 << ($sts - 1));
                my $ytile = ($yi << $ts);
                my $yraw = $ytile + ($j << $sts) + (1 << ($sts - 1));
                my $lacs = $str->{lacs};
                my $cids = $str->{cids};
                my $lengths = $str->{lengths};
                my $asu = $str->{asu};
                my $size = get_size($asu);
                my $shape = "circle";
                my $width = 1;
                my $density = "99";
                my $unmapped_density = "cc";
                if ($cids->[0] > 0) {
                    my $lc = $lacs->[0]."+".$cids->[0];
                    if (defined $cidshape{$lc}) {
                        $shape = $cidshape{$lc};
                    } else {
                        $shape = "unmapped";
                    }
                    if ($shape eq "circle") {
                        my $angle = $angles{$lc};
                        my $sc = mapcol($cidcol{$lc});
                        if (defined $angle) {
                            my ($arg1, $arg2) = draw_sector($xc, $yc, $size, $angle);
                            push @{$phase[0]{$sc.$density}{"none"}{"1"}}, $arg2;
                            push @{$phase[1]{"none"}{"black"}{$width}}, $arg1;
                        } else {
                            my ($arg) = draw_circle($xc, $yc, $size);
                            push @{$phase[0]{$sc.$density}{"black"}{$width}}, $arg;
                        }
                    } elsif ($shape eq "unmapped") {
                        # Replace the rendering
                        my $sc = mapcol($anon_colour);
                        my $arg = draw_circle($xc, $yc, $size);
                        push @{$phase[0]{"none"}{$sc.$unmapped_density}{2}}, $arg;
                    } else {
                        die;
                    }

                    my $towers = cids_to_towers($lacs, $cids, $lengths);
                    for my $name (sort {$a cmp $b} keys %$towers) {
                        if ($zoom >= min_zoom_to_show_tower_direction) {
                            # Try to draw short line from sub-tile centre towards estimated tower location
                            my $tx = $tower_x{$name};
                            my $ty = $tower_y{$name};
                            if ((defined $tx) && (defined $ty)) {
                                my $dx = $tx - $xraw;
                                my $dy = $ty - $yraw;
                                my $d = sqrt ($dx**2 + $dy**2);
                                if ($d > 0) {
                                    my $length = $towers->{$name};
                                    my $zx = int(0.5 + $dx * $length / $d);
                                    my $zy = int(0.5 + $dy * $length / $d);
                                    my $arg = sprintf "line %d,%d %d,%d", $xc, $yc, $xc+$zx, $yc+$zy;
                                    push @{$phase[2]{"none"}{"black"}{$width}}, $arg;
                                }
                            }
                        }
                    }
                } elsif ($lacs->[0] == 0) {
                    # Emergency calls only
                    my $sc = mapcol($anon_colour);
                    my $arg = draw_circle($xc, $yc, $size);
                    push @{$phase[0]{$sc.$density}{"black"}{$width}}, $arg;

                } else {
                    # LAC=65535, totally null coverage
                    my @steps = draw_gate($xc, $yc);
                    push @{$phase[0]{"none"}{mapcol($anon_colour)}{$width}}, @steps;
                }
            } else {
                $missing[$i][$j] = 1;
            }
        }
    }

    push @command, generate_drawing(\@phase, $xi, $yi);

    # Underlay the transparent 'wash' over the missing areas before rendering
    # the towers and text captions
    push @command, make_args_for_missing(@missing);

    # Draw tower(s)
    for my $sx (0..15) {
        for my $sy (0..15) {
            my $name = $tile_towers{$xi}->{$yi}->[$sx]->[$sy];
            next unless (defined $name);
            my $xc = ($sx << $st_scale) + $offset;
            my $yc = ($sy << $st_scale) + $offset;
            if ($name eq "MULTI") {
                my $synthetic_size = get_size(9);
                push @command, "-fill", mapcol($ambig_colour)."80";
                push @command, "-strokewidth", "1";
                push @command, "-stroke", "black";
                push @command, "-draw", draw_up($xc-4, $yc-4, $synthetic_size);
                push @command, "-draw", draw_up($xc+4, $yc-4, $synthetic_size);
                push @command, "-draw", draw_up($xc  , $yc+4, $synthetic_size);
            } else {
                my $arg = "";
                # EQUIVALENT NOW ??? next unless (defined $tower_net{$name});
                my $tag = ($gen == 2) ? $tower_by_name{$name}->{tag2} : $tower_by_name{$name}->{tag3};
                next unless (defined $tag);
                my $col = mapcol($towercol{$name} // $anon_colour);

                my $synthetic_size;
                my $stroke_width;
                my $dash_pattern = undef;

                if (defined $tower_prox{$xi}->{$yi}->[$sx]->[$sy]) {
                    $synthetic_size = get_size(18);
                    $stroke_width = 1;
                    $dash_pattern = "stroke-dasharray 1 1 ";
                } else {
                    $synthetic_size = get_size(29);
                    $stroke_width = 3;
                    $dash_pattern = "stroke-dasharray 3 3 ";
                }
                if ($tower_confirmed{$name} == 1) {
                    # use solid line then
                    $dash_pattern = "";
                }

                push @command, "-fill", mapcol($towercol{$name});
                push @command, "-strokewidth", $stroke_width;
                push @command, "-stroke", "black";
                $arg = draw_square($xc, $yc, $synthetic_size);
                push @command, "-draw", $dash_pattern.$arg;
                #printf STDERR "Drawing shape for tower %s in tile %d,%d st %d,%d x,y=%d,%d\n",
                #    $rec->{name}, $xi, $yi, $rec->{x}, $rec->{y}, $xc, $yc;
            }
        }
    }

    # Draw estimated tower positions
    for my $sx (0..15) {
        for my $sy (0..15) {
            my $name = $tile_towers2{$xi}->{$yi}->[$sx]->[$sy];
            next unless (defined $name);
            my $xc = ($sx << $st_scale) + $offset;
            my $yc = ($sy << $st_scale) + $offset;
            if ($name eq "MULTI") {
                next;
            } elsif (defined $tile_towers{$xi}->{$yi}->[$sx]->[$sy]) {
                next;
            } elsif (defined $tower_prox{$xi}->{$yi}->[$sx]->[$sy]) {
                next;
            } else {
                my $arg = "";
                my $tag = ($gen == 2) ? $tower_by_name{$name}->{tag2} : $tower_by_name{$name}->{tag3};
                next unless (defined $tag);
                my $col = mapcol($towercol{$name} // $anon_colour);

                my $synthetic_size = get_size(29);
                my $stroke_width = 2;
                my $dash_pattern = "stroke-dasharray 2 2 ";

                push @command, "-fill", mapcol($towercol{$name})."c0";
                push @command, "-strokewidth", $stroke_width;
                push @command, "-stroke", "#000000c0";
                $arg = draw_square($xc, $yc, $synthetic_size);
                push @command, "-draw", $dash_pattern.$arg;
                #printf STDERR "Drawing shape for tower %s in tile %d,%d st %d,%d x,y=%d,%d\n",
                #    $rec->{name}, $xi, $yi, $rec->{x}, $rec->{y}, $xc, $yc;
            }
        }
    }

    # Add display of tile X/Y coordinates at the edges
    push @command, "-font", "helvetica", "-pointsize", "13";
    push @command, "-fill", "white";
    push @command, "-undercolor", "#00008040";
    push @command, "-stroke", "none";
    push @command, "-gravity", "south";
    push @command, "-annotate", '0x0+0+0', $xi;
    push @command, "-gravity", "east";
    push @command, "-annotate", '270x270+10+0', $yi;

    # Add names on towers
    for my $sx (0..15) {
        for my $sy (0..15) {
            my $name = $tile_towers{$xi}->{$yi}->[$sx]->[$sy];
            next unless (defined $name);
            my $xc = ($sx << $st_scale) + $offset;
            my $yc = ($sy << $st_scale) + $offset;
            if ($name eq "MULTI") {
                next;
            } elsif (defined $tower_prox{$xi}->{$yi}->[$sx]->[$sy]) {
                next;
            } else {
                my $arg = "";
                # EQUIVALENT ??? next unless (defined $tower_net{$name}); # skip 3g towers if drawing 2g map etc
                my $tag = ($gen == 2) ? $tower_by_name{$name}->{tag2} : $tower_by_name{$name}->{tag3};
                next unless (defined $tag);
                my $col = mapcol($towercol{$name} // $anon_colour);
                $col = "#000000";
                my $text_width = 2 + 5*(length $tag);
                my $buffer = 2;
                my ($xl, $xr);
                $xl = $xc - ($text_width >> 1);
                $xr = $xc + ($text_width >> 1);
                if ($xl < $buffer) {
                  $xl = $buffer;
                }
                if ($xr > (255 - $buffer)) {
                  $xl -= ($xr - (255 - $buffer));
                }

                push @command, "-font", "helvetica", "-pointsize", "11";
                push @command, "-fill", $col;
                push @command, "-undercolor", "#eeeeee90";
                push @command, "-stroke", "none";
                push @command, "-gravity", "northwest";
                push @command, "-annotate", '0x0+'.$xl.'+'.($yc), $tag;
            }
        }
    }

    ### push @command, "png8:".$out.".tile";
    push @command, "png:".$out.".tile";

    #for my $i (0..$#command) {
    #    printf STDERR "%s\n", $command[$i];
    #}
    if (!check_cmdhex($cmdhex, $in, $out, \@command)) {
        print $log (join ' ', @command);
        print $log "\n";
        system @command;
    }
}

############# FOO ###########

sub generate_drawing {
    my ($phase, $x, $y) = @_;
    my @out = ();
    for my $i (0..$#$phase) {
        for my $fill (sort {$a cmp $b} keys %{$phase->[$i]}) {
            for my $stroke (sort {$a cmp $b} keys %{$phase->[$i]{$fill}}) {
                for my $width (sort {$a <=> $b} keys %{$phase->[$i]{$fill}{$stroke}}) {
                    my @args = @{$phase->[$i]{$fill}{$stroke}{$width}};

                    if (scalar @args > 0) {
                        push @out, "-fill", $fill;
                        push @out, "-stroke", $stroke;
                        push @out, "-strokewidth", $width;
                        for my $a (@args) {
                            push @out, "-draw", $a;
                        }
                    }
                }
            }
        }
    }
    return @out;
}

sub make_args_for_missing {
    my @result = ();
    my @missing = @_;
    if ($zoom >= min_zoom_to_show_unmapped_regions) {
        for my $lvl (0..($grain-1)) {
            my $step0 = 1<<$lvl;
            my $step1 = 2<<$lvl;
            my $count = 1<<$lvl;
            my $i = 0;
            while ($i < $nst) {
                my $j = 0;
                while ($j < $nst) {
                    if (($missing[$i][$j] == $count) &&
                        ($missing[$i+$step0][$j] == $count) &&
                        ($missing[$i][$j+$step0] == $count) &&
                        ($missing[$i+$step0][$j+$step0] == $count)) {
                        # all 4 subtiles missing at this level
                        $missing[$i][$j] = $count + $count;
                        $missing[$i+$step0][$j] = 0;
                        $missing[$i][$j+$step0] = 0;
                        $missing[$i+$step0][$j+$step0] = 0;
                    }
                    $j += $step1;
                }
                $i += $step1;
            }
        }

        for my $i (0..$nst1) {
            for my $j (0..$nst1) {
                if ($missing[$i][$j] > 0) {
                    my $count = $missing[$i][$j];
                    my $x0 = 1 + ($i << $st_scale);
                    my $y0 = 1 + ($j << $st_scale);
                    my $x1 = (($i + $count) << $st_scale) - 2;
                    my $y1 = (($j + $count) << $st_scale) - 2;
                    my $arg = sprintf "rectangle %d,%d %d,%d", $x0, $y0, $x1, $y1;
                    push @result, "-draw", $arg;
                }
            }
        }

        if (scalar @result > 0) {
            @result = ("-stroke", "#0000ff40", "-strokewidth", "1", "-fill", "#0000ff10", @result);
        }
    }

    return @result;
}

############# FOO ###########

sub cids_to_towers {
    my ($lacs, $cids, $lengths) = @_;
    my $result = { };
    for my $i (0..$#$cids) {
        my $lac = $lacs->[$i];
        my $cid = $cids->[$i];
        my $length = $lengths->[$i];
        my $lc = $lac."+".$cid;
        my $name = $cidtower{$lc};
        if (defined $name) {
            unless ((defined $result->{$name}) && ($result->{$name} > $length)) {
                $result->{$name} = $length;
            }
        }
    }
    return $result;
}

############# CONVERSIONS ##########

sub mapcol {
    my ($x) = @_;
    my @cols = (
        "#FF2C2C",
        "#00D000",
        "#8747FF",
        "#D97800",
        "#08D6D2",
        "#FF30F6",
        "#96A600",
        "#0E7EDD",
        "#FF9A8C",
        "#91E564",
        "#F98CFF",
        "#5E0857",
        "#5A2C00",
        "#084500",
        "#18256B",
    );

    die "Undefined colour" unless (defined $x);
    if ($x < 0) {
        return "#808080";
    }
    die "Colour $x out of range" unless ($x <= $#cols);
    return $cols[$x];
    ### my $s = sprintf "#%02X%02X%02X", $cols[3*$x], $cols[3*$x+1], $cols[3*$x+2];
    ### return $s;
}

# vim:et:sts=4:ht=4:sw=4:ts=4

