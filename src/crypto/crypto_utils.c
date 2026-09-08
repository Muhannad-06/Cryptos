#include "crypto_utils.h"
#include "crypto_errors.h"

#include <openssl/rand.h>
#include <openssl/crypto.h>

CryptoError crypto_random_bytes(unsigned char *buf, size_t len)
{
    if (buf == NULL || len == 0) {
        return CRYPTO_ERR_INVALID_PARAMETER;
    }

    int result = RAND_bytes(buf, (int)len);

    if (result != 1) {
        return CRYPTO_ERR_KEYGEN;
    }

    return CRYPTO_OK;
}

int crypto_constant_time_compare(
    const unsigned char *a,
    const unsigned char *b,
    size_t len
)
{
    if (a == NULL || b == NULL) {
        return -1;
    }

    return CRYPTO_memcmp(a, b, len);
}