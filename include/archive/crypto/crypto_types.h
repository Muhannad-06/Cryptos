#ifndef CRYPTO_TYPES_H
#define CRYPTO_TYPES_H

#define AES_KEY_SIZE 32
#define AES_IV_SIZE 12
#define AES_TAG_SIZE 16

typedef struct {
    unsigned char data[AES_KEY_SIZE];
} AesKey;

typedef struct {
    unsigned char data[AES_IV_SIZE];
} AesIV;

typedef struct {
    unsigned char data[AES_TAG_SIZE];
} AesTag;

typedef struct RSAKeyPair RSAKeyPair;

typedef struct RSAKey RSAKey;


#endif