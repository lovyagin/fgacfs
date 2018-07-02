/*
  This code originates from CityHash code https://github.com/google/cityhash
  distributed under MIT License

  Copyright (c) 2011 Google, Inc.

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
  THE SOFTWARE.

  CityHash, by Geoff Pike and Jyrki Alakuijala

  FGACFS modification and adoption

  Copyright (C) 2017 Nikita Yu. Lovyagin, <lovyagin@mail.com>

  This program can be distributed under the terms of the GNU GPLv3.
  See the file COPYING.
 */

#include <config.h>
#include <string.h>
#include <stdint.h>
#include "bswap.h"

typedef struct
{
    uint64_t first, second;
} uint128_t;

static const uint64_t k0 = 0xc3a5c85c97cb3127ULL;
static const uint64_t k1 = 0xb492b66fbe98f273ULL;
static const uint64_t k2 = 0x9ae16a3b2f90404fULL;

static void swap64 (uint64_t *x, uint64_t *y)
{
    uint64_t z = *x;
    *x = *y;
    *y = z;
}

#define Uint128Low64(x)  (x.first)
#define Uint128High64(x) (x.second)

static uint64_t Hash128to64(uint128_t x)
{
  const uint64_t kMul = 0x9ddfea08eb382d69ULL;
  uint64_t a = (Uint128Low64(x) ^ Uint128High64(x)) * kMul;
  a ^= (a >> 47);
  uint64_t b = (Uint128High64(x) ^ a) * kMul;
  b ^= (b >> 47);
  b *= kMul;
  return b;
}


static uint64_t HashLen16_2(uint64_t u, uint64_t v)
{
    uint128_t i; i.first = u; i.second = v;
    return Hash128to64(i);
}

static uint64_t HashLen16(uint64_t u, uint64_t v, uint64_t mul)
{
    uint64_t a = (u ^ v) * mul;
    a ^= (a >> 47);
    uint64_t b = (v ^ a) * mul;
    b ^= (b >> 47);
    b *= mul;
    return b;
}

#ifdef WORDS_BIGENDIAN
#define uint32_in_expected_order(x) (bswap_32(x))
#define uint64_in_expected_order(x) (bswap_64(x))
#else
#define uint32_in_expected_order(x) (x)
#define uint64_in_expected_order(x) (x)
#endif

static uint32_t UNALIGNED_LOAD32(const char *p)
{
    uint32_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}
static uint64_t UNALIGNED_LOAD64(const char *p)
{
    uint64_t result;
    memcpy(&result, p, sizeof(result));
    return result;
}

static uint32_t Fetch32(const char *p)
{
    return uint32_in_expected_order(UNALIGNED_LOAD32(p));
}
static uint64_t Fetch64(const char *p)
{
    return uint64_in_expected_order(UNALIGNED_LOAD64(p));
}

static uint64_t Rotate(uint64_t val, int shift)
{
    return shift == 0 ? val : ((val >> shift) | (val << (64 - shift)));
}

static uint64_t ShiftMix(uint64_t val)
{
    return val ^ (val >> 47);
}

static uint64_t HashLen0to16(const char *s, size_t len)
{
    if (len >= 8)
    {
        uint64_t mul = k2 + len * 2;
        uint64_t a = Fetch64(s) + k2;
        uint64_t b = Fetch64(s + len - 8);
        uint64_t c = Rotate(b, 37) * mul + a;
        uint64_t d = (Rotate(a, 25) + b) * mul;
        return HashLen16(c, d, mul);
    }

    if (len >= 4)
    {
        uint64_t mul = k2 + len * 2;
        uint64_t a = Fetch32(s);
        return HashLen16(len + (a << 3), Fetch32(s + len - 4), mul);
    }

    if (len > 0)
    {
        uint8_t a = s[0];
        uint8_t b = s[len >> 1];
        uint8_t c = s[len - 1];
        uint32_t y = (uint32_t) a + (((uint32_t) b) << 8);
        uint32_t z = len + (((uint32_t) c) << 2);
        return ShiftMix(y * k2 ^ z * k0) * k2;
    }
    return k2;
}

static uint64_t HashLen17to32(const char *s, size_t len)
{
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch64(s) * k1;
    uint64_t b = Fetch64(s + 8);
    uint64_t c = Fetch64(s + len - 8) * mul;
    uint64_t d = Fetch64(s + len - 16) * k2;
    return HashLen16(Rotate(a + b, 43) + Rotate(c, 30) + d,
                     a + Rotate(b + k2, 18) + c, mul
                    );
}

static uint64_t HashLen33to64(const char *s, size_t len)
{
    uint64_t mul = k2 + len * 2;
    uint64_t a = Fetch64(s) * k2;
    uint64_t b = Fetch64(s + 8);
    uint64_t c = Fetch64(s + len - 24);
    uint64_t d = Fetch64(s + len - 32);
    uint64_t e = Fetch64(s + 16) * k2;
    uint64_t f = Fetch64(s + 24) * 9;
    uint64_t g = Fetch64(s + len - 8);
    uint64_t h = Fetch64(s + len - 16) * mul;
    uint64_t u = Rotate(a + g, 43) + (Rotate(b, 30) + c) * 9;
    uint64_t v = ((a + g) ^ d) + f + 1;
    uint64_t w = bswap_64((u + v) * mul) + h;
    uint64_t x = Rotate(e + f, 42) + c;
    uint64_t y = (bswap_64((v + w) * mul) + g) * mul;
    uint64_t z = e + f + c;
    a = bswap_64((x + z) * mul + y) + b;
    b = ShiftMix((z + a) * mul + d + h) * mul;
    return b + x;
}

static uint128_t WeakHashLen32WithSeeds_base (uint64_t w, uint64_t x, uint64_t y, uint64_t z, uint64_t a, uint64_t b)
{
    uint64_t c;
    uint128_t r;
    a += w;
    b = Rotate(b + a + z, 21);
    c = a;
    a += x;
    a += y;
    b += Rotate(a, 44);
    r.first = a + z;
    r.second = b + c;
    return r;
}

static uint128_t WeakHashLen32WithSeeds(const char* s, uint64_t a, uint64_t b)
{
    return WeakHashLen32WithSeeds_base(Fetch64(s),
                                       Fetch64(s + 8),
                                       Fetch64(s + 16),
                                       Fetch64(s + 24),
                                       a,
                                       b
                                      );
}


uint64_t CityHash64(const char *s)
{

    size_t len = strlen(s);
    uint64_t x, y, z;
    uint128_t v, w;


    if      (len <= 16) return HashLen0to16(s, len);
    else if (len <= 32) return HashLen17to32(s, len);
    else if (len <= 64) return HashLen33to64(s, len);


    x = Fetch64(s + len - 40);
    y = Fetch64(s + len - 16) + Fetch64(s + len - 56);
    z = HashLen16_2(Fetch64(s + len - 48) + len, Fetch64(s + len - 24));
    v = WeakHashLen32WithSeeds(s + len - 64, len, z);
    w = WeakHashLen32WithSeeds(s + len - 32, y + k1, x);
    x = x * k1 + Fetch64(s);

    len = (len - 1) & ~ (size_t) (63);
    do
    {
        x = Rotate(x + y + v.first + Fetch64(s + 8), 37) * k1;
        y = Rotate(y + v.second + Fetch64(s + 48), 42) * k1;
        x ^= w.second;
        y += v.first + Fetch64(s + 40);
        z = Rotate(z + w.first, 33) * k1;
        v = WeakHashLen32WithSeeds(s, v.second * k1, x + w.first);
        w = WeakHashLen32WithSeeds(s + 32, z + w.second, y + Fetch64(s + 16));
        swap64 (&z, &x);
        s += 64;
        len -= 64;
    } while (len);

    return HashLen16_2(HashLen16_2(v.first, w.first) + ShiftMix(y) * k1 + z,
                       HashLen16_2(v.second, w.second) + x
                      );
}


