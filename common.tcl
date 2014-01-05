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

proc osgrid {x y} {
    set alpha [expr 61.000 * ($x - 0.4944400930)]
    set beta  [expr 36.000 * ($y - 0.3126638550)]
    if {$alpha < -1.0} {
        return "?"
    }
    if {$alpha > 1.0} {
        return "?"
    }
    if {$beta < -1.0} {
        return "?"
    }
    if {$beta > 1.0} {
        return "?"
    }
    set alpha2 [expr $alpha * $alpha]
    set beta2 [expr $beta * $beta]
    set beta4 [expr $beta2 * $beta2]
    set t90 [expr (400001.47) + (-17.07) * $beta]
    set t91 [expr (370523.38) + (53326.92) * $beta]
    set t92 [expr (2025.68) + (-241.27) * $beta]
    set t93 [expr $t91 + $t92 * $beta2]
    set t94 [expr $t93 + (-41.77) * $beta4]
    set t95 [expr $t90 + $t94 * $alpha]
    set t96 [expr (-11.21) * $beta]
    set t97 [expr $t96 + (14.84) * $beta2]
    set t98 [expr (-237.68) + (82.89) * $beta]
    set t99 [expr $t98 + (41.21) * $beta2]
    set t100 [expr $t97 + $t99 * $alpha]
    set E [expr $t95 + $t100 * $alpha2]
    set t101 [expr (649998.33) + (-13.90) * $alpha]
    set t102 [expr $t101 + (15782.38) * $alpha2]
    set t103 [expr (-626496.42) + (1220.67) * $alpha2]
    set t104 [expr $t102 + $t103 * $beta]
    set t105 [expr (-44898.11) + (10.01) * $alpha]
    set t106 [expr $t105 + (-217.21) * $alpha2]
    set t107 [expr (-1088.27) + (-49.59) * $alpha2]
    set t108 [expr $t106 + $t107 * $beta]
    set t109 [expr $t104 + $t108 * $beta2]
    set N [expr $t109 + (107.47) * $beta4]

    set e [expr int(0.5 + $E) / 10]
    set n [expr int(0.5 + $N) / 10]
    set e0 [expr $e / 10000]
    set n0 [expr $n / 10000]
    set e1 [expr $e % 10000]
    set n1 [expr $n % 10000]
    set c0 [string index "SNHTOJ" [expr 3*($e0/5) + ($n0/5)]]
    set c1 [string index "VQLFAWRMGBXSNHCYTOJDZUPKE" [expr 5*($e0%5) + ($n0%5)]]
    return [format "%1s%1s %04d %04d" $c0 $c1 $e1 $n1]
}

proc pi {} {
    return {3.141592653589793}
}

proc rad2deg {x} {
    return [expr $x * (180.0/[pi])]
}

proc deg2rad {x} {
    return [expr $x * ([pi]/180.0)]
}

proc latlon {x y} {
    set lon [rad2deg [expr [pi] * (2.0*$x - 1.0)]]
    set t [expr (1.0 - 2.0*$y) * [pi]]
    set lat [rad2deg [expr 2.0*atan(exp($t)) - 0.5*[pi]]]
    return [format "%.5f %.5f" $lat $lon]
}


proc ZoomIn {} {
    global zoom
    global last_x28 last_y28
    global xc yc
    global MAX_ZOOM
    if {$zoom < $MAX_ZOOM} {
        set delta [expr $last_x28 - $xc]
        set xc [expr $last_x28 - ($delta >> 1)]
        set delta [expr $last_y28 - $yc]
        set yc [expr $last_y28 - ($delta >> 1)]
        incr zoom +1
        Redraw
    }
}

proc ZoomOut {} {
    global zoom
    global last_x28 last_y28
    global xc yc
    global MIN_ZOOM
    if {$zoom > $MIN_ZOOM} {
        set delta [expr $last_x28 - $xc]
        set xc [expr $last_x28 - ($delta << 1)]
        set delta [expr $last_y28 - $yc]
        set yc [expr $last_y28 - ($delta << 1)]
        incr zoom -1
        Redraw
    }
}

# vim:et:sts=4:sw=4

