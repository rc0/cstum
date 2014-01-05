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

# Two screens is the default
set one 0

set config_name "default_viewerc_config.dat"
set config_in_name $config_name
set i 0
while {$i < $argc} {
    set a [lindex $argv $i]
    if [string equal $a "-1"] {
        set one 1
    } else {
        set config_in_name $a
    }
    incr i
}
set H 960
if {$one} {
    set TW 250
    set W [expr 256*7]
    set xlimit 8
} else {
    set TW 110
    set W [expr 128*7]
    set xlimit 4
}

set MAX_ZOOM 16
set MIN_ZOOM 7

array set captions {
    2g 2G
    3g 3G
    m2g {2G todo}
    m3g {3G todo}
    osm {OpenStreetMap OSM}
}

array set tile_paths {
    2g tc/2g
    3g tc/3g
    m2g tc/m2g
    m3g tc/m3g
    osm ../osm
}

array set overlay_keys {
    2g 2
    3g 3
    m2g 2
    m3g 3
    osm 0
}

array set missing_flags {
    2g 0
    3g 0
    m2g 1
    m3g 1
    osm -1
}

array set suffix_paths {
    2g .tile
    3g .tile
    m2g .tile
    m3g .tile
    osm ""
}

set source_tiles "../osm"

frame .l -width $W
label .l.l0 -width 100 -textvariable posL -anchor w
text .l.l1 -width $TW -height 1
.l.l1 tag conf nil -fore "#f00000"
.l.l1 tag conf emerg -fore "#c09000"
.l.l1 tag conf ambig -fore "#a0a0a0"
.l.l1 tag conf nomatch -fore blue
.l.l1 tag conf asu -fore magenta
canvas .l.c -width $W -height $H
entry .l.e -width 50
pack .l.l0 -side top
pack .l.l1 -side top
pack .l.c -side top
pack .l.e -side top
pack .l -side left

if {!$one} {
    canvas .c -width 32  -height $H
    frame .r -width $W
    label .r.l0 -width 100 -textvariable posR -anchor w
    text .r.l1 -width $TW -height 1
    .r.l1 tag conf nil -fore "#f00000"
    .r.l1 tag conf emerg -fore "#c09000"
    .r.l1 tag conf ambig -fore "#a0a0a0"
    .r.l1 tag conf nomatch -fore blue
    .r.l1 tag conf asu -fore magenta
    canvas .r.c -width $W -height $H
    entry .r.e -width 50
    pack .r.l0 -side top
    pack .r.l1 -side top
    pack .r.c -side top
    pack .r.e -side top
    pack .c -side left
    pack .r -side right
    .c create rectangle 0 0 32 $H -fill grey
}

# ---------------------------

source [format "%s/%s" [file dirname $argv0] common.tcl]

# ---------------------------

proc lookup_image {gen z x y} {
    global imc
    global load_direct
    global tile_paths suffix_paths
    global source_tiles
    set tmp "temp.pnm"
    set base_path $tile_paths($gen)
    set suffix $suffix_paths($gen)
    set path [format "%s/%d/%d/%d.png%s" $base_path $z $x $y $suffix]
    set bare_path [format "%s/%d/%d/%d.png" $source_tiles $z $x $y]
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
            if {$z > 7} {
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

proc RedrawOne {canv gen zoom xtl ytl} {
    global xlimits
    global xlimit

    set xx [expr $xtl >> (28 - ($zoom + 8))]
    set yy [expr $ytl >> (28 - ($zoom + 8))]
    set xtile [expr $xx >> 8]
    set ytile [expr $yy >> 8]
    set xscr [expr - ($xx & 255)]
    set yscr [expr - ($yy & 255)]

    $canv delete foobar

    for {set i 0} {$i <= $xlimit} {incr i} {
        for {set j 0} {$j <= 5} {incr j} {
            set XT [expr $xtile + $i]
            set YT [expr $ytile + $j]
            set x0 [expr $xscr + ($i << 8)]
            set y0 [expr $yscr + ($j << 8)]
            set IM [lookup_image $gen $zoom $XT $YT]
            if [lindex $IM 0] {
                $canv create image $x0 $y0 -image [lindex $IM 2] -anchor nw -tag foobar
            } else {
                set x1 [expr $x0 + 256]
                set y1 [expr $y0 + 256]
                $canv create rectangle $x0 $y0 $x1 $y1 -fill grey -tag foobar
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
                $canv create line $xx0 $yy0 $xx1 $yy0 $xx1 $yy1 $xx0 $yy1 $xx0 $yy0 -width 6 -dash "12 6" -fill $colour -tag foobar
            }
        }
    }
}

proc Redraw {} {
    global zoom xc yc W H
    global left right
    global one
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    RedrawOne .l.c $left $zoom $xtl $ytl
    DoCover .l left
    if {!$one} {
        RedrawOne .r.c $right $zoom $xtl $ytl
        DoCover .r right
    }
}

proc default_config {} {
    global zoom xc yc
    global left right
    set XB 4030
    set YB 2729

    set zoom 13
    set xc [expr $XB << (28 - $zoom)]
    set yc [expr $YB << (28 - $zoom)]
    set left 2g
    set right 3g
}

proc load_config {} {
    global zoom xc yc
    global left right
    global config_in_name
    if [file exists $config_in_name] {
        set chan [open $config_in_name r]
        if {[gets $chan line] >= 0} {
            set zoom [lindex $line 0]
            set xc [lindex $line 1]
            set yc [lindex $line 2]
            set left [lindex $line 3]
            set right [lindex $line 4]
        } else {
            default_config
        }
        close $chan
    } else {
        default_config
    }
}

proc load_towers {} {
    global towx towy
    array set towx {}
    array set towy {}
    set cidxy2 "out/cidxy2.txt"
    if [file exists $cidxy2] {
        set chan [open $cidxy2 r]
        while {[gets $chan line] >= 0} {
            set cid [lindex $line 0]
            set lac [lindex $line 1]
            set x [lindex $line 2]
            set y [lindex $line 3]
            set towx($lac/$cid) $x
            set towy($lac/$cid) $y
        }
    }
}

proc write_config {} {
    global config_name
    global left right
    global xc yc zoom
    set chan [open $config_name w]
    puts $chan [format "%d %d %d %s %s" $zoom $xc $yc $left $right]
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

proc build_cid_loc {zoom xtile ytile sx sy cids} {
    global cidmap
    foreach x $cids {
        if [regexp {^(\d+)/(\d+):(\d+)} $x foo lac cid freq] {
            set key $zoom,$xtile,$ytile,$cid
            if {![info exists cidmap($key)]} {
                set cidmap($key) [list]
            }
            lappend cidmap($key) [list $sx $sy]
        }
    }
}

proc inhale_cover {zoom data_path} {
    global cover
    global asu
    global score
    if [file exists $data_path] {
        set fid [open $data_path r]
        while {[gets $fid line] >= 0} {
            if {[string compare [lindex $line 0] "<<"] == 0} {
                set xtile [lindex $line 1]
                set ytile [lindex $line 2]
            } elseif {[string equal [lindex $line 0] "*2"]} {
                set gen 2g
                set key $gen,$zoom,$xtile,$ytile
                set score($key) [lrange $line 1 end]
            } elseif {[string equal [lindex $line 0] "*3"]} {
                set gen 3g
                set key $gen,$zoom,$xtile,$ytile
                set score($key) [lrange $line 1 end]
            } elseif {[string compare [lindex $line 0] ">>"] != 0} {
                set gen [lindex $line 0]
                append gen g
                set sx [lindex $line 1]
                set sy [lindex $line 2]
                set this_asu [lindex $line 4]
                set rank [lindex $line 5]
                set cids [lrange $line 6 end]
                set key $gen,$zoom,$xtile,$ytile,$sx,$sy
                set cover($key) $cids
                set asu($key) $this_asu
                build_cid_loc $zoom $xtile $ytile $sx $sy $cids
            }
        }
        close $fid
    }
}

proc cover_path {zoom xtile ytile} {
    if {$zoom <= 12} {
        return [format "out/level_%s.dat" $zoom]
    } else {
        set x12 [expr $xtile >> ($zoom - 12)]
        set y12 [expr $ytile >> ($zoom - 12)]
        return [format "out/lev12/%d/%d/level_%s.dat" $x12 $y12 $zoom]
    }
}

proc lookup_cover {gen zoom xtile ytile sx sy} {
    global cover
    global asu
    global inhaled_cover
    set data_path [cover_path $zoom $xtile $ytile]
    if {![info exists inhaled_cover($data_path)]} {
        inhale_cover $zoom $data_path
        set inhaled_cover($data_path) 1
    }
    set key $gen,$zoom,$xtile,$ytile,$sx,$sy
    if [info exists cover($key)] {
        return [list $cover($key) $asu($key)]
    } else {
        return "?"
    }
}

proc lookup_subtiles {zoom xtile ytile cid} {
    global cidmap
    global inhaled_cover
    set data_path [cover_path $zoom $xtile $ytile]
    if {![info exists inhaled_cover($data_path)]} {
        inhale_cover $zoom $data_path
        set inhaled_cover($data_path) 1
    }
    set key $zoom,$xtile,$ytile,$cid
    if [info exists cidmap($key)] {
        return $cidmap($key)
    } else {
        return [list]
    }
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

proc draw_cover {canv c xmouse ymouse xtl ytl zoom} {
    global towx towy
    set pixel_shift [expr 20 - $zoom]
    $canv delete towerline
    foreach x $c {
        if [regexp {^(\d+/\d+):(\d+)} $x foo lc freq] {
            if [info exists towx($lc)] {
                set x28 $towx($lc)
                set y28 $towy($lc)
                draw_one $canv $xmouse $ymouse [expr ($x28 - $xtl) >> $pixel_shift] [expr ($y28 - $ytl) >> $pixel_shift] $freq $zoom
            }
        }
    }
}

proc depict_coverage {field which zoom xtile ytile sx sy} {
    global captions
    global towx
    set foo [lookup_cover $which $zoom $xtile $ytile $sx $sy]
    set data [lindex $foo 0]
    set asu [lindex $foo 1]
    $field delete 1.0 end
    $field insert end [format "%s:" $captions($which)]
    if [string equal $asu ""] {
    } else {
        set bars "     "
        if {$asu > 15} {
            set bars " ||||"
        } elseif {$asu > 7} {
            set bars " ||| "
        } elseif {$asu > 3} {
            set bars " ||  "
        } elseif {$asu > 1} {
            set bars " |   "
        }
        $field insert end [format " <%2d%s>" $asu $bars] asu
    }
    foreach x $data {
        $field insert end " "
        if [regexp {^(\d+/\d+):(\d+)} $x foo lc freq] {
            if {[string compare "0/0" $lc] == 0} {
                $field insert end "EMERGENCY/ONLY:" emerg
                $field insert end $freq emerg
            } elseif {[string compare "65535/0" $lc] == 0} {
                $field insert end "NO/COVERAGE:" nil
                $field insert end $freq nil
            } elseif [regexp {\d+/50594049} $lc foo] {
                $field insert end "AMBIG/AMBIG:" ambig
                $field insert end $freq ambig
            } elseif [info exists towx($lc)] {
                $field insert end $x
            } else {
                $field insert end $x nomatch
            }
        }
    }
    return $data
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
    global left right
    global W H
    global one
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
    set data [depict_coverage .l.l1 $left $zoom $xtile $ytile $sx $sy]
    draw_cover .l.c $data $x $y $xtl $ytl $zoom
    if {!$one} {
        set posR [format "%s" $foo]
        set data [depict_coverage .r.l1 $right $zoom $xtile $ytile $sx $sy]
        draw_cover .r.c $data $x $y $xtl $ytl $zoom
    }

    set msize 15
    set xx0 [expr $x - $msize]
    set xx1 [expr $x + $msize]
    set yy0 [expr $y - $msize]
    set yy1 [expr $y + $msize]
    .l.c delete marker
    .l.c create oval $xx0 $yy0 $xx1 $yy1 -tags marker -outline #f00 -width 3
    if {!$one} {
        .r.c delete marker
        .r.c create oval $xx0 $yy0 $xx1 $yy1 -tags marker -outline #f00 -width 3
    }
}

proc SetLeft {foo} {
    global left
    set left $foo
    DoStep 0 0
}

proc SetRight {foo} {
    global right
    set right $foo
    DoStep 0 0
}

proc DoCover {base side} {
    global W H
    global zoom xc yc
    global xlimit
    set canv $base
    append canv .c
    set ent $base
    append ent .e
    set re [$ent get]
    $canv delete coverage
    set z20 [expr 20 - $zoom]
    set z28 [expr 28 - $zoom]
    set xtl [expr $xc - (($W >> 1) << $z20)]
    set ytl [expr $yc - (($H >> 1) << $z20)]
    set xtile [expr $xtl >> $z28]
    set ytile [expr $ytl >> $z28]
    set colours [list [list "#ec0" "#e40"] [list "#d4f" "#40f"] [list "#0ce" "#04f"]]
    set cycle 0
    foreach cid $re {
        set cols [lindex $colours $cycle]
        set col0 [lindex $cols 0]
        set col1 [lindex $cols 1]
        for {set i 0} {$i <= $xlimit} {incr i} {
            for {set j 0} {$j <= 4} {incr j} {
                set XT [expr $xtile + $i]
                set YT [expr $ytile + $j]
                set subtiles [lookup_subtiles $zoom $XT $YT $cid]
                while {[llength $subtiles] > 0} {
                    set ss [lindex $subtiles 0]
                    set sx [lindex $ss 0]
                    set sy [lindex $ss 1]
                    set subtiles [lrange $subtiles 1 end]
                    set xx [expr ($XT << $z28) - $xtl]
                    set yy [expr ($YT << $z28) - $ytl]
                    set x0 [expr ($xx >> $z20) + ($sx << 4)]
                    set y0 [expr ($yy >> $z20) + ($sy << 4)]
                    set x1 [expr $x0 + 16]
                    set y1 [expr $y0 + 16]
                    $canv create oval $x0 $y0 $x1 $y1 -tags coverage -outline $col0 -width 7
                    $canv create oval $x0 $y0 $x1 $y1 -tags coverage -outline $col1 -width 3
                }
            }
        }
        set cycle [expr ($cycle + 1) % 3]
    }
    focus $canv
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

proc RenderTile {x y} {
    global last_x28 last_y28
    global tile_paths suffix_paths overlay_keys missing_flags
    global source_tiles
    global zoom
    global imc
    global left # Can only force redraw of the left map if two maps
    set z $zoom
    set xtile [expr $last_x28 >> (28 - $z)]
    set ytile [expr $last_y28 >> (28 - $z)]
    set base_path $tile_paths($left)
    set suffix $suffix_paths($left)
    set path [format "%s/%d/%d/%d.png%s" $base_path $z $xtile $ytile $suffix]
    set bare_path [format "%s/%d/%d/%d.png" $source_tiles $z $xtile $ytile]
    if [info exists overlay_keys($left)] {
        exec python3 ../cstum/tools/render_tile.py $z $xtile $ytile $overlay_keys($left) $missing_flags($left) $bare_path $path &
        array unset imc $path
    }
}

proc RenderTilesUpwards {x y} {
    global last_x28 last_y28
    global left
    global zoom
    set xtile [expr $last_x28 >> (28 - $zoom)]
    set ytile [expr $last_y28 >> (28 - $zoom)]
    set x0 [expr $xtile - 1]
    set y0 [expr $ytile - 1]
    set x1 [expr $xtile + 1]
    set y1 [expr $ytile + 1]
    exec python3 ../cstum/tools/render_many_tiles.py $zoom $x0 $y0 $x1 $y1 1 $left &
}

proc RenderManyTiles {x y recurse} {
    global zoom xc yc W H
    global left
    global xlimit
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    set xx [expr $xtl >> (28 - ($zoom + 8))]
    set yy [expr $ytl >> (28 - ($zoom + 8))]
    set x0 [expr $xx >> 8]
    set y0 [expr $yy >> 8]
    set x1 [expr $x0 + $xlimit]
    set y1 [expr $y0 + 5]
    exec python3 ../cstum/tools/render_many_tiles.py $zoom $x0 $y0 $x1 $y1 $recurse $left &
}

proc BlastRegion {z x0 y0 x1 y1} {
    global tile_paths suffix_paths
    global imc
    foreach g {2g 3g m2g m3g} {
        set base_path $tile_paths($g)
        set suffix $suffix_paths($g)
        set i $x0
        while {$i <= $x1} {
            set j $y0
            while {$j <= $y1} {
                set path [format "%s/%d/%d/%d.png%s" $base_path $z $i $j $suffix]
                file delete $path
                array unset imc $path
                incr j 1
            }
            incr i 1
        }
    }
}

proc BlastTilesInView {x y} {
    global zoom xc yc W H
    global left
    global xlimit
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    set xx [expr $xtl >> (28 - ($zoom + 8))]
    set yy [expr $ytl >> (28 - ($zoom + 8))]
    set x0 [expr $xx >> 8]
    set y0 [expr $yy >> 8]
    set z $zoom
    set x1 [expr $x0 + $xlimit]
    set y1 [expr $y0 + 5]
    set xx0 $x0
    set xx1 $x1
    set yy0 $y0
    set yy1 $y1
    # First zoom out
    while {$z >= 7} {
        BlastRegion $z $xx0 $yy0 $xx1 $yy1
        set xx0 [expr $xx0 >> 1]
        set yy0 [expr $yy0 >> 1]
        set xx1 [expr $xx1 >> 1]
        set yy1 [expr $yy1 >> 1]
        incr z -1
    }
    # Now zoom in
    set xx0 $x0
    set xx1 $x1
    set yy0 $y0
    set yy1 $y1
    set z $zoom
    while {$z <= 16} {
        BlastRegion $z $xx0 $yy0 $xx1 $yy1
        set xx0 [expr $xx0 << 1]
        set yy0 [expr $yy0 << 1]
        set xx1 [expr 1 + ($xx1 << 1)]
        set yy1 [expr 1 + ($yy1 << 1)]
        incr z 1
    }
}

proc ZotzBaseTile {x y} {
    global last_x28 last_y28
    global overlay_keys missing_flags
    global tile_paths suffix_paths
    global source_tiles
    global zoom
    global imc
    global left # Can only force redraw of the left map if two maps
    set xtile [expr $last_x28 >> (28 - $zoom)]
    set ytile [expr $last_y28 >> (28 - $zoom)]
    # Compute the range of tiles at level 16
    set shift [expr 16 - $zoom]
    set x0 [expr $xtile << $shift]
    set y0 [expr $ytile << $shift]
    set x1 [expr $x0 | ((1 << $shift) - 1)]
    set y1 [expr $y0 | ((1 << $shift) - 1)]
    set z 16
    set z0 [expr $zoom - 3]
    while {$z >= $z0} {
        set i $x0
        while {$i <= $x1} {
            set j $y0
            while {$j <= $y1} {
                set bare_path [format "%s/%d/%d/%d.png" $source_tiles $z $i $j]
                if [file exists $bare_path] {
                    file delete -force $bare_path
                    foreach g {2g 3g m2g m3g} {
                        set base_path $tile_paths($g)
                        set suffix $suffix_paths($g)
                        set path [format "%s/%d/%d/%d.png%s" $base_path $z $i $j $suffix]
                        if [file exists $path] {
                            file delete -force $path
                        }
                        array unset imc $path
                    }
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

proc WriteXlimits {} {
    global xlimits
    puts "-------------"
    foreach key [array names xlimits] {
        puts $key
    }
    puts "-------------"
}

proc RefreshTiles {} {
    global imc
    array unset imc
    Redraw
}

bind . a {SetLeft 2g}
bind . s {SetLeft 3g}
bind . d {SetLeft m2g}
bind . f {SetLeft m3g}
bind . c {SetLeft osm}
bind .l.e <Key-Return> {DoCover .l left}

if {!$one} {
    bind . j {SetRight 2g}
    bind . k {SetRight 3g}
    bind . l {SetRight m2g}
    bind . ";" {SetRight m3g}
    bind . "m" {SetRight osm}
    bind .r.e <Key-Return> {DoCover .r right}
}

bind . q {DoQuit}
bind . <Control-q> {DoQuit}
bind . "#" {RenderTile %x %y}
bind . "^" {RenderTilesUpwards %x %y}
bind . "~" {RenderManyTiles %x %y 0}
bind . "$" {RenderManyTiles %x %y 2}
bind . "B" {BlastTilesInView %x %y}
bind . "Z" {ZotzBaseTile %x %y}
bind . "w" {WriteXlimits}
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
load_towers
Redraw

# vim:et:sts=4:sw=4

