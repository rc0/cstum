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

import os
import sys
import subprocess

po = subprocess.Popen
source_tiles = "../osm"
suffix = {
        "2g" : ".tile",
        "3g" : ".tile",
        "m2g" : ".tile",
        "m3g" : ".tile",
        }
tile_paths = {
        "2g" : "tc/2g",
        "3g" : "tc/3g",
        "m2g" : "tc/m2g",
        "m3g" : "tc/m3g"
}

overlay_keys = {
        "2g" : "2",
        "3g" : "3",
        "m2g" : "2",
        "m3g" : "3"
}

missing_flags = {
        "2g" : "0",
        "3g" : "0",
        "m2g" : "1",
        "m3g" : "1"
}

nice_possibilities = ["/usr/bin/nice", "/bin/nice"]
nice = None
for poss in nice_possibilities:
    if os.path.isfile(poss):
        nice = poss
        break

# ----------------

def one_gen(z, deltaz, x, y, g, primary):
    base = "%s/%d/%d/%d.png" % (source_tiles, z, x, y)
    derived = "%s/%d/%d/%d.png%s" % (tile_paths[g], z, x, y, suffix[g])
    if os.path.isfile(base) and not os.path.isfile(derived):
        # Create output directory here, if needed, to prevent races
        # in the async subprocesses
        outdir = os.path.dirname(derived)
        if not os.path.isdir(outdir):
            os.makedirs(outdir)

        # Try to make the tiles on screen now be rendered more quickly than the others
        this_nice = []
        if nice != None:
            score = deltaz
            if primary != g:
                score += 1
            score = 2 + 10*score
            this_nice = [nice, "--adjustment=%d" % (score,)]

        args = this_nice + ["python3", "../cstum/tools/render_tile.py",
                str(z), str(x), str(y),
                overlay_keys[g],
                missing_flags[g], # the 'show_just_missing' flag
                base, derived]
        p1 = po(args)

# ----------------

z, x0, y0, x1, y1, recurse = [int(y) for y in sys.argv[1:7]]
primary = sys.argv[7] # which set of tiles to prioritise

if recurse == 2:
    for z1 in range(z, 6, -1):
        for x in range(x0,x1+1):
            for y in range(y0,y1+1):
                # Won't recursively generate the 'show_just_missing' maps (yet)
                one_gen(z1, z-z1, x, y, "2g", primary)
                one_gen(z1, z-z1, x, y, "3g", primary)
                one_gen(z1, z-z1, x, y, "m2g", primary)
                one_gen(z1, z-z1, x, y, "m3g", primary)
        x0 >>= 1
        y0 >>= 1
        x1 >>= 1
        y1 >>= 1
elif recurse == 1:
    for z1 in range(z, 6, -1):
        for x in range(x0,x1+1):
            for y in range(y0,y1+1):
                # Won't recursively generate the 'show_just_missing' maps (yet)
                one_gen(z1, z-z1, x, y, primary, primary)
        x0 >>= 1
        y0 >>= 1
        x1 >>= 1
        y1 >>= 1
else:
    for x in range(x0,x1+1):
        for y in range(y0,y1+1):
            one_gen(z, 0, x, y, primary, primary)

