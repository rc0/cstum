# A collection of drawing stuff

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


sub draw_square {
    my ($xc, $yc, $size) = @_;
    my $x0 = $xc - $size;
    my $y0 = $yc - $size;
    my $x1 = $xc + $size;
    my $y1 = $yc + $size;
    my $foo = sprintf "rectangle %d,%d %d,%d", $x0, $y0, $x1, $y1;
    return $foo;
}

sub draw_circle {
    my ($xc, $yc, $size) = @_;
    my $foo = sprintf "circle %d,%d %d,%d", $xc, $yc, $xc + $size, $yc;
    return $foo;
}

sub draw_sector {
    my ($xc, $yc, $size, $angle) = @_;
    my $span = 120;
    #my $foo1 = sprintf "ellipse %d,%d %d,%d 0,360", $xc, $yc, $size, $size;
    my $foo1 = sprintf "ellipse %d,%d %d,%d %d,%d",
      $xc, $yc, $size, $size, $angle-$span, $angle+$span;
    my $foo2 = sprintf "ellipse %d,%d %d,%d %d,%d",
      $xc, $yc, $size, $size, $angle-$span, $angle+$span;
    return ($foo1, $foo2);

}

sub draw_diamond {
    my ($xc, $yc, $size) = @_;
    my $x0 = $xc - $size;
    my $y0 = $yc - $size;
    my $x1 = $xc + $size;
    my $y1 = $yc + $size;
    my $foo = sprintf "polygon %d,%d %d,%d %d,%d %d,%d",
        $xc, $y0, $x0, $yc, $xc, $y1, $x1, $yc;
    return $foo;
}

sub draw_up {
    my ($xc, $yc, $size) = @_;
    my $x0 = $xc - $size;
    my $x1 = $xc + $size;
    my $y0 = $yc - $size;
    my $y1 = $yc + 0.8*$size;
    my $foo = sprintf "polygon %.1f,%.1f %.1f,%.1f %.1f,%.1f",
        $xc, $y0, $x0, $y1, $x1, $y1;
    return $foo;
}

sub draw_down {
    my ($xc, $yc, $size) = @_;
    my $x0 = $xc - $size;
    my $x1 = $xc + $size;
    my $y0 = $yc - 0.8*$size;
    my $y1 = $yc + $size;
    my $foo = sprintf "polygon %.1f,%.1f %.1f,%.1f %.1f,%.1f",
        $xc, $y1, $x0, $y0, $x1, $y0;
    return $foo;
}

sub draw_hexagon {
    my ($xc, $yc, $size) = @_;
    my $dy = 0.5*$size;
    my $y0 = $yc - $size;
    my $y1 = $yc - $dy;
    my $y2 = $yc + $dy;
    my $y3 = $yc + $size;
    my $dx = 0.866 * $size;
    my $x0 = $xc - $dx;
    my $x1 = $xc + $dx;
    my $foo = sprintf "polygon %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f",
        $xc, $y0, $x1, $y1, $x1, $y2,
        $xc, $y3, $x0, $y2, $x0, $y1;
    return $foo;
}

sub draw_penta_up {
    my ($xc, $yc, $size) = @_;
    my $y0 = $yc - 1.0515 * $size;
    my $y1 = $yc - 0.3249 * $size;
    my $y2 = $yc + 0.8507 * $size;
    my $x0 = $xc - $size;
    my $x1 = $xc - 0.6180 * $size;
    my $x2 = $xc + 0.6180 * $size;
    my $x3 = $xc + $size;
    my $foo = sprintf "polygon %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f",
        $xc, $y0, $x3, $y1, $x2, $y2, $x1, $y2, $x0, $y1;
    return $foo;
}

sub draw_penta_down {
    my ($xc, $yc, $size) = @_;
    my $y0 = $yc + 1.0515 * $size;
    my $y1 = $yc + 0.3249 * $size;
    my $y2 = $yc - 0.8507 * $size;
    my $x0 = $xc - $size;
    my $x1 = $xc - 0.6180 * $size;
    my $x2 = $xc + 0.6180 * $size;
    my $x3 = $xc + $size;
    my $foo = sprintf "polygon %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f %.1f,%.1f",
        $xc, $y0, $x3, $y1, $x2, $y2, $x1, $y2, $x0, $y1;
    return $foo;
}

sub get_size {
    my $asu = shift @_;
    #my $size = (1.0 + (8.0 / 32.0) * ($asu));
    my $size = sqrt(1.0 + 2.5*$asu);
    return $size;
}

sub draw_gate {
    # rotated gate sign, signifying lack of coverage
    my ($xc, $yc) = @_;
    my @result = ();
    push @result, (sprintf "line %d,%d %d,%d", $xc-8, $yc-4, $xc,   $yc-8);
    push @result, (sprintf "line %d,%d %d,%d", $xc-8, $yc+4, $xc+8, $yc-4);
    push @result, (sprintf "line %d,%d %d,%d", $xc,   $yc+8, $xc+8, $yc+4);
    push @result, (sprintf "line %d,%d %d,%d", $xc-8, $yc,   $xc-4, $yc+8);
    push @result, (sprintf "line %d,%d %d,%d", $xc-4, $yc-8, $xc+4, $yc+8);
    push @result, (sprintf "line %d,%d %d,%d", $xc+4, $yc-8, $xc+8, $yc);
    return @result;
}

sub draw_semi_gate {
    # rotated gate sign, signifying lack of coverage
    my ($xc, $yc) = @_;
    my @result = ();
    push @result, (sprintf "line %d,%d %d,%d", $xc-8, $yc+4, $xc+8, $yc-4);
    push @result, (sprintf "line %d,%d %d,%d", $xc-4, $yc-8, $xc+4, $yc+8);
    return @result;
}

sub draw_semi_gate_flipped {
    # rotated gate sign, signifying lack of coverage
    my ($xc, $yc) = @_;
    my @result = ();
    push @result, (sprintf "line %d,%d %d,%d", $xc+8, $yc+4, $xc-8, $yc-4);
    push @result, (sprintf "line %d,%d %d,%d", $xc+4, $yc-8, $xc-4, $yc+8);
    return @result;
}

sub draw_semi_gate_45 {
    # rotated gate sign, signifying lack of coverage
    my ($xc, $yc) = @_;
    my @result = ();
    push @result, (sprintf "line %d,%d %d,%d", $xc+8, $yc+8, $xc-8, $yc-8);
    push @result, (sprintf "line %d,%d %d,%d", $xc+8, $yc-8, $xc-8, $yc+8);
    return @result;
}

