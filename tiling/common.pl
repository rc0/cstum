

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

use Math::Trig;

my $zb = 28;
my $scale = 1.0 * (1<<$zb);

sub get_master_shift {
    return $zb;
}

sub get_master_scale {
    return $scale;
}

sub ll_to_frac {
    my ($lat, $lon) = @_;
    my $x = deg2rad($lon);
    my $y = log(tan(deg2rad($lat)) + sec(deg2rad($lat)));
    my $X = (1.0 + $x/pi)/2.0;
    my $Y = (1.0 - $y/pi)/2.0;
    return ($X, $Y);
}

sub frac_to_ll {
    my ($X, $Y) = @_;
    my $lon = rad2deg(pi * ((2.0 * $X ) - 1.0));
    my $lat = rad2deg(atan(sinh(pi * (1.0 - (2.0 * $Y )))));
    return ($lat, $lon);
}

# To read the >=aug2012 format.

sub read_towers {
    my ($filename) = @_;
    open my $in, "<", $filename or die "Cannot open $filename to read from";
    my $tow = undef;
    my $number = 1;
    my @towers = ();
    while (<$in>) {
        chomp;
        s/\#.*$//;              # drop comments
        if (/^\s*$/) {
            next;
        } elsif (/^=\s*(.+)/) {
            $tow = { rest => $1, number => $number };
            $number++;
            push @towers, $tow;
        } elsif (/^\%(.*)$/) {
            push @{$tow->{comments}}, $1;
            next; # chuck away the embedded notes
        } elsif (/^\+/) {
            $tow->{raw_quadcode} = $_;
            next; # chuck away the hexcode line
        } elsif (/^\!\s*(.+)/) {
            $tow->{colour} = $1;
        } elsif (/^\^2\s+(.+)/) {
            $tow->{tag2} = $1;
        } elsif (/^\^3\s+(.+)/) {
            $tow->{tag3} = $1;
        } elsif (/^\@\@\s+(\d+)\s+(\d+)/) {
            $tow->{observed}->{x} = $1;
            $tow->{observed}->{y} = $2;
            $tow->{raw_observed} = $_;
        } elsif (/^\@\?\s+(\d+)\s+(\d+)/) {
            $tow->{estimated}->{x} = $1;
            $tow->{estimated}->{y} = $2;
            $tow->{raw_estimated} = $_;
###        } elsif (/^\<(\d+)\>\s*(.*?)\s*$/) {
###            my $lac = $1;
###            my @cids = split /\s+/, $2;
###            for my $cid (@cids) {
###                $tow->{lacs}{$lac}{$cid} = 1;
###            }
        } elsif (/^\<\>\s*(.*?)\s*$/) {
            my $lacs = $1;
            my @lacs = split /\s+/, $lacs;
            for my $lac (@lacs) {
                $lac =~ m{^([A-Z])(\d+)$} or die "Unmatched LAC $lac";
                my $network = $1;
                my $number = $2;
                push @{$tow->{lacs}{$network}}, $number;
            }
        } elsif (/^\*\s+(.*)/) {
            push @{$tow->{raw_groups}}, $_;
            my $bearing = undef;
            my $foo = $1;
            $foo =~ s{//.*$}{}; # remove bearing data
            if ($foo =~ s{%(\d+)}{}) { # remove bearing data
                $bearing = $1;
            }
            my $np = 0;
            my %offsets = ();
            while (length $foo > 0) {
                if ($foo =~ m{^\s*\[(.*?)?\]\s*(.*)$}) {
                    my $cids = $1;
                    my $rest = $2;
                    if (defined $cids) {
                        $cids =~ s/^ +//;
                        $cids =~ s/ +$//;
                        my @cids = split /\s+/, $cids;
                        if (length @cids > 0) {
                            for my $cid (@cids) {
                                unless ($cid =~ m{^[A-Z]\d+$}) {
                                    die "Unmatched CID $cid";
                                }
                                $offsets{$cid} = $np;
                            }
                        }
                    }
                    $foo = $rest;
                    ++$np;
                } else {
                    die "Unparsed panels $foo";
                }
            }
            my $incr = 360 / $np;
            if (defined $bearing) {
                for my $cid (keys %offsets) {
                    my $this_bearing = $bearing + $incr * $offsets{$cid};
                    # Imagemagic angles are N=270, E=0, S=90, W=180
                    my $angle = $this_bearing - 90;
                    while ($angle < 0) { $angle += 360.0; }
                    while ($angle >= 360) { $angle -= 360.0; }
                    $tow->{angles}{$cid} = $angle;
                }
            }

            push @{$tow->{cids}}, (keys %offsets);
        } else {
            die "Unparsed line <$_> in $filename";
        }
    }
    close $in;

    my $count = 0;
    for my $tow (@towers) {
        $tow->{name} = ($tow->{tag2} // '').'+'.($tow->{tag3} // '').":".($count++);
    }
    return @towers;
}

############# CMD MD5 CACHE ##########

sub read_cmdhex {
    my ($filename) = @_;
    my $cmdhex = { };
    if (-f $filename) {
        open my $in, "<", $filename;
        while (<$in>) {
            chomp;
            my ($tile, $hash) = split /\s+/;
            $cmdhex->{$tile} = $hash;
        }
        close $in;
    }
    return $cmdhex;
}

sub write_cmdhex {
    my ($filename, $cmdhex) = @_;
    open my $out, ">", $filename or die "Cannot write $filename";
    for my $k (sort keys %$cmdhex) {
        printf $out "%s %s\n", $k, $cmdhex->{$k};
    }
    close $out;
}

sub check_cmdhex {
    my ($cmdhex, $in, $out, $cmds) = @_;
    my $stuff = undef;

    # Include a hash of the input tile in the mix.  This ensures the tile
    # will be rebuilt if the original tile has been updated.
    my @stat = stat($in);
    my $mtime = $stat[9];
    my $nbytes = $stat[7];
    my $info = sprintf("%d %d", $mtime, $nbytes);
    my $string0 = md5_hex($info);

    my $string = $string0.(join '##', @$cmds);
    my $digest = md5_hex($string);
    if ((-f $out.".tile") &&
        (defined $cmdhex->{$out}) &&
        ($cmdhex->{$out} eq $digest)) {
        #printf STDERR "1 : %s -> %s %s %s =?= %s\n", $in, $out, $string0, $digest, $cmdhex->{$out};
        return 1;
    } else {
        $cmdhex->{$out} = $digest;
        #printf STDERR "0 : %s -> %s %s %s =?= %s\n", $in, $out, $string0, $digest, $cmdhex->{$out};
        return 0;
    }
}

############# LIMIT HANDLING ##########

sub read_limits {
    my ($filename) = @_;
    my $limits = [ ];
    if (-f $filename) {
        open my $in, "<", $filename or die "Cannot open $filename";
        while (<$in>) {
            chomp;
            if (m{(\d+)\s+(\d+)\s+(\d+)\s+(\d+)}) {
                my $z1 = $1;
                my $x = $2;
                my $y = $3;
                my $z2 = $4;
                my $rec = {
                    max => $z2,
                    base => $z1,
                    x => $x,
                    y => $y
                };
                push @$limits, $rec;
            }
        }
        close $in;
    }
    return $limits;
}

sub check_limits {
    my ($limits, $zoom, $x, $y) = @_;
    my $ok = 0;
    for my $l (@$limits) {
        if ($zoom <= $l->{max}) {
            if (($x >> ($zoom - $l->{base}) == $l->{x}) &&
                ($y >> ($zoom - $l->{base}) == $l->{y})) {
                $ok = 1;
                last;
            }
        }
    }
    return $ok;
}

############# TILE TREE HANDLING ##########

sub ensure_hier {
    my @dirs = @_;
    my $d = "";
    while (@dirs) {
        my $c = shift @dirs;
        if (length $d > 0) { $d .= "/"; }
        $d .= $c;
        unless (-d $d) {
            mkdir $d;
        }
    }
    die unless (-d $d);
    return $d;
}

sub prepare_base_tile {
    my ($family, $zoom, $xi, $yi) = @_;
    my $result = undef;
    my $orig_d = ensure_hier("..", $family, $zoom, $xi);
    my $orig = $orig_d."/".$yi.".png";
    if (-f $orig) {
        $result = $orig;
    } else {
        my $missing = $orig_d."/".$yi.".MISSING";
        unless (-e $missing) {
            system ("touch", $missing);
        }
        # $result left undefined
    }
    return $result;
}

# vim:et:sw=4:ts=4:ht=4:sts=4
