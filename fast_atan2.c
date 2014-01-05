/************************************************************************
 * Copyright (c) 2012, 2013 Richard P. Curnow
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in
 *       the documentation and/or other materials provided with the
 *       distribution.
 *     * Neither the name of the <organization> nor the
 *       names of its contributors may be used to endorse or promote
 *       products derived from this software without specific prior written
 *       permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS
 * IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED
 * TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A
 * PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER>
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 ************************************************************************/

#include <stdint.h>
#include <math.h>

#define K1 (0.97179803008f)
#define K2 (0.19065470515f)

float fast_atan2(float y, float x)
{
  float f;
  float offset;
  float pi = (float) M_PI;
  if (y > x) {
    if (y > -x) { /* north */
      f = -x/y;
      offset = 0.5f*pi;
    } else { /* west */
      f = y/x;
      if (y > 0.0f) {
        offset = pi;
      } else {
        offset = -pi;
      }
    }
  } else {
    if (y > -x) { /* east */
      f = y/x;
      offset = 0.0f;
    } else {
      if (y == 0.0f) {
        f = 0.0;
        offset = 0.0;
      } else {
        f = -x/y;
        offset = -0.5f*pi;
      }
    }
  }

  return offset + f * (K1 - K2*f*f);
}

#define K3 ((float)((double)(1<<29) * 4.0 * K1 / M_PI))
#define K4 ((float)((double)(1<<29) * 4.0 * K2 / M_PI))

int32_t atan2i_approx(int32_t y, int32_t x)
{
  int32_t u, v, s;
  uint32_t w;
  float f, f2, g;
  int32_t t, b;
  int32_t r;
  int32_t nx;

  nx = -x;
  u = x + y;
  v = x - y;
  w = (u ^ v) >> 31;
  t = (w) ? nx : y;
  b = (w) ? y : x;
  f = (float) t / (float) b;
  f2 = f * f;
  g = f * (K3 - K4*f2);
  s = (((u >> 30) & 3) ^ ((v >> 31) & 1)) << 30;
  r = s + (int32_t) g;
  return r;
}

int32_t atan2i_approx2(int32_t y, int32_t x)
{
  int32_t u, v, s;
  uint32_t w;
  float f, f2, g;
  int32_t r;
  float fx, fy;
  float tt, bb;

  fx = (float) x;
  fy = (float) y;
  u = x + y;
  v = x - y;
  w = (u ^ v) >> 31;
  tt = (w) ? -fx : fy;
  bb = (w) ? fy : fx;
  f = tt / bb;
  f2 = f * f;
  g = f * (K3 - K4*f2);
  s = (((u >> 30) & 3) ^ ((v >> 31) & 1)) << 30;
  r = s + (int32_t) g;
  return r;
}

#ifdef TEST

#include <stdio.h>

int main (int argc, char **argv)
{
  double deg;

  for (deg=-360.0; deg<=360.0; deg+=1.0) {
    double x, y;
    double theta;
    double t1, t2;
    theta = (M_PI/180.0) * deg;
    x = cos(theta);
    y = sin(theta);
    t1 = atan2(y, x);
    t2 = (double) fast_atan2((float)y, (float)x);

    printf("%.1f %.8f %.8f %.8f %.2f\n",
        deg,
        t1,
        t2,
        t1 - t2,
        (180.0/M_PI) * (t1 - t2));
  }
  return 0;

}
#endif

#ifdef TEST2

#include <stdio.h>

int main (int argc, char **argv)
{
  int ang;

  for (ang=-180.0; ang<=180.0; ang+=1) {
    double deg = (double) ang;
    double x, y;
    double theta;
    double theta2;
    double t1, t2;
    int32_t X, Y, R, R2;
    theta = (M_PI/180.0) * deg;
    x = cos(theta);
    y = sin(theta);
    X = (int32_t) (0.5 + 65536.0 * x);
    Y = (int32_t) (0.5 + 65536.0 * y);
    R = atan2i_approx(Y, X);
    R2 = atan2i_approx2(Y, X);
    theta = R * (180.0 / (double)(1U<<31));
    theta2 = R2 * (180.0 / (double)(1U<<31));

    printf("%4d %08x %.2f %08lx %.2f\n",
        ang,
        (unsigned) R, deg - theta,
        (unsigned) R2, deg - theta2);
  }
  return 0;
}

#endif

#define ITERS 100000000

#ifdef TIME1
#include <stdio.h>

int main (int argc, char **argv)
{
  double x, y;
  double total;
  int i;

  x = -10.0;
  y = 5.0;
  for (i=0; i<ITERS; i++) {
    total = 0.5 * (total + atan2(y, x));
    y += 0.001;
    x -= 0.001;
  }
  printf("total=%e\n", total);
  return 0;

}

#endif

#ifdef TIME2
#include <stdio.h>

int main (int argc, char **argv)
{
  float x, y;
  float total;
  int i;

  x = -10.0f;
  y = 5.0f;
  for (i=0; i<ITERS; i++) {
    total = 0.5f * (total + fast_atan2(y, x));
    y += 0.001f;
    x -= 0.001f;
  }
  printf("total=%e\n", (double) total);
  return 0;

}

#endif

#ifdef TIME3
#include <stdio.h>

int main (int argc, char **argv)
{
  float x, y;
  float total;
  int i;

  x = -10.0f;
  y = 5.0f;
  for (i=0; i<ITERS; i++) {
    total = 0.5f * (total + atan2f(y, x));
    y += 0.001f;
    x -= 0.001f;
  }
  printf("total=%e\n", (double) total);
  return 0;

}

#endif

#ifdef TIME4
#include <stdio.h>

static inline uint32_t genrand(int context)
{
  static uint32_t xx[4] = {0xdeadbeef, 0xfee1c001, 0xbeefca6e, 0xc001babe};
  uint32_t x = xx[context];
  x ^= x<<11;
  x ^= x>>21;
  x ^= x<<13;
  xx[context] = x;
  return x;
}

#define NITER 50000000

int main (int argc, char **argv)
{
  int iter;
  int32_t total;

  for (iter=0; iter<NITER; iter++) {
    int32_t y, x;
    int32_t r;
    y = genrand(0);
    x = genrand(0);
    r = atan2i_approx(y, x);
    total += r;
    r = atan2i_approx(x, y);
    total += r;
    r = atan2i_approx(-y, x);
    total += r;
    r = atan2i_approx(x, -y);
    total += r;
  }
  printf("%08x\n", total);
  return 0;
}

#endif

#ifdef TIME5
#include <stdio.h>

static inline uint32_t genrand(int context)
{
  static uint32_t xx[4] = {0xdeadbeef, 0xfee1c001, 0xbeefca6e, 0xc001babe};
  uint32_t x = xx[context];
  x ^= x<<11;
  x ^= x>>21;
  x ^= x<<13;
  xx[context] = x;
  return x;
}

#define NITER 50000000

int main (int argc, char **argv)
{
  int iter;
  int32_t total;

  for (iter=0; iter<NITER; iter++) {
    int32_t y, x;
    int32_t r;
    y = genrand(0);
    x = genrand(0);
    r = atan2i_approx2(y, x);
    total += r;
    r = atan2i_approx2(x, y);
    total += r;
    r = atan2i_approx2(-y, x);
    total += r;
    r = atan2i_approx2(x, -y);
    total += r;
  }
  printf("%08x\n", total);
  return 0;
}

#endif
