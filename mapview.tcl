#!/usr/bin/wish

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

# Tool for viewing coverage maps
# The overlay.db file is used as the information source, and a cache of
# rendered tiles is built up in the tc/ subdirectory

if {[string compare "8.6" [info tclversion]] == 0} {
    set load_direct 1
} elseif [catch {package require Img}] {
    puts "No Img package, using emulation for version [info tclversion]"
    set load_direct 0
} else {
    # Img package loaded
    set load_direct 1
}

set config_name "default_mapview_config.dat"
set config_in_name $config_name
set i 0
while {$i < $argc} {
    set a [lindex $argv $i]
    if [string equal $a "-1"] {
    } else {
        set config_in_name $a
    }
    incr i
}

set H 960
set TW 250
set W [expr 256*7]
set show_missing 0

set MAX_ZOOM 18
set MIN_ZOOM 4

array set captions {
    osm {OpenStreetMap OSM}
    ocm {OpenCycleMap OCM}
}

array set tile_paths {
    osm ../osm
    ocm ../ocm
}

array set overlay_keys {
    osm 0
    ocm 0
}

array set missing_flags {
    osm -1
    ocm -1
}

array set suffix_paths {
    osm ""
    ocm ""
}

set basedir ".."

frame .l -width $W
label .l.l0 -width 100 -textvariable posL -anchor w
canvas .l.c -width $W -height $H
pack .l.l0 -side top
pack .l.c -side top
pack .l -side left

# ---------------------------

source [format "%s/%s" [file dirname $argv0] common.tcl]

# ---------------------------

proc lookup_image {gen z x y} {
    global imc
    global load_direct
    global tile_paths suffix_paths
    global basedir left
    set tmp "temp.pnm"
    set base_path $tile_paths($gen)
    set suffix $suffix_paths($gen)
    set path [format "%s/%d/%d/%d.png%s" $base_path $z $x $y $suffix]
    set bare_path [format "%s/%s/%d/%d/%d.png" $basedir $left $z $x $y]
    if [info exists imc($path)] {
        set im [lindex $imc($path) 0]
        set zoomed [lindex $imc($path) 1]
        if {$zoomed && [file exists $path]} {
            # The renderer has plugged a gap
            array unset imc $path
            # Recursion...
            return [lookup_image $gen $z $x $y]
        }
        return [list true $zoomed $im]
    } else {
        if [file exists $path] {
            if {$load_direct} {
                set im [image create photo -file $path -format png -width 256 -height 256]
            } else {
                # clunky workaround with netpbm...
                exec pngtopam $path > $tmp
                set im [image create photo -file $tmp -width 256 -height 256]
            }
            set imc($path) [list $im 0]
            return [list true 0 $im]
        } else {
            if [file exists $bare_path] {
                set zoomed 1
            } else {
                set zoomed 2
            }
            if {$z > 3} {
                set parent [lookup_image $gen [expr $z - 1] [expr $x >> 1] [expr $y >> 1]]
                if [lindex $parent 0] {
                    set im [image create photo -width 256 -height 256]
                    set ox0 [expr ($x & 1) << 7]
                    set oy0 [expr ($y & 1) << 7]
                    set ox1 [expr $ox0 + 128]
                    set oy1 [expr $oy0 + 128]
                    $im copy [lindex $parent 2] -from $ox0 $oy0 $ox1 $oy1 -zoom 2 2
                    set imc($path) [list $im $zoomed]
                    return [list true $zoomed $im]
                } else {
                    return [list false $zoomed ""]
                }
            } else {
                return [list false $zoomed ""]
            }
        }
    }
}

proc overlay_missing_children {canv depth x0 y0 zoom xtile ytile} {
    global basedir left
    global MAX_ZOOM
    if {$depth <= 3} {
        if {$zoom < $MAX_ZOOM} {
            set delta [expr 256 >> $depth]
            for {set i 0} {$i <= 1} {incr i} {
                for {set j 0} {$j <= 1} {incr j} {
                    set xx [expr $x0 + $i * $delta]
                    set yy [expr $y0 + $j * $delta]
                    set xt1 [expr ($xtile << 1) + $i]
                    set yt1 [expr ($ytile << 1) + $j]
                    set z1 [expr $zoom + 1]
                    set path [format "%s/%s/%d/%d/%d.png" $basedir $left $z1 $xt1 $yt1]
                    #puts [format "Checking %s" $path]
                    if [file exists $path] {
                        #puts [format "Checking %s : FOUND" $path]
                        overlay_missing_children $canv [expr 1 + $depth] $xx $yy $z1 $xt1 $yt1
                    } else {
                        #puts [format "Checking %s : NOT FOUND" $path]
                        set xx0 [expr $xx + 2]
                        set yy0 [expr $yy + 2]
                        set xx1 [expr $xx + $delta - 2]
                        set yy1 [expr $yy + $delta - 2]
                        $canv create line $xx0 $yy0 $xx1 $yy0 $xx1 $yy1 $xx0 $yy1 $xx0 $yy0 -fill #f00 -width [expr 4-$depth]
                    }
                }
            }
        }
    }
}

proc RedrawOne {canv gen zoom xtl ytl} {
    global xlimits
    global show_missing

    set xx [expr $xtl >> (28 - ($zoom + 8))]
    set yy [expr $ytl >> (28 - ($zoom + 8))]
    set xtile [expr $xx >> 8]
    set ytile [expr $yy >> 8]
    set xscr [expr - ($xx & 255)]
    set yscr [expr - ($yy & 255)]

    $canv delete all

    set limit 8

    for {set i 0} {$i <= $limit} {incr i} {
        for {set j 0} {$j <= 5} {incr j} {
            set XT [expr $xtile + $i]
            set YT [expr $ytile + $j]
            set x0 [expr $xscr + ($i << 8)]
            set y0 [expr $yscr + ($j << 8)]
            set IM [lookup_image $gen $zoom $XT $YT]
            if [lindex $IM 0] {
                $canv create image $x0 $y0 -image [lindex $IM 2] -anchor nw
            } else {
                set x1 [expr $x0 + 256]
                set y1 [expr $y0 + 256]
                $canv create rectangle $x0 $y0 $x1 $y1 -fill grey
            }
            set zoomed [lindex $IM 1]
            if {$zoomed > 0} {
                if {$zoomed == 2} {
                    # Base tile is missing
                    set colour "#c00"
                } else {
                    # Base tile is present, could render the tile
                    set colour "#0c0"
                }
                set xx0 [expr $x0 + 8]
                set yy0 [expr $y0 + 8]
                set xx1 [expr $x0 + 248]
                set yy1 [expr $y0 + 248]
                $canv create line $xx0 $yy0 $xx1 $yy0 $xx1 $yy1 $xx0 $yy1 $xx0 $yy0 -width 6 -dash "12 6" -fill $colour
            } elseif {$show_missing} {
                overlay_missing_children $canv 1 $x0 $y0 $zoom $XT $YT
            }
        }
    }
}

proc Redraw {} {
    global zoom xc yc W H
    global left
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    RedrawOne .l.c $left $zoom $xtl $ytl
}

proc default_config {} {
    global zoom xc yc
    global left
    set XB 4030
    set YB 2729

    set zoom 13
    set xc [expr $XB << (28 - $zoom)]
    set yc [expr $YB << (28 - $zoom)]
    set left osm
}

proc load_config {} {
    global zoom xc yc
    global left
    global config_in_name
    if [file exists $config_in_name] {
        set chan [open $config_in_name r]
        if {[gets $chan line] >= 0} {
            set zoom [lindex $line 0]
            set xc [lindex $line 1]
            set yc [lindex $line 2]
            set left [lindex $line 3]
        } else {
            default_config
        }
        close $chan
    } else {
        default_config
    }
}

proc write_config {} {
    global config_name
    global left
    global xc yc zoom
    set chan [open $config_name w]
    puts $chan [format "%d %d %d %s" $zoom $xc $yc $left]
    close $chan
}

proc DoQuit {} {
    write_config
    exit 0
}

proc DoStep {dx dy} {
    global zoom xc yc
    set step [expr (1 << (28 - ($zoom + 3)))]
    incr xc [expr $step * $dx]
    incr yc [expr $step * $dy]
    Redraw
}

proc draw_one {canv xmouse ymouse xtower ytower freq zoom} {
    if {$freq > 80} {
        set width 4
    } elseif {$freq > 50} {
        set width 2
    } else {
        set width 2
    }
    set dx [expr $xmouse - $xtower]
    set dy [expr $ymouse - $ytower]
    set m_per_pix  [expr 25200000 / (1<<($zoom+8))]
    set dist [format "%dm" [expr int($m_per_pix * sqrt($dx*$dx + $dy*$dy))]]
    set mx [expr 0.5*($xmouse + $xtower)]
    set my [expr 0.5*($ymouse + $ytower)]
    $canv create line $xmouse $ymouse $xtower $ytower -width $width -fill #f00 -tag towerline
    $canv create rectangle [expr $mx-30] [expr $my-10] [expr $mx+30] [expr $my+10] -fill #fff -tag towerline
    $canv create text $mx $my -font "lucidasanstypewriter-12" -fill #000 -text $dist -tag towerline
}

proc make_quadtile {x28 y28 zoom} {
    set out {}
    for {set i 0} {$i < $zoom} {incr i 1} {
        set mask [expr 1 << (27 - $i)]
        set foo [expr (($x28 & $mask) ? 1 : 0) | (($y28 & $mask) ? 2 : 0)]
        lappend out [format "%d" $foo]
        if {($i & 3) == 3} {
            lappend out "_"
        }
    }
    return [join $out ""]
}

proc DoMotion {x y} {
    global zoom xc yc
    global last_x28 last_y28
    global posL posR
    global left
    global W H
    set xtl [expr $xc - (($W >> 1) << (20 - $zoom))]
    set ytl [expr $yc - (($H >> 1) << (20 - $zoom))]
    set x28 [expr $xtl + ($x << (20 - $zoom))]
    set y28 [expr $ytl + ($y << (20 - $zoom))]
    set last_x28 $x28
    set last_y28 $y28
    set xtile [expr $x28 >> (28 - $zoom)]
    set ytile [expr $y28 >> (28 - $zoom)]
    set sx [expr ($x28 - ($xtile << (28 - $zoom))) >> (20 - $zoom) >> 4]
    set sy [expr ($y28 - ($ytile << (28 - $zoom))) >> (20 - $zoom) >> 4]
    set scale [expr 1.0 / (1<<28)]
    set x01 [expr $x28 * $scale]
    set y01 [expr $y28 * $scale]
    set gridref [osgrid $x01 $y01]
    set ll [latlon $x01 $y01]
    set quadtile [make_quadtile $x28 $y28 $zoom]
    set foo [format {[%s] [%s] [%s] [%d %d] [%5d %5d %2d %2d]} $ll $gridref $quadtile $x28 $y28 $xtile $ytile $sx $sy]
    set posL [format "%s" $foo]

    set msize 15
    set xx0 [expr $x - $msize]
    set xx1 [expr $x + $msize]
    set yy0 [expr $y - $msize]
    set yy1 [expr $y + $msize]
    .l.c delete marker
    .l.c create oval $xx0 $yy0 $xx1 $yy1 -tags marker -outline #f00 -width 3
}

proc canvas_bindings {canv} {
    bind $canv <Key-Left> {DoStep -1 0}
    bind $canv <Key-Right> {DoStep +1 0}
}
proc ToggleXlimits {x y} {
    # MESS : make this decode common with DoMotion
    global last_x28 last_y28
    global zoom
    global xlimits
    set xtile [expr $last_x28 >> (28 - $zoom)]
    set ytile [expr $last_y28 >> (28 - $zoom)]
    set key "$zoom $xtile $ytile $zoom"
    if [info exists xlimits($key)] {
        array unset xlimits $key
    } else {
        set xlimits($key) 1
    }
    Redraw
}

proc DownloadTile {x y dz} {
    global last_x28 last_y28
    global tile_paths suffix_paths overlay_keys missing_flags
    global basedir left
    global zoom
    global imc
    global left # Can only force redraw of the left map if two maps
    set z $zoom
    set xtile [expr $last_x28 >> (28 - $z)]
    set ytile [expr $last_y28 >> (28 - $z)]
    set base_path $tile_paths($left)
    set suffix $suffix_paths($left)
    set bare_path [format "%s/%s/%d/%d/%d.png" $basedir $left $z $xtile $ytile]
    if [info exists overlay_keys($left)] {
        exec python3 ../cstum/tools/download_tiles.py $z $xtile $ytile $xtile $ytile $dz 0 $basedir $left &
        array unset imc $bare_path
    }
}

proc DownloadTiles {x y} {
    global xc yc W H
    global tile_paths suffix_paths overlay_keys missing_flags
    global basedir left
    global zoom
    global imc
    global count
    global left # Can only force redraw of the left map if two maps
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    set x0 [expr $xtl >> (28 - $zoom)]
    set y0 [expr $ytl >> (28 - $zoom)]
    set x1 [expr $x0 + 8]
    set y1 [expr $y0 + 4]
    exec python3 ../cstum/tools/download_tiles.py $zoom $x0 $y0 $x1 $y1 0 0 $basedir $left &
    # Manually re-cohere imc by hitting 'r' now and again!
}

proc ZotzBaseTile {x y} {
    global last_x28 last_y28
    global overlay_keys missing_flags
    global tile_paths suffix_paths
    global basedir left
    global zoom
    global imc
    global left # Can only force redraw of the left map if two maps
    global MAX_ZOOM
    set xtile [expr $last_x28 >> (28 - $zoom)]
    set ytile [expr $last_y28 >> (28 - $zoom)]
    # Compute the range of tiles at level 16
    set shift [expr $MAX_ZOOM - $zoom]
    set x0 [expr $xtile << $shift]
    set y0 [expr $ytile << $shift]
    set x1 [expr $x0 | ((1 << $shift) - 1)]
    set y1 [expr $y0 | ((1 << $shift) - 1)]
    set z $MAX_ZOOM
    set z0 [expr $zoom - 3]
    while {$z >= $z0} {
        set i $x0
        while {$i <= $x1} {
            set j $y0
            while {$j <= $y1} {
                set bare_path [format "%s/%s/%d/%d/%d.png" $basedir $left $z $i $j]
                if [file exists $bare_path] {
                    file delete -force $bare_path
                }
                incr j 1
            }
            incr i 1
        }
        incr z -1
        set x0 [expr $x0 >> 1]
        set y0 [expr $y0 >> 1]
        set x1 [expr $x1 >> 1]
        set y1 [expr $y1 >> 1]
    }
}

proc WritePos28 {x y} {
    # MESS : make this decode common with DoMotion
    global last_x28 last_y28
    puts "$last_x28 $last_y28"
}

proc RefreshTiles {} {
    global imc
    array unset imc
    Redraw
}

proc ToggleShowMissing {x y} {
    global show_missing
    set show_missing [expr 1 - $show_missing]
}

proc SetLeft {foo} {
    global left
    set left $foo
    DoStep 0 0
}

bind . q {DoQuit}
bind . <Control-q> {DoQuit}
bind . v {SetLeft osm}
bind . c {SetLeft ocm}
#bind . "#" {RenderTile %x %y}
bind . "m" {ToggleShowMissing %x %y}
bind . "1" {DownloadTile %x %y 0}
bind . "2" {DownloadTile %x %y 1}
bind . "4" {DownloadTile %x %y 2}
bind . "8" {DownloadTile %x %y 3}
bind . "*" {DownloadTiles %x %y}
bind . "Z" {ZotzBaseTile %x %y}
bind . "p" {WritePos28 %x %y}
bind . "r" {RefreshTiles}
bind . "-" {ZoomOut}
bind . "+" {ZoomIn}
bind . "=" {ZoomIn}
#canvas_bindings .l.c
#canvas_bindings .r.c
bind . <Key-Left> {DoStep -1 0}
bind . <Key-Right> {DoStep +1 0}
bind . <Key-Up> {DoStep 0 -1}
bind . <Key-Down> {DoStep 0 +1}
# Mouse wheel
bind . <Button-4> {DoStep 0 -1}
bind . <Button-5> {DoStep 0 +1}
bind . <Shift-Button-4> {DoStep -1 0}
bind . <Shift-Button-5> {DoStep +1 0}
bind . <Shift-Left> {DoStep -8 0}
bind . <Shift-Right> {DoStep +8 0}
bind . <Shift-Up> {DoStep 0 -8}
bind . <Shift-Down> {DoStep 0 +8}
bind . <Key-KP_4> {DoStep -1 0}
bind . <Key-KP_6> {DoStep +1 0}
bind . <Key-KP_1> {DoStep -1 +1}
bind . <Key-KP_2> {DoStep 0 +1}
bind . <Key-KP_3> {DoStep +1 +1}
bind . <Key-KP_7> {DoStep -1 -1}
bind . <Key-KP_8> {DoStep 0 -1}
bind . <Key-KP_9> {DoStep +1 -1}
bind . <Motion> {DoMotion %x %y}

load_config
array set xlimits ""
set last_x28 $xc
set last_y28 $yc
Redraw

# vim:et:sts=4:sw=4

