#ifndef AES_H
#define AES_H

#include "crypto_types.h"
#include "crypto_errors.h"
#include <stddef.h>

/**
 * Generate a new AES key.
 *
 * @param key Pointer to AesKey structure where the generated key will be stored.
 *
 * @return CRYPTO_OK on success, or an appropriate error code on failure.
 */
CryptoError aes_generate_key(AesKey *key);

/**
 * Generate a new AES initialization vector (IV).
 *
 * @param iv Pointer to AesIV structure where the generated IV will be stored.
 *
 * @return CRYPTO_OK on success, or an appropriate error code on failure.
 */
CryptoError aes_generate_iv(AesIV *iv);

/**
 * Encrypt plaintext using AES.
 *
 * @param plaintext Pointer to the plaintext buffer.
 * @param plaintext_len Length of the plaintext in bytes.
 * @param key Pointer to the AES key.
 * @param iv Pointer to the initialization vector.
 * @param ciphertext Output buffer where the encrypted data will be stored.
 * @param tag Output pointer where the authentication tag will be stored.
 *
 * IMPORTANT:
 * Before calling this function:
 * - ciphertext must be exactly plaintext_len bytes.
 *
 * @return CRYPTO_OK on success, or an appropriate error code on failure.
 */
CryptoError aes_encrypt(
    const unsigned char *plaintext,
    size_t plaintext_len,
    const AesKey *key,
    const AesIV *iv,
    unsigned char *ciphertext,
    AesTag *tag
);

/**
 * Decrypt ciphertext using AES and verify its authentication tag.
 *
 * @param ciphertext Pointer to the ciphertext buffer.
 * @param ciphertext_len Length of the ciphertext in bytes.
 * @param key Pointer to the AES key.
 * @param iv Pointer to the initialization vector.
 * @param tag Pointer to the authentication tag to verify.
 * @param plaintext Output buffer where the decrypted data will be stored.
 *
 * IMPORTANT:
 * Before calling this function:
 * - plaintext must be exactly ciphertext_len bytes.
 *
 * @return CRYPTO_OK on success, or an appropriate error code on failure.
 */
CryptoError aes_decrypt(
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const AesKey *key,
    const AesIV *iv,
    const AesTag *tag,
    unsigned char *plaintext
);

#endif /* AES_H */