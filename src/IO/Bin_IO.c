#include "../../include/Bin_IO.h"
#include <stdlib.h>
#include <string.h>

#include "../../include/types.h"

ErrorCode IO_enumWriteU8(FILE *fp, uint8_t value)
{
    if (fputc(value, fp) == EOF) {
        return ERROR_WRITE_FAILED;
    }
    return SUCCESS;
}

ErrorCode IO_enumWriteU16(FILE *fp, uint16_t value)
{
    uint8_t buffer[2];
    buffer[0] = (uint8_t)(value & 0xFF);        
    buffer[1] = (uint8_t)((value >> 8) & 0xFF); 
    
    if (fwrite(buffer, 1, 2, fp) != 2) {
        return ERROR_WRITE_FAILED;
    }
    return SUCCESS;
}

ErrorCode IO_enumWriteU32(FILE *fp, uint32_t value)
{
    uint8_t buffer[4];
    buffer[0] = (uint8_t)(value & 0xFF);
    buffer[1] = (uint8_t)((value >> 8) & 0xFF);
    buffer[2] = (uint8_t)((value >> 16) & 0xFF);
    buffer[3] = (uint8_t)((value >> 24) & 0xFF);
    
    if (fwrite(buffer, 1, 4, fp) != 4) {
        return ERROR_WRITE_FAILED;
    }
    return SUCCESS;
}

ErrorCode IO_enumWriteBytes(FILE *fp, const void *buffer, size_t size)
{
    if (fwrite(buffer, 1, size, fp) != size) {
        return ERROR_WRITE_FAILED;
    }
    return SUCCESS;
}

ErrorCode IO_enumWriteString(FILE *fp, const char *str, uint16_t length)
{
    if (fwrite(str, 1, length, fp) != length) {
        return ERROR_WRITE_FAILED;
    }
    return SUCCESS;
}

/* Primitive Readers (Little-Endian) */
ErrorCode IO_enumReadU8(FILE *fp, uint8_t *out_val)
{
    int val = fgetc(fp);
    if (val == EOF) return ERROR_READ_FAILED;
    
    *out_val = (uint8_t)val;
    return SUCCESS;
}

ErrorCode IO_enumReadU16(FILE *fp, uint16_t *out_val)
{
    uint8_t buffer[2];
    if (fread(buffer, 1, 2, fp) != 2) return ERROR_READ_FAILED;
    
    // Reconstruct Little-Endian
    *out_val = (uint16_t)(buffer[0] | (buffer[1] << 8));
    return SUCCESS;
}

ErrorCode IO_enumReadU32(FILE *fp, uint32_t *out_val)
{
    uint8_t buffer[4];
    if (fread(buffer, 1, 4, fp) != 4) return ERROR_READ_FAILED;
    
    *out_val = (uint32_t)(buffer[0] | 
                         (buffer[1] << 8) | 
                         (buffer[2] << 16) | 
                         (buffer[3] << 24));
    return SUCCESS;
}

ErrorCode IO_enumReadBytes(FILE *fp, void *buffer, size_t size)
{
    if (fp == NULL) return FILE_NOT_FOUND;
    if (buffer == NULL) return NULL_POINTER;
    if (fread(buffer, 1, size, fp) != size) return ERROR_READ_FAILED;
    return SUCCESS;
}

char* IO_charReadString(FILE *fp, uint16_t length)
{
    if (fp == NULL) return NULL /*FILE_NOT_FOUND*/; 
    char *str = (char*)malloc(length + 1);
    if (!str) return NULL /*MEMORY_ALLOCATION_FAILED*/;
    
    if (fread(str, 1, length, fp) != length) {
        free(str);
        return NULL /*ERROR_READ_FAILED*/;
    }
    
    str[length] = '\0'; // Ensure string is null-terminated
    return str;
}


/* Navigation Helpers */
uint32_t IO_u32Tell(FILE *fp)
{
    long pos = ftell(fp);
    if (pos == -1L) {
        return FAILURE; // 
    }
    return (uint32_t)pos;
}

ErrorCode IO_enumSeek(FILE *fp, uint32_t offset)
{
    if (fseek(fp, (long)offset, SEEK_SET) != 0) {
        return FAILURE; // Seek failed
    }
    return SUCCESS;
}
