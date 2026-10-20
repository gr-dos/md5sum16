#include <string.h>
#include "md5.h"

#define F(x, y, z) (((x) & (y)) | (~(x) & (z)))
#define G(x, y, z) (((x) & (z)) | ((y) & ~(z)))
#define H(x, y, z) ((x) ^ (y) ^ (z))
#define I(x, y, z) ((y) ^ ((x) | ~(z)))

#define ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define STEP(f, a, b, c, d, x, s, t) \
    (a) += f((b), (c), (d)) + (x) + (md5_u32)(t); \
    (a) = ROTL((a), (s)) + (b);

static void md5_transform(md5_u32 state[4], const unsigned char block[64])
{
    md5_u32 a = state[0], b = state[1], c = state[2], d = state[3];
    md5_u32 x[16];
    int i;

    for (i = 0; i < 16; i++) {
        const unsigned char *p = block + i * 4;
        x[i] = (md5_u32)p[0]
             | ((md5_u32)p[1] << 8)
             | ((md5_u32)p[2] << 16)
             | ((md5_u32)p[3] << 24);
    }

    STEP(F, a, b, c, d, x[ 0],  7, 0xd76aa478UL)
    STEP(F, d, a, b, c, x[ 1], 12, 0xe8c7b756UL)
    STEP(F, c, d, a, b, x[ 2], 17, 0x242070dbUL)
    STEP(F, b, c, d, a, x[ 3], 22, 0xc1bdceeeUL)
    STEP(F, a, b, c, d, x[ 4],  7, 0xf57c0fafUL)
    STEP(F, d, a, b, c, x[ 5], 12, 0x4787c62aUL)
    STEP(F, c, d, a, b, x[ 6], 17, 0xa8304613UL)
    STEP(F, b, c, d, a, x[ 7], 22, 0xfd469501UL)
    STEP(F, a, b, c, d, x[ 8],  7, 0x698098d8UL)
    STEP(F, d, a, b, c, x[ 9], 12, 0x8b44f7afUL)
    STEP(F, c, d, a, b, x[10], 17, 0xffff5bb1UL)
    STEP(F, b, c, d, a, x[11], 22, 0x895cd7beUL)
    STEP(F, a, b, c, d, x[12],  7, 0x6b901122UL)
    STEP(F, d, a, b, c, x[13], 12, 0xfd987193UL)
    STEP(F, c, d, a, b, x[14], 17, 0xa679438eUL)
    STEP(F, b, c, d, a, x[15], 22, 0x49b40821UL)

    STEP(G, a, b, c, d, x[ 1],  5, 0xf61e2562UL)
    STEP(G, d, a, b, c, x[ 6],  9, 0xc040b340UL)
    STEP(G, c, d, a, b, x[11], 14, 0x265e5a51UL)
    STEP(G, b, c, d, a, x[ 0], 20, 0xe9b6c7aaUL)
    STEP(G, a, b, c, d, x[ 5],  5, 0xd62f105dUL)
    STEP(G, d, a, b, c, x[10],  9, 0x02441453UL)
    STEP(G, c, d, a, b, x[15], 14, 0xd8a1e681UL)
    STEP(G, b, c, d, a, x[ 4], 20, 0xe7d3fbc8UL)
    STEP(G, a, b, c, d, x[ 9],  5, 0x21e1cde6UL)
    STEP(G, d, a, b, c, x[14],  9, 0xc33707d6UL)
    STEP(G, c, d, a, b, x[ 3], 14, 0xf4d50d87UL)
    STEP(G, b, c, d, a, x[ 8], 20, 0x455a14edUL)
    STEP(G, a, b, c, d, x[13],  5, 0xa9e3e905UL)
    STEP(G, d, a, b, c, x[ 2],  9, 0xfcefa3f8UL)
    STEP(G, c, d, a, b, x[ 7], 14, 0x676f02d9UL)
    STEP(G, b, c, d, a, x[12], 20, 0x8d2a4c8aUL)

    STEP(H, a, b, c, d, x[ 5],  4, 0xfffa3942UL)
    STEP(H, d, a, b, c, x[ 8], 11, 0x8771f681UL)
    STEP(H, c, d, a, b, x[11], 16, 0x6d9d6122UL)
    STEP(H, b, c, d, a, x[14], 23, 0xfde5380cUL)
    STEP(H, a, b, c, d, x[ 1],  4, 0xa4beea44UL)
    STEP(H, d, a, b, c, x[ 4], 11, 0x4bdecfa9UL)
    STEP(H, c, d, a, b, x[ 7], 16, 0xf6bb4b60UL)
    STEP(H, b, c, d, a, x[10], 23, 0xbebfbc70UL)
    STEP(H, a, b, c, d, x[13],  4, 0x289b7ec6UL)
    STEP(H, d, a, b, c, x[ 0], 11, 0xeaa127faUL)
    STEP(H, c, d, a, b, x[ 3], 16, 0xd4ef3085UL)
    STEP(H, b, c, d, a, x[ 6], 23, 0x04881d05UL)
    STEP(H, a, b, c, d, x[ 9],  4, 0xd9d4d039UL)
    STEP(H, d, a, b, c, x[12], 11, 0xe6db99e5UL)
    STEP(H, c, d, a, b, x[15], 16, 0x1fa27cf8UL)
    STEP(H, b, c, d, a, x[ 2], 23, 0xc4ac5665UL)

    STEP(I, a, b, c, d, x[ 0],  6, 0xf4292244UL)
    STEP(I, d, a, b, c, x[ 7], 10, 0x432aff97UL)
    STEP(I, c, d, a, b, x[14], 15, 0xab9423a7UL)
    STEP(I, b, c, d, a, x[ 5], 21, 0xfc93a039UL)
    STEP(I, a, b, c, d, x[12],  6, 0x655b59c3UL)
    STEP(I, d, a, b, c, x[ 3], 10, 0x8f0ccc92UL)
    STEP(I, c, d, a, b, x[10], 15, 0xffeff47dUL)
    STEP(I, b, c, d, a, x[ 1], 21, 0x85845dd1UL)
    STEP(I, a, b, c, d, x[ 8],  6, 0x6fa87e4fUL)
    STEP(I, d, a, b, c, x[15], 10, 0xfe2ce6e0UL)
    STEP(I, c, d, a, b, x[ 6], 15, 0xa3014314UL)
    STEP(I, b, c, d, a, x[13], 21, 0x4e0811a1UL)
    STEP(I, a, b, c, d, x[ 4],  6, 0xf7537e82UL)
    STEP(I, d, a, b, c, x[11], 10, 0xbd3af235UL)
    STEP(I, c, d, a, b, x[ 2], 15, 0x2ad7d2bbUL)
    STEP(I, b, c, d, a, x[ 9], 21, 0xeb86d391UL)

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void md5_init(MD5_CTX *ctx)
{
    ctx->count[0] = ctx->count[1] = 0;
    ctx->state[0] = 0x67452301UL;
    ctx->state[1] = 0xefcdab89UL;
    ctx->state[2] = 0x98badcfeUL;
    ctx->state[3] = 0x10325476UL;
}

void md5_update(MD5_CTX *ctx, const unsigned char *data, unsigned len)
{
    unsigned idx = (unsigned)((ctx->count[0] >> 3) & 0x3F);
    unsigned fill = 64 - idx;
    md5_u32 bits = (md5_u32)len << 3;

    ctx->count[0] += bits;
    if (ctx->count[0] < bits)
        ctx->count[1]++;
    ctx->count[1] += (md5_u32)len >> 29;

    if (idx && len >= fill) {
        memcpy(ctx->buffer + idx, data, fill);
        md5_transform(ctx->state, ctx->buffer);
        data += fill;
        len -= fill;
        idx = 0;
    }
    while (len >= 64) {
        md5_transform(ctx->state, data);
        data += 64;
        len -= 64;
    }
    if (len)
        memcpy(ctx->buffer + idx, data, len);
}

void md5_final(unsigned char digest[16], MD5_CTX *ctx)
{
    static const unsigned char padding[64] = { 0x80 };
    unsigned char bits[8];
    unsigned idx, padlen;
    int i;

    for (i = 0; i < 8; i++)
        bits[i] = (unsigned char)(ctx->count[i >> 2] >> ((i & 3) * 8));

    idx = (unsigned)((ctx->count[0] >> 3) & 0x3F);
    padlen = (idx < 56) ? (56 - idx) : (120 - idx);
    md5_update(ctx, padding, padlen);
    md5_update(ctx, bits, 8);

    for (i = 0; i < 16; i++)
        digest[i] = (unsigned char)(ctx->state[i >> 2] >> ((i & 3) * 8));

    memset(ctx, 0, sizeof(*ctx));
}
