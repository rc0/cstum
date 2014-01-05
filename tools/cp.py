"""

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

Implement a tool for interactively choosing the colours
used for filling in the towers and their coverages
"""

import tkinter
import math
import sys
import re

WIDTH = 720
PADX = 100
HEIGHT = 300
PADY = 100
SIZE = 20

DARK_THRESH = 0.3
LIGHT_THRESH = 0.7

def grid(canv):
    for foo in [0.0,0.5,1.0]:
        y = PADY + int(foo*HEIGHT)
        canv.create_line(PADX,y,PADX+WIDTH,y,width=2)
    for foo in [LIGHT_THRESH, DARK_THRESH]:
        y = PADY + int((1.0 - foo) * HEIGHT)
        canv.create_line(PADX,y,PADX+WIDTH,y)
    for foo in [0,1,2,3,4,5,6]:
        x = PADX + int(WIDTH*foo*60/360)
        y = PADY + int(foo*HEIGHT)
        canv.create_line(x,PADY,x,PADY+HEIGHT,width=2)

class Slave(tkinter.Canvas):
    def __init__(self,master):
        tkinter.Canvas.__init__(self,master)
        total_width = WIDTH + 2*PADX
        total_height = HEIGHT + 2*PADY
        self.configure({'width':total_width, 'height':total_height})
        grid(self)

class Selector(tkinter.Canvas):
    def __init__(self,master,slave,start_colours):
        tkinter.Canvas.__init__(self,master)
        total_width = WIDTH + 2*PADX
        total_height = HEIGHT + 2*PADY
        self.configure({'width':total_width, 'height':total_height})
        self.slave = slave

        grid(self)

        self.nodes = {}
        self.node_to_slave = {}

        for col in start_colours:
            self.display(col)

        self.bind('<Return>', self.finished)
        self.bind('q', self.finished)
        self.bind('Q', self.quiet_finished)
        self.bind('+', self.new_colour)
        self.bind('a', self.new_colour)
        self.focus_set()

    def display(self, col):
        hue, lightness = col
        r, g, b = get_rgb(hue, 1.0, lightness)
        hc = get_hexcol(r, g, b)
        x = PADX + WIDTH * (hue / 360.0)
        y = PADY + (1.0 - lightness) * HEIGHT
        id = self.create_oval(x-SIZE,y-SIZE,x+SIZE,y+SIZE,fill=hc,width="2",tags="pts")
        ar, ag, ab = blend(r, g, b)
        shc = get_hexcol(ar, ag, ab)
        sid = self.slave.create_oval(x-SIZE,y-SIZE,x+SIZE,y+SIZE,fill=shc,width="2",tags="pts")

        self.tag_bind(id, '<ButtonPress-1>', self.start_move)
        self.tag_bind(id, '<B1-Motion>', self.do_motion)
        self.tag_bind(id, '<B1-ButtonRelease>', self.do_release)
        self.tag_bind(id, '<ButtonPress-3>', self.zotz_me)
        self.nodes[id] = (hue,lightness,r,g,b,hc)
        self.node_to_slave[id] = sid

    def zotz_me(self,event):
        cx = self.canvasx(event.x)
        cy = self.canvasx(event.y)
        id, = self.find_closest(cx, cy)
        self.delete(id)
        self.slave.delete(self.node_to_slave[id])
        del self.node_to_slave[id]
        del self.nodes[id]

    def start_move(self,event):
        cx = self.canvasx(event.x)
        cy = self.canvasx(event.y)
        id, = self.find_closest(cx, cy)
        self.oldx, self.oldy = cx, cy
        self.move_id =id

    def do_motion(self,event):
        cx = self.canvasx(event.x)
        cy = self.canvasx(event.y)
        dx = int(cx - self.oldx)
        dy = int(cy - self.oldy)
        self.oldx, self.oldy = cx, cy
        self.move(self.move_id, dx, dy)
        self.slave.move(self.node_to_slave[self.move_id], dx, dy)
        x0, y0, x1, y1 = self.coords(self.move_id)
        xx = (x0 + x1) * 0.5
        yy = (y0 + y1) * 0.5
        hue = 360.0 * ((xx - PADX) / WIDTH)
        lightness = 1.0 - ((yy - PADY) / HEIGHT)
        if (hue < 0.0): hue = 0.0
        if (hue >= 360.0): hue = 360.0
        if (lightness < 0.0): lightness = 0.0
        if (lightness > 1.0): lightness = 1.0
        r, g, b = get_rgb(hue, 1.0, lightness)
        hc = get_hexcol(r, g, b)
        self.itemconfigure(self.move_id,fill=hc)
        ar, ag, ab = blend(r, g, b)
        shc = get_hexcol(ar, ag, ab)
        self.slave.itemconfigure(self.node_to_slave[self.move_id],fill=shc)
        self.nodes[self.move_id] = (hue,lightness,r,g,b,hc)

    def do_release(self,event):
        pass

    def new_colour(self,event):
        self.display((180.0,0.5))

    def finished(self, event):
        medium = []
        light = []
        dark = []
        unordered = []
        for idx in self.nodes:
            c = self.nodes[idx]
            h,l,r,g,b,x = c
            if (l < DARK_THRESH):
                dark.append(c)
            elif (l < LIGHT_THRESH):
                medium.append(c)
            else:
                light.append(c)
        # hue comes first in the tuples
        medium = sorted(medium)
        light = sorted(light)
        dark = sorted(dark)
        in_order = medium + light + dark
        for c in in_order:
            h,l,r,g,b,x = c
            print ("{0:7.1f} {1:6.3f} {2:6.3f} {3:6.3f} {4:6.3f} {5:s}".format(h,l,r,g,b,x))
        foo = []
##         for c in in_order:
##             h,l,r,g,b,x = c
##             foo.append('({0:.1f},{1:.3f})'.format(h,l))
##         print ('[' + ','.join(foo) + ']')
        root.destroy()

    def quiet_finished(self,event):
        root.destroy()

# ----------------------------------------------------

def clamp(a):
    if (a < 0.0): a = 0.0
    if (a > 1.0): a = 1.0
    return a

# ----------------------------------------------------

def blend(r, g, b):
    alpha = 0.75
    ar = r*alpha + 1.0*(1-alpha)
    ag = g*alpha + 1.0*(1-alpha)
    ab = b*alpha + 1.0*(1-alpha)
    return ar, ag, ab

# ----------------------------------------------------

def get_rgb(H,S,L):
    hd = H / 60.0
    C = (1.0 - math.fabs(2.0*L - 1.0)) * S;
    if   hd < 1.0: r, g, b = C, C*hd, 0.0
    elif hd < 2.0: r, g, b = C*(2.0-hd), C, 0.0
    elif hd < 3.0: r, g, b = 0.0, C, C*(hd-2.0)
    elif hd < 4.0: r, g, b = 0.0, C*(4.0-hd), C
    elif hd < 5.0: r, g, b = C*(hd-4.0), 0.0, C
    else:          r, g, b = C, 0.0, C*(6.0-hd)
    m = L - (0.30*r + 0.59*g + 0.11*b)
    r += m
    g += m
    b += m
    r = clamp(r)
    g = clamp(g)
    b = clamp(b)
    return r, g, b

# ----------------------------------------------------

def get_hexcol(r, g, b):
    return "#{0:02X}{1:02X}{2:02X}".format(int(255.0*r), int(255.0*g), int(255.0*b))

# ----------------------------------------------------

start_colours = []

try:
    handle = open(sys.argv[1], 'r')
except:
    print ('Could not open an input file')
    handle = None

if handle != None:
    try:
        for line in handle:
            line = line.rstrip()
            line = line.lstrip()
            hue,lightness,*rest = re.split(r' +', line)
            foo = float(hue), float(lightness)
            start_colours.append(foo)
    except:
        print ('Syntax error in input file')

root = tkinter.Tk()
slave = Slave(root)
canv = Selector(root, slave, start_colours)
slave.pack(side=tkinter.TOP)
canv.pack(side=tkinter.TOP)
tkinter.mainloop()

