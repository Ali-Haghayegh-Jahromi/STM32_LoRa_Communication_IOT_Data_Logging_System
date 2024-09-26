/*
 * zcrypt.h
 */


#ifndef _ZCRYPT_H
#define _ZCRYPT_H

#include <stdint.h>

typedef enum
{
  PKCS7_EMPTY_DATA = -4,
  PKCS7_INVALID_PADDED_BUFFER = -3,
  PKCS7_INVALID_PAD_BYTE = -2,
  PKCS7_INVALID_BUFFER_SIZE = -1,
  CRYPTO_OK = 0,
} cryptoerr_t;




/* ****************************  pkcs7  *******************************/
/* source: https://github.com/bonybrown/tiny-AES128-C/ */


/* Pad a buffer with bytes as defined in PKCS#7 
 * Returns the number of pad bytes added, or 
 * Returns zero if the buffer size is not large enough to hold the correctly padded data
 */
uint8_t pkcs7_pad(uint8_t *buf, uint32_t buf_size, uint32_t data_len, uint8_t block_size);

/* Given a block of pkcs7 padded data, return the actual data length in the block based on the padding applied.
 * buffer_size must be a multiple of modulus
 * last byte 'x' in buffer must be between 1 and modulus
 * last 'x' bytes in buffer must be same as 'x'
 * returned size will be buffer_size - 'x'
 */
int32_t pkcs7_data_length(uint8_t *buf, uint32_t buf_size, uint8_t block_size);

/*****************************  aes  *******************************/
/* source: https://github.com/kokke/tiny-AES-c */

#define AES_BLOCKLEN      16 // Block length in bytes - AES is 128b block only
#define AES_IV_SIZE       16
#define AES_KEYLEN        32
#define AES_keyExpSize    240

typedef struct
{
  uint8_t RoundKey[AES_keyExpSize];
  uint8_t Iv[AES_BLOCKLEN];
} AES_ctx_t;

void AES_init_ctx(AES_ctx_t *ctx, const uint8_t *key);
void AES_ctx_set_iv(AES_ctx_t *ctx, const uint8_t *iv);
void AES_init_ctx_iv(AES_ctx_t *ctx, const uint8_t *key, const uint8_t *iv);
void AES_CBC_encrypt_buffer(AES_ctx_t *ctx, uint8_t *buf, uint32_t length);
void AES_CBC_decrypt_buffer(AES_ctx_t *ctx, uint8_t *buf, uint32_t length);

#endif // _ZCRYPT_H
