#!/usr/bin/env perl

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
# >= Sep 2012 read routine.
# Start to export LAC into the tidied up data
# based on the realisation that CIDs are not unique country-wide

use strict;
use Math::Trig;

my $zb = 28;
my $nb = (2**$zb);

my %data = ();

my @files = @ARGV;

for my $f (@files) {
    process_file($f);
}

writeout(%data);

exit 0;

# -----------------------------------------------------------------------------

sub process_file {
    my $filename = $_[0];
    my $period = $filename;
    if ($period =~ m{^(.*/)?\d{2}(\d{2}\d{2})(\d)\d-\d{6}\.log$}) {
        $period = $2.(($3 >= 2) ? '2' : $3);
    } else {
        die "Unparsed filename $filename";
    }

    my $n2g = 0;
    my $n3g = 0;
    my $nnull = 0;
    my $neco = 0; # emergency calls only
    my $nogen = 0; # readings with no G, E, U, H to indicate service generation
    my $gen = undef;
    open IN, "<$filename" or die "Cannot open $filename";
    while (<IN>) {
        chomp;
        next if (m{^\#\#});
        if (m{^\s*
                (\S+)\s+    # latitude
                (\S+)\s+    # longitude
                (\d+)\s+    # GPS accuracy
                ([EAXO])\s+ # service status
                ([-GEUH])\s+ # network type
                (-?\d+)\s+    # CID (maybe negative if bad)
                (-?\d+)\s+    # LAC (maybe negative if bad)
                (\S+)       # dBm
                (?:
                \s+
                (\d+)       # MCCMNC
                )?
            }x) {

            my ($lat,$lon,$acc,$state,$net_type,$cid,$lac,$dBm,$mccmnc) = ($1,$2,$3,$4,$5,$6,$7,$8,$9);

            next if ($acc == 0);
            next if ($acc > 24);
            # These rules need to be changed for other parts of the world
            next if ($lat < 40.0);
            next if ($lat > 60.0);
            next if ($lon < -10.0);

            next if ($dBm >= 0);

            # Catch another obvious fail case that has occurred in the past.
            next if (($lat == 0.0) && ($lon == 0.0));

            my $asu = undef;
            if (($net_type eq '-') || ($cid < 0) || ($lac < 0)) {
                # Keep $gen from the last good sample, but apply a heuristic at the start of the file
                unless (defined $gen) {
                  if ($cid >= 65536) {
                    # 2g CIDs are only 16-bits? (Seems to be the case for the UK networks so far)
                    $gen = 3;
                  } else {
                    $gen = 2;
                  }
                }
                # The 8160 seems to do this when there's no coverage
                $asu = 0;
                $cid = 0;
                $lac = 65535;
                ++$nnull;
            } else {
              if (($net_type eq 'G') || ($net_type eq 'E')) {
                  ++$n2g;
                  $gen = 2;
              } elsif (($net_type eq 'U') || ($net_type eq 'H')) {
                  ++$n3g;
                  $gen = 3;
              }
              $asu = ($dBm + 113) >> 1;
              if (($cid == 0) || ($lac == 65535)) {
                  $asu = 0;
                  $cid = 0;
                  $lac = 65535;
                  ++$nnull;
              } elsif ($state ne 'A') {
                  # Leave ASU as-is
                  $cid = $lac = 0;
                  ++$neco;
              }
            }


            my ($X, $Y) = ll_to_frac($lat, $lon);
            $X = int(1.0 * $nb * $X);
            $Y = int(1.0 * $nb * $Y);

            # Which level-20 tile the reading falls in : no point storing
            # anything more fine-grained than that.
            my $xnt = $X >> 8;
            my $ynt = $Y >> 8;

            if (defined $gen) {
                # If we start the file out of coverage, and it's from the 8160,
                # we don't know if the radio is in 2G or 3G mode, so we don't
                # know what to log against then, unfortunately
                ++$data{$xnt}->{$ynt}->{$gen}->{$period}->{$lac.':'.$cid}->[$asu];
            }

        } else {
            print STDERR "Runt line <".$_."> in $filename\n";
        }
    }
    printf STDERR "File %s : %6d 2g, %6d 3g, %6d emerg only, %6d null, %6d nogen\n",
        $filename, $n2g, $n3g, $neco, $nnull, $nogen;

    close IN;
}

# -----------------------------------------------------------------------------

sub writeout {
    my %db = @_;
    for my $xnt (sort {$a <=> $b} keys %db) {
        for my $ynt (sort {$a <=> $b} keys %{$db{$xnt}}) {
            write_nano_tile($xnt, $ynt, $db{$xnt}->{$ynt});
        }
    }
}

sub write_nano_tile {
    my ($xnt, $ynt, $z) = @_;
    printf ": %7d %7d\n", $xnt, $ynt;
    for my $gen (sort {$a <=> $b} keys %$z) {
        my $zz = $z->{$gen};
        for my $period (sort {$a <=> $b} keys %$zz) {
            my $zzz = $zz->{$period};
            for my $laccid (sort {$a <=> $b} keys %$zzz) {
                my @obs = @{$zzz->{$laccid}};
                my @xobs = ();
                for my $i (0..31) {
                    $xobs[$i] = $obs[$i];
                }
                my $show = join ',', @xobs;
                my ($lac, $cid) = split /:/, $laccid;
                printf "%1d %d %s %s %s\n", $gen, $period, $lac, $cid, $show;
            }
        }
    }
    print "\n";
}

# -----------------------------------------------------------------------------

sub ll_to_frac {
    my ($lat, $lon) = @_;
    my $x = deg2rad($lon);
    my $y = log(tan(deg2rad($lat)) + sec(deg2rad($lat)));
    my $X = (1.0 + $x/pi)/2.0;
    my $Y = (1.0 - $y/pi)/2.0;
    return ($X, $Y);
}

# -----------------------------------------------------------------------------

# vim:et:sts=4:ht=4:sw=4:ts=4

