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

use strict;
my $path = $0;
my $prefix = $path;
$prefix =~ s{[^/]+$}{};
my $common = $prefix.'../tiling/common.pl';
unless (-f $common) {
  die "No $common";
}
do $common;

my @towers = read_towers("towers.dat");

my @recs = ();

my $i = 0;
for my $tower (@towers) {
    my $pos = $tower->{observed} // $tower->{estimated};
    next unless (defined $pos);
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
            push @recs, {
                cid => $cid,
                lac => $lac,
                x => $pos->{x},
                y => $pos->{y},
                colour => $tower->{colour},
                index => $i
            };
        }
    }
    $i++;
}

open OUT, ">out/cidxy.txt";
printf OUT "%d\n", (scalar @recs);
for my $r (@recs) {
    printf OUT "%d\n", $r->{cid};
    printf OUT "%d\n", $r->{lac};
    printf OUT "%d\n", $r->{x};
    printf OUT "%d\n", $r->{y};
}
close OUT;

open OUT, ">out/cidxy2.txt";
for my $r (@recs) {
    printf OUT "%d %d %d %d\n",
        $r->{cid},
        $r->{lac},
        $r->{x},
        $r->{y};
}
close OUT;

open OUT, ">out/cid_paint.txt";
for my $r (@recs) {
    if (defined $r->{colour}) {
        printf OUT "%d %d %d %d\n",
            $r->{cid},
            $r->{lac},
            $r->{index},
            $r->{colour};
        }
}
close OUT;

# vim:et:sw=4:ts=4:ht=4:sts=4
