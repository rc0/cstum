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

gen_map = { 2:'G', 3:'U' }

def emit(cells):
    if len(cells) == 0:
        return
    gen = cells[0][0]
    lac = cells[0][1]
    # Limit caption to 5 digits
    sys.stdout.write("\n= TBD\n^%d %d\n<> %s%d\n*" % (gen, cells[0][2] % 100000, gen_map[gen], lac))
    for c in cells:
        if c[2] == -1:
            sys.stdout.write (" []")
        else:
            sys.stdout.write (" [%s%d]" % (gen_map[gen], c[2]))
    sys.stdout.write ("\n")

try:
    input = sys.argv[1]
except:
    sys.stderr.write ("Usage: python list_to_tower.py <input.dat>\n")
    sys.exit(1)

spaces = re.compile(r'[ \t]+')
cells = []
with open (input, "r") as handle:
    for line in handle:
        line = line.strip()
        if len(line) == 0:
            continue
        if line.startswith("===") or line.startswith("---"):
            emit(cells)
            cells = []
        elif line.startswith("?"):
            gen = -1
            lac = -1
            cid = -1
            cells += [(gen, lac, cid)]
        else:
            fields = spaces.split(line)
            known = fields[0]
            gen = int(fields[1])
            lac = int(fields[2])
            cid = int(fields[3])
            if known == ".":
                sys.stderr.write ("Skipping LAC=%d,CID=%d : tower already in catalogue?\n" % (lac, cid))
            else:
                cells += [(gen, lac, cid)]
emit(cells)

