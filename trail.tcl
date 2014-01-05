#!/usr/bin/wish

if {[string compare "8.6" [info tclversion]] == 0} {
    set load_direct 1
} elseif [catch {package require Img}] {
    puts "No Img package, using emulation for version [info tclversion]"
    set load_direct 0
} else {
    # Img package loaded
    set load_direct 1
}

set trail_file [lindex $argv 0]
if {[file exists $trail_file] == 0} {
    puts stderr "File $trail_file does not exist, giving up"
    exit 1
}

set config_name "default_trail_config.dat"
set H 896
set W [expr 256*7]
set colours [list \
        "#FF2C2C" "#00D000" "#8747FF" "#D97800" "#08D6D2" "#FF30F6" "#96A600" "#0E7EDD" \
        "#FF9A8C" "#91E564" "#F98CFF" \
        "#5E0857" "#5A2C00" "#084500" "#18256B" \
     ]
array set active_towers {}

frame .l -width $W
label .l.l0 -width 120 -textvariable posL -anchor w
canvas .l.c -width $W -height $H
pack .l.l0 -side top
pack .l.c -side top
pack .l -side left

# ---------------------------

source [format "%s/%s" [file dirname $argv0] common.tcl]

# ---------------------------

proc lookup_image {z x y} {
    global imc
    global load_direct
    set tmp  "temp.pnm"
    set path [format "../osm/%d/%d/%d.png" $z $x $y]
    if [info exists imc($path)] {
        return $imc($path)
    } else {
        if [file exists $path] {
            if {$load_direct} {
                set im [image create photo -file $path -format png -width 256 -height 256]
            } else {
                # clunky workaround with netpbm...
                exec pngtopam $path > $tmp
                set im [image create photo -file $tmp -width 256 -height 256]
            }
            set imc($path) $im
            return $im
        } else {
            return blank
        }
    }
}

proc render_trail {canv zoom xtl ytl} {
    global trail
    global towcol towx towy colours
    $canv delete trail
    $canv delete towline
    foreach seg $trail {
        set idx [lindex $seg 0]
        set line [lindex $seg 1]
        set coords {}
        foreach point $line {
            set x [lindex $point 0]
            set y [lindex $point 1]
            set xx [expr ($x - $xtl) >> (20 - $zoom)]
            set yy [expr ($y - $ytl) >> (20 - $zoom)]
            lappend coords $xx
            lappend coords $yy
        }
        set ci -1
        set xt -1
        set yt -1
        if {$idx >= 0} {
            set ci $towcol($idx)
            set xt $towx($idx)
            set yt $towy($idx)
        } else {
            set ci -1
        }
        if {$ci >= 1} {
            set col [lindex $colours $ci]
        } else {
            set col [lindex $colours 0]
        }
        $canv create line $coords -width 8 -fill $col -tag trail

        if {$ci >= 1} {
            # Draw line to tower
            set a [llength $line]
            set a [expr $a >> 1]
            set point [lindex $line $a]
            set x [lindex $point 0]
            set y [lindex $point 1]
            set xx [expr ($x - $xtl) >> (20 - $zoom)]
            set yy [expr ($y - $ytl) >> (20 - $zoom)]
            set xxt [expr ($xt - $xtl) >> (20 - $zoom)]
            set yyt [expr ($yt - $ytl) >> (20 - $zoom)]
            $canv create line $xx $yy $xxt $yyt -width 2 -fill [lindex $colours $ci] -tags towline
        }

    }
}

proc render_towers {canv zoom xtl ytl} {
    global active_towers
    global towcol towx towy
    global colours
    $canv delete towers
    foreach laccid [array names active_towers] {
        if [info exists towx($laccid)] {
            set xt $towx($laccid)
            set yt $towy($laccid)
            set xx [expr ($xt - $xtl) >> (20 - $zoom)]
            set yy [expr ($yt - $ytl) >> (20 - $zoom)]
            set xx0 [expr $xx - 8]
            set xx1 [expr $xx + 8]
            set yy0 [expr $yy - 8]
            set yy1 [expr $yy + 8]
            if [info exists towcol($laccid)] {
                set col [lindex $colours $towcol($laccid)]
                $canv create oval $xx0 $yy0 $xx1 $yy1 -width 2 -outline black -fill $col -tags towers
            }
        }
    }
}

proc RedrawOne {canv zoom xtl ytl} {
    set xx [expr $xtl >> (28 - ($zoom + 8))]
    set yy [expr $ytl >> (28 - ($zoom + 8))]
    set xtile [expr $xx >> 8]
    set ytile [expr $yy >> 8]
    set xscr [expr - ($xx & 255)]
    set yscr [expr - ($yy & 255)]

    $canv delete foobar

    for {set i 0} {$i <= 8} {incr i} {
        for {set j 0} {$j <= 4} {incr j} {
            set XT [expr $xtile + $i]
            set YT [expr $ytile + $j]
            set x0 [expr $xscr + ($i << 8)]
            set y0 [expr $yscr + ($j << 8)]
            set IM [lookup_image $zoom $XT $YT]
            if {[string compare $IM "blank"] == 0} {
                set x1 [expr $x0 + 256]
                set y1 [expr $y0 + 256]
                $canv create rectangle $x0 $y0 $x1 $y1 -fill grey -tag foobar
            } else {
                $canv create image $x0 $y0 -image $IM -anchor nw -tag foobar
            }
        }
    }

    render_towers $canv $zoom $xtl $ytl
    render_trail $canv $zoom $xtl $ytl
}

proc Redraw {} {
    global zoom xc yc W H
    global left right
    set xtl [expr $xc - (($W >> 1) << (28 - ($zoom + 8)))]
    set ytl [expr $yc - (($H >> 1) << (28 - ($zoom + 8)))]
    RedrawOne .l.c $zoom $xtl $ytl
}

proc default_config {} {
    global zoom xc yc
    set XB 4030
    set YB 2729

    set zoom 13
    set xc [expr $XB << (28 - $zoom)]
    set yc [expr $YB << (28 - $zoom)]
}

proc load_config {} {
    global zoom xc yc
    global config_name
    if [file exists $config_name] {
        set chan [open $config_name r]
        if {[gets $chan line] >= 0} {
            set zoom [lindex $line 0]
            set xc [lindex $line 1]
            set yc [lindex $line 2]
        } else {
            default_config
        }
        close $chan
    } else {
        default_config
    }
}

proc load_towers {} {
    # REWRITE FOR PAINT COLOURS
    global towx towy towcol towidx
    array set towx {}
    array set towy {}
    array set towidx {}
    array set towcol {}
    set cidxy2 "out/cidxy2.txt"
    set cid_paint "out/cid_paint.txt"
    if [file exists $cid_paint] {
        set chan [open $cid_paint r]
        while {[gets $chan line] >= 0} {
            set cid [lindex $line 0]
            set lac [lindex $line 1]
            set index [lindex $line 2]
            set colour [lindex $line 3]
            set towidx($lac/$cid) $index
            set towcol($index) $colour
        }
        close $chan
    } else {
        print "No cid_paint.txt"
        exit 2
    }
    if [file exists $cidxy2] {
        set chan [open $cidxy2 r]
        while {[gets $chan line] >= 0} {
            set cid [lindex $line 0]
            set lac [lindex $line 1]
            set laccid $lac/$cid
            set x [lindex $line 2]
            set y [lindex $line 3]
            if [info exist towidx($laccid)] {
                set idx $towidx($laccid)
                set towx($idx) $x
                set towy($idx) $y
            }
        }
        close $chan
    }
}

proc write_config {} {
    global config_name
    global xc yc zoom
    set chan [open $config_name w]
    puts $chan [format "%d %d %d" $zoom $xc $yc]
    close $chan
}

proc DoQuit {} {
    write_config
    exit 0
}

proc DoStep {dx dy} {
    global zoom xc yc
    set step [expr (1 << (28 - ($zoom + 2)))]
    incr xc [expr $step * $dx]
    incr yc [expr $step * $dy]
    Redraw
}

proc ll_to_xy {lat lon} {
    set x [expr int((1<<28) * 0.5 * (1.0 + $lon/180.0))]
    set y [expr int((1<<28) * 0.5 * (1.0 - (1.0/[pi]) * log(tan(0.5*[deg2rad $lat] + 0.25*[pi]))))]
    return [list $x $y]
}


proc DoMotion {x y} {
    global zoom xc yc
    global last_x28 last_y28
    global posL
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

proc lookup_cid {cid lac} {
    global cidtow
    if [info exists cidtow($lac/$cid)] {
        return $cidtow($lac/$cid)
    } else {
        return -1
    }
}

proc simplify2 {p pe} {
    # p is a list of points to be shortened.
    # pe is the point off the end of the list.
    # each 'point' is a 2-element list [X, Y]

    if {[llength $p] < 2} {
        return $p
    }

    set p0 [lindex $p 0]
    set dx [expr [lindex $pe 0] - [lindex $p0 0]]
    set dy [expr [lindex $pe 1] - [lindex $p0 1]]
    set d [expr sqrt(($dx*$dx) + ($dy*$dy))]
    set worsti -1
    set worst 0
    if {$d > 0.0} {
        # find point with worst orthogonal distance to this line
        set uy [expr $dx / $d]
        set ux [expr -$dy / $d]
        for {set i 1} {$i < [llength $p]} {incr i} {
            set pi [lindex $p $i]
            set dx [expr [lindex $pi 0] - [lindex $p0 0]]
            set dy [expr [lindex $pi 1] - [lindex $p0 1]]
            set vp [expr abs(($dx * $ux) + ($dy * $uy))]
            if {($worsti < 0) || ($vp > $worst)} {
                set worsti $i
                set worst $vp
            }
        }
    } else {
        # A loop! Find point furthest from the common start/end
        for {set i 1} {$i < [llength $p]} {incr i} {
            set pi [lindex $p $i]
            set dx [expr [lindex $pi 0] - [lindex $p0 0]]
            set dy [expr [lindex $pi 1] - [lindex $p0 1]]
            set d [expr sqrt(($dx*$dx) + ($dy*$dy))]
            if {($worsti < 0) || ($d > $worst)} {
                set worsti $i
                set worst $d
            }
        }
    }
    set thresh 100
    if {$worst > $thresh} {
        set left [simplify2 [lrange $p 0 [expr $worsti - 1]] [lindex $p $worsti] ]
        set right [simplify2 [lrange $p $worsti end] $pe]
        return [concat $left $right]
    } else {
        # Just the endpoints
        return [lrange $p 0 0]
    }
}

proc simplify {p} {
    set left [simplify2 [lrange $p 0 end-1] [lindex $p end]]
    set result [concat $left [lrange $p end end]]
    return $result
}

proc load_trail {} {
    global trail_file
    global trail
    global active_towers
    global towidx
    set in [open $trail_file r]
    set points {}
    set last_idx -1
    set trail {}
    while {[gets $in line] >= 0} {
        if {[regexp {^#} $line] == 1} {
            continue
        }
        set lat [lindex $line 0]
        set lon [lindex $line 1]
        set cid [lindex $line 5]
        set lac [lindex $line 6]
        set laccid $lac/$cid
        if [info exists towidx($laccid)] {
            set idx $towidx($laccid)
            set active_towers($idx) 1
        } else {
            set idx -1
        }
        if {($lat != 0.0) || ($lon != 0.0)} {
            # Some 0,0 points are logged!
            set xy [ll_to_xy $lat $lon]
            set x [lindex $xy 0]
            set y [lindex $xy 1]
            if {($last_idx != $idx)} {
                if {[llength $points] > 1} {
                    lappend trail [list $last_idx [simplify $points]]
                }
                set last_idx $idx
                set points {}
            }
            lappend points $xy
        }
    }
    if {[llength $points] > 1} {
        lappend trail [list $last_idx [simplify $points]]
    }
    close $in
}

bind . q {DoQuit}
bind . <Control-q> {DoQuit}
bind . "-" {ZoomOut}
bind . "+" {ZoomIn}
bind . "=" {ZoomIn}
bind . <Key-Left> {DoStep -1 0}
bind . <Key-Right> {DoStep +1 0}
bind . <Key-Up> {DoStep 0 -1}
bind . <Key-Down> {DoStep 0 +1}
# Mouse wheel
bind . <Button-4> {DoStep 0 -1}
bind . <Button-5> {DoStep 0 +1}
bind . <Shift-Button-4> {DoStep -1 0}
bind . <Shift-Button-5> {DoStep +1 0}
bind . <Shift-Left> {DoStep -4 0}
bind . <Shift-Right> {DoStep +4 0}
bind . <Shift-Up> {DoStep 0 -4}
bind . <Shift-Down> {DoStep 0 +4}
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
load_towers
load_trail
Redraw

# vim:et:sts=4:sw=4

