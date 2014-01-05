#!/usr/bin/python3

import re
import sys
import subprocess

orig_tiles = "../osm"
tile_cache = "tc"

pat = re.compile(r'(m?)(\d)g_(\d+)_(\d+)_(\d+)')

for line in sys.argv[1:]:
    m = pat.match(line)
    if m:
        only_missing = 1 if m.group(1) == 'm' else 0
        g = m.group(2)
        out_prefix = line[m.start(1):m.end(2)] + "g"
        z = int(m.group(3))
        x = int(m.group(4))
        y = int(m.group(5))
        infile = "%s/%d/%d/%d.png" % (orig_tiles, z, x, y)
        outfile = "%s/%s/%d/%d/%d.png.tile" % (tile_cache, out_prefix, z, x, y)
        args = ["python3", "../cstum/tools/render_tile.py",
                str(z), str(x), str(y), str(g), str(only_missing), infile, outfile]
        print (' '.join(args))
        subprocess.call(args)
    else:
        print ("Mis-parse on line", x, file=sys.stderr);
        sys.exit(2)

