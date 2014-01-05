import os
import sys
import math
import subprocess
from itertools import chain

# ---------------------------

class Recipe:
    colours = [
        "#FF2C2C",
        "#00D000",
        "#8747FF",
        "#D97800",
        "#08D6D2",
        "#FF30F6",
        "#96A600",
        "#0E7EDD",
        "#FF9A8C",
        "#91E564",
        "#F98CFF",
        "#5E0857",
        "#5A2C00",
        "#084500",
        "#18256B"
    ]

    def __init__(self):
        self.circles = []
        self.sectors = []
        self.donuts = []
        self.nulls = []
        self.long_vanes = []
        self.short_vanes = []
        self.towers = []
        self.adjacents = []
        self.multis = []
        self.missing = [[1 for i in range(0,16)] for j in range(0,16)]
        pass

    def add_circle(self, x, y, size, colour):
        self.circles += [(x, y, size, colour)]
        self.missing[x][y] = 0

    def add_sector(self, x, y, size, colour, angle):
        self.sectors += [(x, y, size, colour, angle)]
        self.missing[x][y] = 0

    def add_donut(self, x, y, size, colour):
        self.donuts += [(x, y, size, colour)]
        self.missing[x][y] = 0

    def add_null(self, x, y):
        self.nulls += [(x, y)]
        self.missing[x][y] = 0

    def add_long_vane(self, x, y, angle):
        self.long_vanes += [(x, y, angle)]
        self.missing[x][y] = 0

    def add_short_vane(self, x, y, angle):
        self.short_vanes += [(x, y, angle)]
        self.missing[x][y] = 0

    def add_tower(self, x, y, colour, eno, caption):
        self.towers += [(x, y, colour, eno, caption)]
        self.missing[x][y] = 0

    def add_adjacent(self, x, y, colour, eno):
        self.adjacents += [(x, y, colour, eno)]
        self.missing[x][y] = 0

    def add_multi(self, x, y):
        self.multis += [(x, y)]
        self.missing[x][y] = 0

    def raw_dump(self):
        if self.circles:
            print ("CIRCLES:")
            for c in self.circles:
                print ("  %2d %2d sz=%2d col=%2d" % (c[0], c[1], c[2], c[3]))
        if self.sectors:
            print ("SECTORS:")
            for s in self.sectors:
                print ("  %2d %2d sz=%2d col=%2d ang=%d" % (s[0], s[1], s[2], s[3], s[4]))
        if self.donuts:
            print ("DONUTS:")
            for d in self.donuts:
                print ("  %2d %2d sz=%2d col=%2d" % (d[0], d[1], d[2], d[3]))
        if self.nulls:
            print ("NULLS:")
            for n in self.nulls:
                print ("  %2d %2d" % (n[0], n[1]))
        if self.long_vanes:
            print ("LONG VANES:")
            for v in self.long_vanes:
                print ("  %2d %2d ang=%d" % (v[0], v[1], v[2]))
        if self.short_vanes:
            print ("SHORT VANES:")
            for v in self.short_vanes:
                print ("  %2d %2d ang=%d" % (v[0], v[1], v[2]))

    def circle_args(self, c, alpha):
        x0 = 8 + c[0]*16
        y0 = 8 + c[1]*16
        col = self.colours[c[3]] + alpha
        args = ["-fill", col]
        args += ["-draw", "circle %d,%d %d,%d" %
                (x0, y0, x0+c[2], y0)]
        return args

    def donut_args(self, d):
        x0 = 8 + d[0]*16
        y0 = 8 + d[1]*16
        args = ["-draw", "circle %d,%d %d,%d" %
                (x0, y0, x0+d[2], y0)]
        return args

    def sector_args(self, c, alpha):
        span = 120
        xx, yy, size, colour, angle = c
        x0 = 8 + xx*16
        y0 = 8 + yy*16
        angle *= 360.0 / 32.0
        angle -= 90
        col = self.colours[colour] + alpha
        a = ["-draw", "ellipse %d,%d %d,%d %d,%d" %
                (x0, y0, size, size, angle-span, angle+span)]
        a1 = ["-fill", col, "-stroke", "none"] + a
        a2 = ["-fill", "none", "-stroke", "black"] + a
        return (a1, a2)

    def vane_args(self, length, v):
        x0 = 8 + v[0]*16
        y0 = 8 + v[1]*16
        ang = v[2] * math.pi / 16.0
        dx = length * math.cos(ang)
        dy = length * math.sin(ang)
        args = ["-draw", "line %d,%d %d,%d" %
                (x0, y0, x0+dx, y0+dy)]
        return args

    def null_args(self, n):
        xc = 8 + n[0]*16
        yc = 8 + n[1]*16
        args = []
        args += [ "-draw", "line %d,%d %d,%d" % (xc-8, yc-4, xc  , yc-8)]
        args += [ "-draw", "line %d,%d %d,%d" % (xc-8, yc+4, xc+8, yc-4)]
        args += [ "-draw", "line %d,%d %d,%d" % (xc  , yc+8, xc+8, yc+4)]
        args += [ "-draw", "line %d,%d %d,%d" % (xc-8, yc  , xc-4, yc+8)]
        args += [ "-draw", "line %d,%d %d,%d" % (xc-4, yc-8, xc+4, yc+8)]
        args += [ "-draw", "line %d,%d %d,%d" % (xc+4, yc-8, xc+8, yc  )]
        return args

    def tower_args(self,t):
        x, y, col, eno, caption = t
        x0 = x*16
        y0 = y*16
        if eno:
            extra = "stroke-dasharray 2 2 "
        else:
            extra = ""
        args = [ "-stroke", "black", "-strokewidth", "2", "-fill", self.colours[col]]
        args += [ "-draw", extra + ("rectangle %d,%d %d,%d" % (x0+1, y0+1, x0+15, y0+15))]
        if len(caption) > 0:
            tw = 2 + 5*len(caption)
            buffer = 2
            xl = x0 + 8 - (tw>>1)
            xr = x0 + 8 + (tw>>1)
            if xl < buffer:
                xl = buffer
            if xr > (255 - buffer):
                xl -= (xr - (255 - buffer))
            args += [ "-font", "helvetica", "-pointsize", "10"]
            args += [ "-fill", "black", "-stroke", "none", "-undercolor", "#eeeeeea0"]
            args += [ "-gravity", "northwest" ]
            args += [ "-annotate", "0x0+%d+%d" % (xl, y0+9), caption]
        return args

    def adjacent_args(self,a):
        x, y, col, eno = a
        x0 = x*16
        y0 = y*16
        args = [ "-stroke", "black", "-strokewidth", "1", "-fill", self.colours[col]]
        args += [ "-draw", "rectangle %d,%d %d,%d" % (x0+3, y0+3, x0+13, y0+13)]
        return args

    def multi_args(self,m):
        x, y = m
        x0 = x*16
        y0 = y*16
        args = [ "-stroke", "black", "-strokewidth", "1", "-fill", "#ff2020a0"]
        args += ["-draw", "rectangle %d,%d %d,%d" % (x0+1, y0+1, x0+9, y0+9)]
        args += ["-draw", "rectangle %d,%d %d,%d" % (x0+7, y0+3, x0+15, y0+11)]
        args += ["-draw", "rectangle %d,%d %d,%d" % (x0+3, y0+6, x0+11, y0+14)]
        return args

    def missing_args(self, is_dummy):
        for lvl in [0,1,2,3]:
            step0 = 1<<lvl
            count = 1<<lvl
            step1 = 2<<lvl
            i = 0
            while i < 16:
                j = 0
                while j < 16:
                    if ((self.missing[i][j] == count) and
                        (self.missing[i+step0][j] == count) and
                        (self.missing[i][j+step0] == count) and
                        (self.missing[i+step0][j+step0] == count)):
                        self.missing[i][j] = count + count
                        self.missing[i+step0][j] = 0
                        self.missing[i][j+step0] = 0
                        self.missing[i+step0][j+step0] = 0
                    j += step1
                i += step1
        args = []
        for i in range(0,16):
            for j in range(0,16):
                count = self.missing[i][j]
                if count > 0:
                    x0 = 1 + (i << 4)
                    y0 = 1 + (j << 4)
                    x1 = ((i + count) << 4) - 2
                    y1 = ((j + count) << 4) - 2
                    args += [ "-draw", "rectangle %d,%d %d,%d" % (x0, y0, x1, y1)]
        colour, linewidth = ("#e00000c0", "1") if is_dummy else ("#0000ff40", "1")
        if len(args) > 0:
            #return ["-stroke", "#0000ff40", "-strokewidth", "1", "-fill", "#0000ff10"] + args
            return ["-stroke", colour, "-strokewidth", linewidth, "-fill", "none"] + args
        else:
            return []

    def dummy_args(self, x):
        '''When we want to depict covered areas as just coloured in blocks'''
        x, y, *junk = x
        x0 = x*16
        y0 = y*16
        args = [ "-stroke", "none", "-fill", "#8080aa80"]
        args += [ "-draw", "rectangle %d,%d %d,%d" % (x0, y0, x0+15, y0+15)]
        return args

    def do_convert(self, in_path, out_path, alpha, show_just_missing):
        args = []
        args += ["convert", in_path]
        if show_just_missing:
            for x in chain(self.circles, self.donuts, self.sectors, self.nulls):
                args += self.dummy_args(x)
        else:
            if self.circles:
                args += ["-stroke", "black"]
                args += ["-strokewidth", "1"]
                for x in self.circles:
                    args += self.circle_args(x, alpha)
            if self.donuts:
                args += ["-fill", "none"]
                args += ["-stroke", "#ff0000" + alpha]
                args += ["-strokewidth", "2"]
                for x in self.donuts:
                    args += self.donut_args(x)
            if self.sectors:
                args += ["-strokewidth", "1"]
                for x in self.sectors:
                    a1, a2 = self.sector_args(x, alpha)
                    args += a1
                    args += a2
            if self.nulls:
                args += ["-stroke", "#ff0000"]
                args += ["-strokewidth", "1"]
                for x in self.nulls:
                    args += self.null_args(x)
            if self.long_vanes:
                args += ["-stroke", "black"]
                args += ["-strokewidth", "1"]
                for x in self.long_vanes:
                    args += self.vane_args(8, x)
            if self.short_vanes:
                args += ["-stroke", "black"]
                args += ["-strokewidth", "1"]
                for x in self.short_vanes:
                    args += self.vane_args(5, x)

        if self.towers:
            for x in self.towers:
                args += self.tower_args(x)
        if self.adjacents:
            for x in self.adjacents:
                args += self.adjacent_args(x)
        if self.multis:
            for x in self.multis:
                args += self.multi_args(x)

        # Do blank areas
        args += self.missing_args(show_just_missing)

        args += [out_path]
        subprocess.call(args)


# --------------------------

class FileFormatException(Exception):
    pass

# --------------------------

class DB:

    # Index by [5:4] is the x+x+y of the subtile
    # and [3:0] is bits [3:0] of the 'presence' byte
    table = [
        -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0, -1, 0,
        -1, -1, 0, 1, -1, -1, 0, 1, -1, -1, 0, 1, -1, -1, 0, 1,
        -1, -1, -1, -1, 0, 1, 1, 2, -1, -1, -1, -1, 0, 1, 1, 2,
        -1, -1, -1, -1, -1, -1, -1, -1, 0, 1, 1, 2, 1, 2, 2, 3
    ]
    table2 = [
        0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4
    ]

    table3 = [
        -1, 0, -1, 0,
        -1, -1, 0, 1
    ]

    genmap = { 2:0, 3:4 }

    def __init__(self, name):
        self.h = open(name, "rb")
        self.name = name
        self.len = os.path.getsize(name)

    def read_32(self, pos):
        self.h.seek(pos, os.SEEK_SET)
        b = self.h.read(4)
        return int.from_bytes(b, 'big')

    def read_16(self, pos):
        self.h.seek(pos, os.SEEK_SET)
        b = self.h.read(2)
        return int.from_bytes(b, 'big')

    def read_8(self, pos):
        self.h.seek(pos, os.SEEK_SET)
        b = self.h.read(1)
        return b[0]

    def read_8_multi(self, pos, n):
        self.h.seek(pos, os.SEEK_SET)
        b = self.h.read(n)
        return b

    def lookup(self, z, x, y, g):
        control_word = self.read_8_multi(0, 4)
        # Check whether the first 4 bytes of the file match the expected
        # pattern.  This makes sure the file isn't likely to be something else,
        # or the wrong version
        if control_word != bytes(b'\xfeOV\x02'):
            raise FileFormatException("{} file does not have the expected format".format(self.name))

        loc = self.len - 4
        delta = self.read_32(loc)
        loc -= delta

        while z > 0:
            z -= 1
            xz = (x >> z) & 1
            yz = (y >> z) & 1
            p = self.read_8(loc)
            wide = True if (p & 0x80) != 0 else False
            pp = p & 15
            idx = pp + ((xz + xz + yz) << 4)
            t = self.table[idx]
            if t < 0:
                return None
            if wide:
                delta = self.read_32(loc + 1 + 4*t)
            else:
                delta = self.read_16(loc + 1 + 2*t)
            loc -= delta

        p = self.read_8(loc)
        wide = True if (p & 0x80) != 0 else False
        pp = p & 15
        ppp = (p >> 4) & 0x3
        idx = self.table2[pp] # base of the gen2/gen3 tables
        t = self.table3[self.genmap[g] + ppp]
        if t < 0:
            return None
        else:
            t += idx
            if wide:
                delta = self.read_32(loc + 1 + 4*t)
            else:
                delta = self.read_16(loc + 1 + 2*t)
            loc -= delta
            return loc

    def posxy(self, pos):
        return pos>>4, pos&15

    def size_col(self, loc):
        b = self.read_8(loc+1);
        size = (b >> 4) & 0xf;
        colour = b & 0xf
        return size, colour

    def eno_len_col(self, loc):
        b = self.read_8(loc+1)
        eno = (b >> 7) & 0x1
        len = (b >> 4) & 0x7
        colour = b & 0xf
        return eno, len, colour

    def get_recipe(self, z, x, y, g):
        result = Recipe()

        try:
            loc = self.lookup(z, x, y, g)
        except FileFormatException:
            sys.stderr.write("============\noverlay.db has the wrong format!!\n===========\n")
            sys.exit(2)

        if loc == None:
            return result # empty recipe
        pos = -1
        while True:
            b = self.read_8(loc)
            opc = (b >> 4) & 0xf
            if opc < 2:
                x, y = self.posxy(pos)
                angle = b & 0x1f
                result.add_long_vane(x, y, angle)
                loc += 1
            elif 2 <= opc <= 3:
                x, y = self.posxy(pos)
                angle = b & 0x1f
                result.add_short_vane(x, y, angle)
                loc += 1
            elif 4 <= opc <= 5: # sector
                angle = b & 0x1f
                size, colour = self.size_col(loc)
                pos += 1 # implicit skip by 1
                x, y = self.posxy(pos)
                result.add_sector(x, y, size, colour, angle)
                loc += 2
            elif 6 <= opc <= 12:
                # embedded skip
                pos += (1 + (b & 15))
                if opc == 6: # regular circle
                    size, colour = self.size_col(loc)
                    x, y = self.posxy(pos)
                    result.add_circle(x, y, size, colour)
                    loc += 2
                elif opc == 7:
                    size, colour = self.size_col(loc)
                    x, y = self.posxy(pos)
                    result.add_donut(x, y, size, colour)
                    loc += 2
                elif opc == 8:
                    eno, len, colour = self.eno_len_col(loc)
                    x, y = self.posxy(pos)
                    result.add_adjacent(x, y, colour, eno)
                    loc += 2
                elif opc == 9:
                    eno, len, colour = self.eno_len_col(loc)
                    x, y = self.posxy(pos)
                    caption = self.read_8_multi(loc+2, len).decode()
                    result.add_tower(x, y, colour, eno, caption)
                    loc += 2 + len
                elif opc == 10:
                    x, y = self.posxy(pos)
                    result.add_multi(x, y)
                    loc += 1
                elif opc == 11:
                    x, y = self.posxy(pos)
                    result.add_null(x, y)
                    loc += 1
                elif opc == 12:
                    loc += 1 # plain skip code
            elif opc == 15:
                opc2 = b & 15
                if opc2 == 0: # wide skip
                    pos += self.read_8(loc+1)
                    loc += 2
                elif opc2 == 15: # end of stream
                    break
                else:
                    sys.stderr.write("Oops, got subcode %d in opcode 15\n" % (opc2,))
                    sys.exit(2)
            else:
                sys.stderr.write("Oops, got unrecognized opcode %d\n" % (opc,))
                sys.exit(2)

        return result

