#ifndef BIN_IO_H
#define BIN_IO_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "types.h"

/* Primitive Writers (Little-Endian) */
ErrorCode IO_boolWriteU8(FILE *fp, uint8_t value);
ErrorCode IO_boolWriteU16(FILE *fp, uint16_t value);
ErrorCode IO_boolWriteU32(FILE *fp, uint32_t value);
ErrorCode IO_boolWriteBytes(FILE *fp, const void *buffer, size_t size);
ErrorCode IO_boolWriteString(FILE *fp, const char *str, uint16_t length);

/* Primitive Readers (Little-Endian) */
ErrorCode IO_boolReadU8(FILE *fp, uint8_t *out_val);
ErrorCode IO_boolReadU16(FILE *fp, uint16_t *out_val);
ErrorCode IO_boolReadU32(FILE *fp, uint32_t *out_val);
ErrorCode IO_boolReadBytes(FILE *fp, void *buffer, size_t size);
char* IO_charReadString(FILE *fp, uint16_t length);

/* Navigation Helpers */
uint32_t IO_u32Tell(FILE *fp);
bool IO_boolSeek(FILE *fp, uint32_t offset);


#endif /* BIN_IO_H */