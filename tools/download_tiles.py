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

from overlay import DB
import os
import sys
import time
import urllib.request

urls = {
        "osm" : "http://a.tile.openstreetmap.org/%d/%d/%d.png",
        "ocm" : "http://a.tile.opencyclemap.org/cycle/%d/%d/%d.png",
}

def fetch_upto(z, x, y, dz, basedir, tileset):
    outfile = "%s/%s/%d/%d/%d.png" % (basedir, tileset, z, x, y)
    if os.path.isfile(outfile):
        # Try to fetch children
        if dz > 0:
            #print ("  doing children of z=%d x=%d y=%d" % (z, x, y))
            for i in [0,1]:
                for j in [0,1]:
                    xx = x + x + i
                    yy = y + y + j
                    fetch_upto(z+1, xx, yy, dz-1, basedir, tileset)
    else:
        # Do a download.  Assume standard OpenStreetMap base tiles
        #print ("  downloading z=%d x=%d y=%d" % (z, x, y))
        url = urls[tileset] % (z, x, y)
        tempfile = outfile + ".tmp"
        tempdir = os.path.dirname(tempfile)
        if not os.path.isdir(tempdir):
            os.makedirs(tempdir)
        try:
            print ("Download (%d,%d,%d)" % (z, x, y))
            urllib.request.urlretrieve(url, tempfile)
        except:
            sys.stderr.write("Problem fetching tile from %s\n" % (url,))
            sys.exit(1)
        finally:
            os.rename(tempfile, outfile)



z, x0, y0, x1, y1, dz, sleep_count = [int (y) for y in sys.argv[1:8]]
basedir, tileset = sys.argv[8:10]
#print ("Doing z=%d x=%d y=%d" % (z, x, y))
time.sleep(sleep_count)
# Fetch outwards
while z >= 4:
    for X in range(x0, x1+1):
        for Y in range(y0, y1+1):
            fetch_upto(z, X, Y, dz, basedir, tileset)
    x0 >>= 1
    x1 >>= 1
    y0 >>= 1
    y1 >>= 1
    z -= 1

