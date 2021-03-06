#!/usr/bin/python3

########################################################################
# Copyright (c) 2013 Richard P. Curnow
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

# Given a trail file, and a zoom level passed as an argument, find all the
# tiles at that zoom level that are touched by the trail.

import os, sys, re
from math import *

try:
    maxzoom = sys.argv[1]
    trails = sys.argv[2:]
except:
    sys.stderr.write ("Usage: python touched_by.py <maxzoom> <trail.log> ..\n")
    sys.exit(1)

tiles = set()
maxzoom = int(maxzoom)
spaces = re.compile(r'[ \t]+')
for trail in trails:
    with open (trail, "r") as handle:
        for line in handle:
            line = line.strip()
            fields = spaces.split(line)
            lat = float(fields[0])
            lon = float(fields[1])
            x = 0.5 * (1.0 + radians(lon)/pi)
            latr = radians(lat)
            y = 0.5 * (1.0 - log(tan(latr) + 1.0/cos(latr))/pi)
            for z in range(7,1+maxzoom):
                scale = float(1<<z)
                X = int(x*scale)
                Y = int(y*scale)
                tiles.add((z,X,Y))

for z, X, Y in sorted(tiles):
    print ("%2d %6d %6d %2d" % (z, X, Y, z))


