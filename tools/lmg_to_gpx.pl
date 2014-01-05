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


use strict;
use POSIX qw(strftime);

# Parse arguments
my $file = $ARGV[0];
shift;
my @ranges = ();
for my $r (@ARGV) {
    $r =~ m{^(\d+)-(\d+)} or die "Unrecognized range $r";
    my $range = { start => $1, end => $2 };
    push @ranges, $range;
}

if ($#ranges < 0) {
    die "No track ranges given on command line";
}

# Check ranges
@ranges = sort {$a->{start} <=> $b->{start}} @ranges;
for my $i (1 .. $#ranges) {
    if ($ranges[$i-1]->{end} >= $ranges[$i]->{start}) {
        die "Overlapping ranges";
    }
}

process_file($file, @ranges);
exit 0;


sub process_file {
    my ($filename, @ranges) = @_;
    my $period = $filename;
    if ($period =~ m{^(.*/)?(\d{2}(\d{2}\d{2})(\d)\d-\d{6})\.log$}) {
        $period = $2;
    } else {
        die "Unparsed filename $filename";
    }

    my $last_marker = 0;
    my @chunks = ();
    open IN, "<$filename" or die "Cannot open $filename";
    while (<IN>) {
        chomp;
        if (m{^\s*
                (\S+)\s+    # latitude
                (\S+)\s+    # longitude
                (\d+)\s+    # GPS accuracy
                ([EAXO])\s+ # service status
                ([GEUH])\s+ # network type
                (\d+)\s+    # CID
                (\d+)\s+    # LAC
                (\S+)       # dBm
                (?:
                \s+
                (\d+)       # MCCMNC
                \s+
                ([0-9.]+)   # elevation
                \s+
                (\d+)       # timestamp
                )?
            }x) {

            my ($lat,$lon,$acc,$state,$net_type,$cid,$lac,$dBm,$mccmnc,$ele,$ts) = ($1,$2,$3,$4,$5,$6,$7,$8,$9,$10,$11);

            next if ($acc == 0);
            next if ($acc > 24);
            next if ($lat < 40.0);
            next if ($lat > 60.0);
            next if ($lon < -10.0);
            next if (($lat == 0.0) && ($lon == 0.0));

            my $rec = {
                lon => $lon,
                lat => $lat,
                ele => $ele,
                timestamp => $ts,
                acc => $acc
            };
            push @{$chunks[$last_marker]}, $rec;

        } elsif (m{^\#\# MARKER (\d+)}) {
            $last_marker = $1;
        } else {
            print STDERR "Runt line <".$_."> in $filename\n";
        }
    }
    close IN;

    # TODO : reduce @data array to just the ranges given.

    print '<?xml version="1.0" encoding="UTF-8" ?>'."\n";
    print '<gpx'."\n";
    print '  version="1.0"'."\n";
    print '  creator="LogMyGSM - http://github.com/rc0/logmygsm"'."\n";
    print '  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"'."\n";
    print '  xmlns="http://www.topografix.com/GPX/1/0"'."\n";
    print '  xsi:schemaLocation="http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd">'."\n";

    # TODO PRINT <time>2011-07-17T11:10:39Z</time>
    print "<time>";
    my $epoch = $chunks[0][0]{timestamp};
    print gentime($epoch);
    print "</time>\n";

    # TODO : emit waypoints (markers - the first point in each chunk)
    for my $chunk (1..$#chunks) {
        my $r = $chunks[$chunk][0];
        printf "<wpt lat=\"%.8f\" lon=\"%.8f\">\n  <ele>%d</ele>\n  <time>%s</time>\n  <name>M%d</name>\n</wpt>\n",
            $r->{lat}, $r->{lon}, $r->{ele},
            gentime($r->{timestamp}),
            $chunk;
    }

    print '<trk>'."\n";
    for my $r (@ranges) {
        my ($start, $end) = ($r->{start}, $r->{end});

        print '<trkseg>'."\n";
        my $chunk;
        for ($chunk = $start; $chunk < $end; $chunk++) {
            for my $r (@{$chunks[$chunk]}) {
                emit_trackpoint($r);
            }
        }
        # Emit first point of next segment
        if ($end <= $#chunks) {
            emit_trackpoint($chunks[$end]->[0]);
        }
        print '</trkseg>'."\n";
    }
    print '</trk>'."\n";
    print '</gpx>'."\n";
}

sub gentime {
    my $epoch = $_[0];
    return strftime "%Y-%m-%dT%H:%M:%SZ", gmtime $epoch;
}

sub emit_trackpoint {
    my $r = $_[0];
    printf "<trkpt lat=\"%.8f\" lon=\"%.8f\"><ele>%d</ele><time>%s</time></trkpt>\n",
        $r->{lat}, $r->{lon}, $r->{ele},
        gentime($r->{timestamp});
}


# vim:et:sts=4:ht=4:sw=4

