#include "aes.h"
#include "crypto_types.h"
#include "crypto_errors.h"
#include "crypto_utils.h"

#include <openssl/evp.h>
#include <string.h>

CryptoError aes_generate_key(AesKey *key)
{
    if (key == NULL) {
        return CRYPTO_ERR_INVALID_PARAMETER;
    }

    return crypto_random_bytes(key->data, AES_KEY_SIZE);
}

CryptoError aes_generate_iv(AesIV *iv)
{
    if (iv == NULL) {
        return CRYPTO_ERR_INVALID_PARAMETER;
    }

    return crypto_random_bytes(iv->data, AES_IV_SIZE);
}

CryptoError aes_encrypt(
    const unsigned char *plaintext,
    size_t plaintext_len,
    const AesKey *key,
    const AesIV *iv,
    unsigned char *ciphertext,
    AesTag *tag
)
{
    if (plaintext == NULL || key == NULL || iv == NULL ||
        ciphertext == NULL || tag == NULL) {
        return CRYPTO_ERR_INVALID_PARAMETER;
    }

    CryptoError result = CRYPTO_ERR_ENCRYPT;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        return CRYPTO_ERR_ENCRYPT;
    }

    int len = 0;
    int ciphertext_len = 0;

    /* 1. Init cipher: AES-256-GCM, with key and IV */
    if (EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        goto cleanup;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1) {
        goto cleanup;
    }

    if (EVP_EncryptInit_ex(ctx, NULL, NULL, key->data, iv->data) != 1) {
        goto cleanup;
    }

    /* 2. Encrypt the plaintext */
    if (EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, (int)plaintext_len) != 1) {
        goto cleanup;
    }
    ciphertext_len = len;

    /* 3. Finalize encryption (GCM produces no extra bytes here, but must be called) */
    if (EVP_EncryptFinal_ex(ctx, ciphertext + ciphertext_len, &len) != 1) {
        goto cleanup;
    }
    ciphertext_len += len;

    /* 4. Extract the authentication tag */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, AES_TAG_SIZE, tag->data) != 1) {
        goto cleanup;
    }

    result = CRYPTO_OK;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return result;
}

CryptoError aes_decrypt(
    const unsigned char *ciphertext,
    size_t ciphertext_len,
    const AesKey *key,
    const AesIV *iv,
    const AesTag *tag,
    unsigned char *plaintext
)
{
    if (ciphertext == NULL || key == NULL || iv == NULL ||
        tag == NULL || plaintext == NULL) {
        return CRYPTO_ERR_INVALID_PARAMETER;
    }

    CryptoError result = CRYPTO_ERR_DECRYPT;
    EVP_CIPHER_CTX *ctx = EVP_CIPHER_CTX_new();
    if (ctx == NULL) {
        return CRYPTO_ERR_DECRYPT;
    }

    int len = 0;
    int plaintext_len = 0;

    /* 1. Init cipher: AES-256-GCM, with key and IV */
    if (EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL) != 1) {
        goto cleanup;
    }

    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, AES_IV_SIZE, NULL) != 1) {
        goto cleanup;
    }

    if (EVP_DecryptInit_ex(ctx, NULL, NULL, key->data, iv->data) != 1) {
        goto cleanup;
    }

    /* 2. Decrypt the ciphertext */
    if (EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, (int)ciphertext_len) != 1) {
        goto cleanup;
    }
    plaintext_len = len;

    /* 3. Set the expected tag BEFORE calling Final */
    if (EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, AES_TAG_SIZE,
                              (void *)tag->data) != 1) {
        goto cleanup;
    }

    /* 4. Finalize decryption — this is where tag verification happens.
       If the tag doesn't match, this call fails and we know the data
       was tampered with or the key/iv was wrong. */
    if (EVP_DecryptFinal_ex(ctx, plaintext + plaintext_len, &len) != 1) {
        result = CRYPTO_ERR_AUTH;
        goto cleanup;
    }
    plaintext_len += len;

    result = CRYPTO_OK;

cleanup:
    EVP_CIPHER_CTX_free(ctx);
    return result;
}