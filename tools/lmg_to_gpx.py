import sys

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

import re
import time

def usage():
    sys.stderr.write("Usage: " + sys.argv[0] + " <lmg_file> <ranges>...\n");

def idiot():
    usage()
    sys.exit(1)

def oops(m):
    sys.stderr.write("Error : " + m + "\n")
    sys.exit(1)

# ----------------

def munge_range(r):
    x = re.split('-',r)
    return tuple(x)

# ----------------

def gentime(e):
    g = time.gmtime(e)
    return time.strftime("%Y-%m-%dT%H:%M:%SZ", g)

# ----------------

def emit_trackpoint(pt):
    lat, lon, ele, ts, acc = pt
    print ('  <trkpt lat="' + lat + '" lon="' + lon + '"><ele>' + ele + '</ele><time>' + gentime(int(ts)) + '</time></trkpt>');

# ----------------

try:
    trace_name = sys.argv[1]
except IndexError as e:
    idiot()

ranges = [munge_range(r) for r in sys.argv[2:]]

if len(ranges) == 0:
    idiot()

for x in range(1,len(ranges)):
    s0, e0 = ranges[x-1]
    s1, e1 = ranges[x]
    if e0 >= s1:
        oops("Ranges are not ordered")

# Process the input file

try:
    handle = open(trace_name, "r")
except:
    oops("Problem opening " + trace_name)

chunks = []
last_marker = 0
marker_re = re.compile(r'^\#\# MARKER (\d+)')

for line in handle:
    line0 = line.rstrip("\n")
    m = marker_re.match(line0)
    if m:
        last_marker = int(m.group(1))
    else:
        line0 = line0.lstrip()
        fields = re.split(r'\s+', line0)
        lat, lon, acc, state, net_type, cid, lac, dBm, mccmnc, ele, ts = tuple(fields)
        acci = int(acc)
        latf = float(lat)
        lonf = float(lon)

        if (acci == 0) or (acci > 24) or (latf < 40.0) or (latf > 60.0) or (lonf < -10.0) or \
                ((latf == 0.0) and (lonf == 0.0)):
            continue

        rec = lat, lon, ele, ts, acc
        if len(chunks) <= last_marker:
            chunks.append([])
        chunks[last_marker].append(rec)

handle.close()

print ('<?xml version="1.0" encoding="UTF-8" ?>')
print ('<gpx')
print ('  version="1.0"')
print ('  creator="LogMyGSM - http://github.com/rc0/logmygsm"')
print ('  xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance"')
print ('  xmlns="http://www.topografix.com/GPX/1/0"');
print ('  xsi:schemaLocation="http://www.topografix.com/GPX/1/0 http://www.topografix.com/GPX/1/0/gpx.xsd">')

epoch = int(chunks[0][0][3])
print ('<time>' + gentime(epoch) + '</time>')

# Emit waypoints
for i in range(1,len(chunks)):
    lat, lon, ele, ts, acc = chunks[i][0]
    print ('<wpt lat="' + lat + '" lon="' + lon + '">')
    print ('  <ele>' + ele + '</ele>')
    print ('  <time>' + gentime(int(ts)) + '</time>')
    print ('  <name>' + 'M' + str(i) + '</name>')
    print ('</wpt>')

# Emit tracks
print ('<trk>')
for s, e in ranges:
    print ('  <trkseg>')
    s = int(s)
    e = int(e)
    if e > len(chunks):
        e = len(chunks)
    for i in range(s, e):
        for pt in chunks[i]:
            emit_trackpoint(pt)
    # Emit the first point in the next chunk
    if (e < len(chunks)):
        emit_trackpoint(chunks[e][0])
    print ('  </trkseg>')
print ('</trk>')
print ('</gpx>')




