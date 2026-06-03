#include "ed25519.h"

#include "sha512.h"

/*
 * Field element: 16 signed 64-bit limbs, radix 2^16 (TweetNaCl `gf`).
 * The algorithm below mirrors the public-domain TweetNaCl reference exactly;
 * only types and names were adapted. RFC 8032 known-answer tests gate it.
 */
typedef int64_t gf[16];

static const gf gf0 = {0};
static const gf gf1 = {1};
static const gf D = {0x78a3, 0x1359, 0x4dca, 0x75eb, 0xd8ab, 0x4141,
                     0x0a4d, 0x0070, 0xe898, 0x7779, 0x4079, 0x8cc7,
                     0xfe73, 0x2b6f, 0x6cee, 0x5203};
static const gf D2 = {0xf159, 0x26b2, 0x9b94, 0xebd6, 0xb156, 0x8283,
                      0x149a, 0x00e0, 0xd130, 0xeef3, 0x80f2, 0x198e,
                      0xfce7, 0x56df, 0xd9dc, 0x2406};
static const gf X = {0xd51a, 0x8f25, 0x2d60, 0xc956, 0xa7b2, 0x9525,
                     0xc760, 0x692c, 0xdc5c, 0xfdd6, 0xe231, 0xc0a4,
                     0x53fe, 0xcd6e, 0x36d3, 0x2169};
static const gf Y = {0x6658, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
                     0x6666, 0x6666, 0x6666, 0x6666, 0x6666, 0x6666,
                     0x6666, 0x6666, 0x6666, 0x6666};
static const gf I = {0xa0b0, 0x4a0e, 0x1b27, 0xc4ee, 0xe478, 0xad2f,
                     0x1806, 0x2f43, 0xd7a7, 0x3dfb, 0x0099, 0x2b4d,
                     0xdf0b, 0x4fc1, 0x2480, 0x2b83};

static const uint64_t L[32] = {0xed, 0xd3, 0xf5, 0x5c, 0x1a, 0x63, 0x12, 0x58,
                               0xd6, 0x9c, 0xf7, 0xa2, 0xde, 0xf9, 0xde, 0x14,
                               0,    0,    0,    0,    0,    0,    0,    0,
                               0,    0,    0,    0,    0,    0,    0,    0x10};

static void wipe(void *ptr, size_t len) {
  volatile uint8_t *p = (volatile uint8_t *)ptr;
  while (len--) {
    *p++ = 0;
  }
}

static int vn(const uint8_t *x, const uint8_t *y, int n) {
  uint32_t d = 0u;
  int i;
  for (i = 0; i < n; ++i) {
    d |= (uint32_t)(x[i] ^ y[i]);
  }
  return (int)((1u & ((d - 1u) >> 8)) - 1u);
}

static int crypto_verify_32(const uint8_t *x, const uint8_t *y) {
  return vn(x, y, 32);
}

static void set25519(gf r, const gf a) {
  int i;
  for (i = 0; i < 16; ++i) {
    r[i] = a[i];
  }
}

static void car25519(gf o) {
  int i;
  int64_t c;
  for (i = 0; i < 16; ++i) {
    o[i] += ((int64_t)1 << 16);
    c = o[i] >> 16;
    /* Faithful to TweetNaCl: limbs 0..14 carry into the next limb (c-1);
     * limb 15 wraps into limb 0 with the 2^255-19 reduction (38*(c-1)).
     * Written with an explicit branch so the index is provably in [0,15]. */
    if (i < 15) {
      o[i + 1] += c - 1;
    } else {
      o[0] += 38 * (c - 1);
    }
    o[i] -= c << 16;
  }
}

static void sel25519(gf p, gf q, int b) {
  int64_t t;
  int64_t c = ~((int64_t)b - 1);
  int i;
  for (i = 0; i < 16; ++i) {
    t = c & (p[i] ^ q[i]);
    p[i] ^= t;
    q[i] ^= t;
  }
}

static void pack25519(uint8_t *o, const gf n) {
  int i, j, b;
  gf m, t;
  for (i = 0; i < 16; ++i) {
    t[i] = n[i];
  }
  car25519(t);
  car25519(t);
  car25519(t);
  for (j = 0; j < 2; ++j) {
    m[0] = t[0] - 0xffed;
    for (i = 1; i < 15; ++i) {
      m[i] = t[i] - 0xffff - ((m[i - 1] >> 16) & 1);
      m[i - 1] &= 0xffff;
    }
    m[15] = t[15] - 0x7fff - ((m[14] >> 16) & 1);
    b = (int)((m[15] >> 16) & 1);
    m[14] &= 0xffff;
    sel25519(t, m, 1 - b);
  }
  for (i = 0; i < 16; ++i) {
    o[2 * i] = (uint8_t)(t[i] & 0xff);
    o[2 * i + 1] = (uint8_t)(t[i] >> 8);
  }
}

static int neq25519(const gf a, const gf b) {
  uint8_t c[32], d[32];
  pack25519(c, a);
  pack25519(d, b);
  return crypto_verify_32(c, d);
}

static uint8_t par25519(const gf a) {
  uint8_t d[32];
  pack25519(d, a);
  return (uint8_t)(d[0] & 1);
}

static void unpack25519(gf o, const uint8_t *n) {
  int i;
  for (i = 0; i < 16; ++i) {
    o[i] = n[2 * i] + ((int64_t)n[2 * i + 1] << 8);
  }
  o[15] &= 0x7fff;
}

static void add_gf(gf o, const gf a, const gf b) {
  int i;
  for (i = 0; i < 16; ++i) {
    o[i] = a[i] + b[i];
  }
}

static void sub_gf(gf o, const gf a, const gf b) {
  int i;
  for (i = 0; i < 16; ++i) {
    o[i] = a[i] - b[i];
  }
}

static void mul_gf(gf o, const gf a, const gf b) {
  int64_t t[31];
  int i, j;
  for (i = 0; i < 31; ++i) {
    t[i] = 0;
  }
  for (i = 0; i < 16; ++i) {
    for (j = 0; j < 16; ++j) {
      t[i + j] += a[i] * b[j];
    }
  }
  for (i = 0; i < 15; ++i) {
    t[i] += 38 * t[i + 16];
  }
  for (i = 0; i < 16; ++i) {
    o[i] = t[i];
  }
  car25519(o);
  car25519(o);
}

static void sq_gf(gf o, const gf a) { mul_gf(o, a, a); }

static void inv25519(gf o, const gf i) {
  gf c;
  int a;
  for (a = 0; a < 16; ++a) {
    c[a] = i[a];
  }
  for (a = 253; a >= 0; --a) {
    sq_gf(c, c);
    if (a != 2 && a != 4) {
      mul_gf(c, c, i);
    }
  }
  for (a = 0; a < 16; ++a) {
    o[a] = c[a];
  }
}

static void pow2523(gf o, const gf i) {
  gf c;
  int a;
  for (a = 0; a < 16; ++a) {
    c[a] = i[a];
  }
  for (a = 250; a >= 0; --a) {
    sq_gf(c, c);
    if (a != 1) {
      mul_gf(c, c, i);
    }
  }
  for (a = 0; a < 16; ++a) {
    o[a] = c[a];
  }
}

static void add_point(gf p[4], gf q[4]) {
  gf a, b, c, d, t, e, f, g, h;
  sub_gf(a, p[1], p[0]);
  sub_gf(t, q[1], q[0]);
  mul_gf(a, a, t);
  add_gf(b, p[0], p[1]);
  add_gf(t, q[0], q[1]);
  mul_gf(b, b, t);
  mul_gf(c, p[3], q[3]);
  mul_gf(c, c, D2);
  mul_gf(d, p[2], q[2]);
  add_gf(d, d, d);
  sub_gf(e, b, a);
  sub_gf(f, d, c);
  add_gf(g, d, c);
  add_gf(h, b, a);
  mul_gf(p[0], e, f);
  mul_gf(p[1], h, g);
  mul_gf(p[2], g, f);
  mul_gf(p[3], e, h);
}

static void cswap(gf p[4], gf q[4], uint8_t b) {
  int i;
  for (i = 0; i < 4; ++i) {
    sel25519(p[i], q[i], (int)b);
  }
}

static void pack(uint8_t *r, gf p[4]) {
  gf tx, ty, zi;
  inv25519(zi, p[2]);
  mul_gf(tx, p[0], zi);
  mul_gf(ty, p[1], zi);
  pack25519(r, ty);
  r[31] ^= (uint8_t)(par25519(tx) << 7);
}

static void scalarmult(gf p[4], gf q[4], const uint8_t *s) {
  int i;
  set25519(p[0], gf0);
  set25519(p[1], gf1);
  set25519(p[2], gf1);
  set25519(p[3], gf0);
  for (i = 255; i >= 0; --i) {
    uint8_t b = (uint8_t)((s[i / 8] >> (i & 7)) & 1);
    cswap(p, q, b);
    add_point(q, p);
    add_point(p, p);
    cswap(p, q, b);
  }
}

static void scalarbase(gf p[4], const uint8_t *s) {
  gf q[4];
  set25519(q[0], X);
  set25519(q[1], Y);
  set25519(q[2], gf1);
  mul_gf(q[3], X, Y);
  scalarmult(p, q, s);
}

static void modL(uint8_t *r, int64_t x[64]) {
  int64_t carry;
  int i, j;
  for (i = 63; i >= 32; --i) {
    carry = 0;
    for (j = i - 32; j < i - 12; ++j) {
      x[j] += carry - 16 * x[i] * (int64_t)L[j - (i - 32)];
      carry = (x[j] + 128) >> 8;
      x[j] -= carry << 8;
    }
    x[j] += carry;
    x[i] = 0;
  }
  carry = 0;
  for (j = 0; j < 32; ++j) {
    x[j] += carry - (x[31] >> 4) * (int64_t)L[j];
    carry = x[j] >> 8;
    x[j] &= 255;
  }
  for (j = 0; j < 32; ++j) {
    x[j] -= carry * (int64_t)L[j];
  }
  for (i = 0; i < 32; ++i) {
    x[i + 1] += x[i] >> 8;
    r[i] = (uint8_t)(x[i] & 255);
  }
}

static void reduce(uint8_t *r) {
  int64_t x[64];
  int i;
  for (i = 0; i < 64; ++i) {
    x[i] = (int64_t)(uint64_t)r[i];
  }
  for (i = 0; i < 64; ++i) {
    r[i] = 0;
  }
  modL(r, x);
}

static int unpackneg(gf r[4], const uint8_t p[32]) {
  gf t, chk, num, den, den2, den4, den6;
  set25519(r[2], gf1);
  unpack25519(r[1], p);
  sq_gf(num, r[1]);
  mul_gf(den, num, D);
  sub_gf(num, num, r[2]);
  add_gf(den, r[2], den);
  sq_gf(den2, den);
  sq_gf(den4, den2);
  mul_gf(den6, den4, den2);
  mul_gf(t, den6, num);
  mul_gf(t, t, den);
  pow2523(t, t);
  mul_gf(t, t, num);
  mul_gf(t, t, den);
  mul_gf(t, t, den);
  mul_gf(r[0], t, den);
  sq_gf(chk, r[0]);
  mul_gf(chk, chk, den);
  if (neq25519(chk, num)) {
    mul_gf(r[0], r[0], I);
  }
  sq_gf(chk, r[0]);
  mul_gf(chk, chk, den);
  if (neq25519(chk, num)) {
    return -1;
  }
  if (par25519(r[0]) == (p[31] >> 7)) {
    sub_gf(r[0], gf0, r[0]);
  }
  mul_gf(r[3], r[0], r[1]);
  return 0;
}

void capy_ed25519_keypair_from_seed(const uint8_t seed[CAPY_ED25519_SEED_LEN],
                                    uint8_t public_key[CAPY_ED25519_PUBLIC_KEY_LEN],
                                    uint8_t secret_key[CAPY_ED25519_SECRET_KEY_LEN]) {
  uint8_t d[64];
  gf p[4];
  int i;
  for (i = 0; i < 32; ++i) {
    secret_key[i] = seed[i];
  }
  capy_sha512(secret_key, 32u, d);
  d[0] &= 248;
  d[31] &= 127;
  d[31] |= 64;
  scalarbase(p, d);
  pack(public_key, p);
  for (i = 0; i < 32; ++i) {
    secret_key[32 + i] = public_key[i];
  }
  wipe(d, sizeof(d));
}

void capy_ed25519_sign(uint8_t sig[CAPY_ED25519_SIGNATURE_LEN], const uint8_t *m,
                       size_t n, const uint8_t secret_key[CAPY_ED25519_SECRET_KEY_LEN]) {
  uint8_t d[64];
  uint8_t r[64];
  uint8_t h[64];
  int64_t x[64];
  gf p[4];
  struct capy_sha512_ctx ctx;
  size_t i;
  size_t j;

  capy_sha512(secret_key, 32u, d);
  d[0] &= 248;
  d[31] &= 127;
  d[31] |= 64;

  /* r = SHA-512(prefix || M), prefix = d[32..63] */
  capy_sha512_init(&ctx);
  capy_sha512_update(&ctx, d + 32, 32u);
  capy_sha512_update(&ctx, m, n);
  capy_sha512_final(&ctx, r);
  reduce(r);

  scalarbase(p, r);
  pack(sig, p); /* sig[0..31] = R */

  /* h = SHA-512(R || A || M), A = secret_key[32..63] */
  capy_sha512_init(&ctx);
  capy_sha512_update(&ctx, sig, 32u);
  capy_sha512_update(&ctx, secret_key + 32, 32u);
  capy_sha512_update(&ctx, m, n);
  capy_sha512_final(&ctx, h);
  reduce(h);

  for (i = 0; i < 64u; ++i) {
    x[i] = 0;
  }
  for (i = 0; i < 32u; ++i) {
    x[i] = (int64_t)(uint64_t)r[i];
  }
  for (i = 0; i < 32u; ++i) {
    for (j = 0; j < 32u; ++j) {
      x[i + j] += (int64_t)h[i] * (int64_t)d[j];
    }
  }
  modL(sig + 32, x); /* sig[32..63] = S */

  wipe(d, sizeof(d));
  wipe(r, sizeof(r));
  wipe(h, sizeof(h));
  wipe(x, sizeof(x));
}

int capy_ed25519_verify(const uint8_t sig[CAPY_ED25519_SIGNATURE_LEN],
                        const uint8_t *m, size_t n,
                        const uint8_t public_key[CAPY_ED25519_PUBLIC_KEY_LEN]) {
  uint8_t t[32];
  uint8_t h[64];
  gf p[4];
  gf q[4];
  struct capy_sha512_ctx ctx;

  if (unpackneg(q, public_key)) {
    return -1;
  }
  capy_sha512_init(&ctx);
  capy_sha512_update(&ctx, sig, 32u);
  capy_sha512_update(&ctx, public_key, 32u);
  capy_sha512_update(&ctx, m, n);
  capy_sha512_final(&ctx, h);
  reduce(h);

  scalarmult(p, q, h);   /* p = h * (-A) */
  scalarbase(q, sig + 32); /* q = S * B */
  add_point(p, q);       /* p = S*B - h*A */
  pack(t, p);

  if (crypto_verify_32(sig, t) != 0) {
    return -1;
  }
  return 0;
}
