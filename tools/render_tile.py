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
import urllib.request

z, x, y, g, show_just_missing = [int (y) for y in sys.argv[1:6]]
infile = sys.argv[6]
outfile = sys.argv[7]

# If we end up running multiple renderers in parallel for the same tile, we
# could find the file exists.  For now, bail out.
# AU CONTRAIRE : allow user to regenerate a few tiles if he knows the data has
# changed in that area.
#if os.path.isfile(outfile):
#    sys.exit(0)

if not os.path.isfile(infile):
    # Do a download.  Assume standard OpenStreetMap base tiles
    url = "http://a.tile.openstreetmap.org/%d/%d/%d.png" % (z, x, y);
    tempfile = infile + ".tmp"
    tempdir = os.path.dirname(tempfile)
    if not os.path.isdir(tempdir):
        os.makedirs(tempdir)
    try:
        urllib.request.urlretrieve(url, tempfile)
        os.rename(tempfile, infile)
    except:
        sys.stderr.write("Problem fetching tile from %s\n" % (url,))
        if os.path.isfile(tempfile):
            os.unlink(tempfile)
        sys.exit(1)

# Allow for just downloading if the OSM base map is being displayed
if g in {2,3}:
    file = "out/overlay.db"
    db = DB(file)

    outdir = os.path.dirname(outfile)
    if not os.path.isdir(outdir):
        os.makedirs(outdir)

    recipe = db.get_recipe(z, x, y, g)

    # Assumes $pwd is on the same filesystem as outfile will be
    temp_name = "render_tile_%d_%d_%d_%d_%d.png" % (z, x, y, g, os.getpid())
    recipe.do_convert(infile, temp_name, "aa", show_just_missing)

    # The rename is guaranteed atomic.  This has two benefits : tools cannot see a
    # part-written png file, and it's safe if two rendering jobs happen to be
    # creating the same file at the same time
    os.rename(temp_name, outfile)

