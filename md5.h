#ifndef MD5_H
#define MD5_H

/* MD5 (RFC 1321) for 16-bit compilers: int is 16 bits, long is 32 bits. */

typedef unsigned long md5_u32;

typedef struct {
    md5_u32 state[4];
    md5_u32 count[2];           /* bit count, low word first */
    unsigned char buffer[64];
} MD5_CTX;

void md5_init(MD5_CTX *ctx);
void md5_update(MD5_CTX *ctx, const unsigned char *data, unsigned len);
void md5_final(unsigned char digest[16], MD5_CTX *ctx);

#endif
