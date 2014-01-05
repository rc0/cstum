#!/usr/bin/python3
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

import os
import sys

level = int(sys.argv[1])
if level <= 12:
    print ("I don't split below level 13")
    sys.exit(2)

shift = level - 12

infile = "out/level_%d.dat" % level
handle = open(infile, "r")
out = {}
for line in handle:
    if line.startswith("<<"):
        fields = line.split()
        xx = int(fields[1])
        yy = int(fields[2])
        x12 = xx >> shift
        y12 = yy >> shift
        key = (x12,y12)
        if key not in out:
            out[key] = []
    out[key].append(line)

for x12,y12 in sorted(out.keys()):
    dir = "out/lev12/%d/%d" % (x12, y12)
    file = "%s/level_%d.dat" % (dir, level)
    if not os.path.isdir(dir):
        os.makedirs(dir)
    with open(file, "w") as f:
        for line in out[(x12,y12)]:
            f.write(line)


