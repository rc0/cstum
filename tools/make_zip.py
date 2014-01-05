import time

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

import zipfile
import os
import fnmatch
import re
import sys
import stat

LAST_PATH = "./LAST"
t = int(time.time())

INSERT_PATH = "./INSERT"

def generate(path, dest, cutoff):
    result = []
    for root, dirnames, filenames in os.walk(path):
        for filename in fnmatch.filter(filenames, '*.png.tile'):
            fullname = os.path.join(root, filename)
            mt = int(os.stat(fullname).st_mtime)
            if mt > cutoff:
                components = fullname.split('/')
                munged_components = [dest] + components[-3:]
                munged = '/'.join(munged_components)
                record = (fullname, munged)
                result += [record]
    return result

def output_to_zip(tiles):
    nfiles = 0
    output_name = "%d.zip" % t
    with zipfile.ZipFile(output_name, mode="w") as zip:
        for tile in tiles:
            full, munged = tile
            zip.write(full, munged)
            nfiles += 1
    print ("Wrote %d files to %s" % (nfiles, output_name))
    with open(LAST_PATH, mode="w") as last:
        tt = time.localtime(t)
        last.write(time.asctime(tt))

def output_to_stdout(tiles):
    for tile in tiles:
        full, munged = tile
        print ("%s -> %s" % (full, munged))

def croak():
    sys.stderr.write("Usage: python3 make_zip.py 36m|10h|2d\n")
    sys.exit(1)

if os.path.exists(LAST_PATH):
    last_update_t = os.stat(LAST_PATH).st_mtime
    offset = t - last_update_t
    print ("Using %s as basis : %d seconds ago" % (LAST_PATH, offset))
else:
    if len(sys.argv) < 2:
        croak()
    offset_data = re.match(r'(\d+)([mhdwy])$', sys.argv[1])
    if offset_data:
        count, unit = offset_data.groups()
        count = int(count)
        if unit == 'm':
            offset = count * 60
        elif unit == 'h':
            offset = count * 3600
        elif unit == 'd':
            offset = count * 86400
        elif unit == 'w':
            offset = count * 7 * 86400
        elif unit == 'y':
            offset = count * 365 * 86400
    else:
        croak()

    print ("Using command line option as basis : %d seconds ago" % offset)

cutoff = t - offset

if os.path.exists(INSERT_PATH):
    with open(INSERT_PATH, "r") as handle:
        line = handle.readline()
        insert = line.strip()
    tiles_2g     = generate('../out/2g',    'logmygsm_' + insert + '_2g', cutoff)
    tiles_3g     = generate('../out/3g',    'logmygsm_' + insert + '_3g', cutoff)
    tiles_age2g  = generate('../out/age2g', 'logmygsm_' + insert + '_age2g', cutoff)
    tiles_age3g  = generate('../out/age3g', 'logmygsm_' + insert + '_age3g', cutoff)
else:
    tiles_2g     = generate('../out/2g',    'Custom 2', cutoff)
    tiles_3g     = generate('../out/3g',    'Custom 3', cutoff)
    tiles_age2g  = generate('../out/age2g', 'logmygsm_age2g', cutoff)
    tiles_age3g  = generate('../out/age3g', 'logmygsm_age3g', cutoff)

tiles = tiles_2g + tiles_3g + tiles_age2g + tiles_age3g
if len(tiles) == 0:
    print ("Nothing to write")
else:
    #output_to_stdout(tiles)
    output_to_zip(tiles)

# vim:et:sw=4:sts=4:ht=4

