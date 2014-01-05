

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


# optimise the limits file by coalescing regions and removing redundant ones

import re
import sys

total = 0

subs = [(i,j) for i in [0,1] for j in [0,1]]

class Node():
    def __init__(self, z, x, y):
        global total
        self.c = [[None, None], [None, None]]
        self.z = z
        self.x = x
        self.y = y
        self.active = 0
        self.allow = set()
        total += 1

def lookup(z, x, y):
    global top
    cursor = top
    zz = z
    while z > 0:
        s = z - 1
        i = (x >> s) & 1
        j = (y >> s) & 1
        if cursor.c[i][j] == None:
            cursor.c[i][j] = Node(zz-s, x>>s, y>>s)
        cursor = cursor.c[i][j]
        z = z - 1
    return cursor

def activate(n, levels):
    n.active = 1
    if levels > 0:
        for i, j in subs:
            nn = n.c[i][j]
            if nn == None:
                nn = n.c[i][j] = Node(n.z+1, (n.x<<1)+i, (n.y<<1)+j)
            activate(nn, levels-1)


def gather(z1, x, y, z2):
    n = lookup(z1, x, y)
    activate(n, z2-z1)


def show_tree(n):
    print ("%2d %5d %5d %1d %s" % (n.z, n.x, n.y, n.active, str(n.allow)))
    for i, j in subs:
        nn = n.c[i][j]
        if nn != None:
            show_tree(nn)

def traverse1(n):
    # Build a set 'allow' in each node.  This is the set of all levels at and
    # below this node which could be expanded here.  i.e. if a level is in this
    # set, it means every descendent at that level is active
    below = None
    for i, j in subs:
        nn = n.c[i][j]
        # ASSUME if nn exists, it is active.  i.e. if you are showing a zoomed-in tile,
        # you want every ancestor above it to be in the active set
        if nn != None:
            traverse1(nn)
            below = nn.allow if below == None else below & nn.allow
        else:
            # Missing child means the other children have to be roots of new expansions
            below = set()
    n.allow = below | {n.z}

def traverse2(n, active):
    available = n.allow - active
    if len(available) > 0:
        choice = max(available)
        print ("%2d %5d %5d %2d" % (n.z, n.x, n.y, choice))
        active = active | available

    for child in [nn for nn in [n.c[i][j] for i, j in subs] if nn != None]:
        traverse2(child, active)

top = Node(0, 0, 0)

with open("limits.txt", "r") as handle:
    for line in handle:
        line = line.strip()
        if line.startswith('#'):
            continue
        fields = line.split()
        if len(fields) == 4:
            f = [int(x) for x in fields]
            gather(*tuple(f))

traverse1(top)
traverse2(top, set())

#show_tree(top)

