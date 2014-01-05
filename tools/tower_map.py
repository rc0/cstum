#!/usr/bin/python3

# Generate a map of coverage from a given tower

from sys import argv as av
import sys
import re
import os
import subprocess

# -------------------------

PRIMARY3 = 8
PRIMARY2 = 7
PRIMARY1 = 6
PRIMARY0 = 5
PRIMARY=4
SECONDARY=3
OTHER=2
UNKNOWN=1

# -------------------------

class Tile:
    def __init__(self, x, y):
        self.t2 = self.create()
        self.t3 = self.create()
        self.score = [0,0,0,0]
        self.x = x
        self.y = y

    def create(self):
        return [[UNKNOWN for x in range(0,16)] for y in range(0,16)]

    def insert(self, gen, sx, sy, asu, cids):
        t = self.t2 if gen==2 else self.t3
        t[sx][sy] = OTHER
        if cids[0].endswith("#"):
            t[sx][sy] = PRIMARY
        else:
            for cid in cids[1:]:
                if cid.endswith("#"):
                    t[sx][sy] = SECONDARY
                    break
        if   t[sx][sy] == PRIMARY:
            self.score[gen] += 3
            if   asu > 20: t[sx][sy] = PRIMARY0
            elif asu > 11: t[sx][sy] = PRIMARY1
            elif asu >  4: t[sx][sy] = PRIMARY2
            elif asu >= 0: t[sx][sy] = PRIMARY3
        elif t[sx][sy] == SECONDARY: self.score[gen] += 1

# -------------------------

class Coverage:
    def __init__(self, datafile, lac, cid, zoom):
        self.tiles = {}
        self.minx = None
        self.miny = None
        self.maxx = None
        self.maxy = None
        args = ["../cstum/cstum", "tiles", str(zoom), datafile, "--", "%d/%d" % (lac,cid)]
        child = subprocess.Popen(args, stdout=subprocess.PIPE)
        for line in child.stdout:
            ll = line.decode("latin-1").strip()
            self.process(ll)
        print ("Tile range X:[%d,%d] Y:[%d,%d]" % (self.minx,self.maxx,self.miny,self.maxy))

    def process(self, line):
        m = re.match(r'<< +(\d+) +(\d+)', line)
        if m:
            x = int(m.group(1))
            y = int(m.group(2))
            if not self.minx or x < self.minx: self.minx = x
            if not self.maxx or x > self.maxx: self.maxx = x
            if not self.miny or y < self.miny: self.miny = y
            if not self.maxy or y > self.maxy: self.maxy = y
            self.current = Tile(x, y)
            self.tiles[(x,y)] = self.current
            return
        m = re.match(r'\*[23]', line)
        if m:
            return
        m = re.match(r'>>', line)
        if m:
            return
        # Assume now it's a "normal" line
        fields = re.split(r' +', line)
        gen, sx, sy, age, asu, junk = [int(x) for x in fields[0:6]]
        others = fields[6:]
        self.current.insert(gen, sx, sy, asu, others)

    def best_area(self, gen, rx, ry):
        x0 = self.minx
        y0 = self.miny
        x1 = self.maxx - rx + 1
        y1 = self.maxy - ry + 1
        if x0 > x1 or y0 > y1:
            mx = self.maxx - self.minx + 1
            my = self.maxy - self.miny + 1
            sys.stderr.write("Tile range %d,%d is too big!\nMax is %d by %d\n" %
                    (rx,ry,mx,my))
            sys.exit(2)
        best_total = 0
        best_i0 = None
        best_j0 = None
        for i0 in range(x0, x1+1):
            for j0 in range(y0, y1+1):
                total = 0
                for ii in range(rx):
                    for jj in range(ry):
                        ix = (i0+ii, j0+jj)
                        if ix in self.tiles:
                            t = self.tiles[ix]
                            score = t.score[gen]
                            total += score
                if total > best_total or not best_i0:
                    best_total = total
                    best_i0 = i0
                    best_j0 = j0
        return (best_i0, best_j0)

# -------------------------

try:
    zoom, lac, cid, gen, rx, ry = [int(av[x]) for x in [1,3,4,5,6,7]]
    datafile = av[2]
    outfile = av[8]
except:
    sys.stderr.write("Usage: zoom [+]datafile lac cid gen xrange yrange outfile\n")
    sys.exit(2)

data = Coverage(datafile, lac, cid, zoom)
xs, ys = data.best_area(gen, rx, ry)
print ("Best area for a %d,%d region starts at %d,%d\n" % (rx, ry, xs, ys))

TEMP = "foobar.png"
args = ["montage", "-geometry", "256x", "-border", "0"]
args += ["-tile", "%dx%d" % (rx, ry)]
for jj in range (0,ry):
    for ii in range (0,rx):
        yy = jj + ys
        xx = ii + xs
        path = "../osm/%d/%d/%d.png" % (zoom, xx, yy)
        if os.path.exists(path):
            args += [path]
        else:
            args += ["./offwhite.png"]
args += [TEMP]
aa = ' '.join(args)
#print ("Command line %s" % (aa,))
subprocess.call(args)

# -----------------------------
# Now overlay the picture

def build(colour, which):
    myargs = ["-fill", colour]
    for jj in range (0,ry):
        for ii in range (0,rx):
            yy = jj + ys
            xx = ii + xs
            ix = (xx, yy)
            if ix in data.tiles:
                t = data.tiles[ix]
                tt = t.t2 if gen==2 else t.t3
                for i in range(0,16):
                    for j in range(0,16):
                        ttt = tt[i][j]
                        if ttt == which:
                            a0 = (ii<<8) + (i<<4)
                            b0 = (jj<<8) + (j<<4)
                            myargs += ["-draw",
                                       "rectangle %d,%d %d,%d" % (a0,b0,a0+15,b0+15)]
    return myargs

def fill_missing(colour):
    args = ["-fill", colour]
    for jj in range (0,ry):
        for ii in range (0,rx):
            yy = jj + ys
            xx = ii + xs
            ix = (xx, yy)
            if ix not in data.tiles:
                a0 = (ii<<8)
                b0 = (jj<<8)
                args += ["-draw", "rectangle %d,%d %d,%d" % (a0,b0,a0+255,b0+255)]
    return args

trans = "80"

colours = {
    PRIMARY0:  "#c41685" + trans,
    PRIMARY1:  "#bf3f01" + trans,
    PRIMARY2:  "#c18f00" + trans,
    PRIMARY3:  "#c4c42f" + trans,
    SECONDARY: "#059600" + trans,
    OTHER:     "#2d2da6" + trans,
    UNKNOWN:   "#202020a0"
}

args = []
args += ["convert", TEMP]
for c in colours:
    args += build(colours[c], c)
args += fill_missing(colours[UNKNOWN])

args += [outfile]
aa = ' '.join(args)
#print ("Command line %s" % (aa,))
subprocess.call(args)


