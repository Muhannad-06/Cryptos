#ifndef CRYPTO_UTILS_H
#define CRYPTO_UTILS_H

#include <stddef.h>
#include "crypto_errors.h"

/*
 * Generate cryptographically secure random bytes.
 * buf must point to a buffer of at least len bytes.
 * Returns CRYPTO_OK on success or a CryptoError on failure.
 */
CryptoError crypto_random_bytes(unsigned char *buf, size_t len);

/*
 * Compare two byte buffers in constant time.
 * a and b must point to buffers of at least len bytes.
 * Returns 0 if equal, non-zero if different.
 */
int crypto_constant_time_compare(
    const unsigned char *a,
    const unsigned char *b,
    size_t len
);

#endif