#ifndef BIN_IO_H
#define BIN_IO_H

#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>

#include "../types.h"

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  * Primitive Writers (Little-Endian) * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
* These functions serialize data from memory to a file.
* They likely return an ErrorCode (e.g., SUCCESS or ERROR_WRITE_FAILED) 
* based on whether the standard C fwrite or fputc functions succeed.
*
*   Main Functionalities: Data serialization, error handling, and file I/O operations
*   Mechanism: Division of data into bytes, writing to file, and checking for write success.          
*/
ErrorCode IO_enumWriteU8(FILE *fp, uint8_t value);

ErrorCode IO_enumWriteU16(FILE *fp, uint16_t value);

ErrorCode IO_enumWriteU32(FILE *fp, uint32_t value);

ErrorCode IO_enumWriteBytes(FILE *fp, const void *buffer, size_t size);

ErrorCode IO_enumWriteU64(FILE *fp, uint64_t value);

ErrorCode IO_enumWriteString(FILE *fp, const char *str);

/*============================================================================================================*/



/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  * Primitive Readers (Little-Endian) * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
* These functions deserialize data from a file to memory.
* They likely return an ErrorCode (e.g., SUCCESS or ERROR_READ_FAILED) 
* based on whether the standard C fread or fgetc functions succeed.
*
*   Main Functionalities: Data deserialization, error handling, and file I/O operations
*   Mechanism: Division of data into bytes, reading from file, and checking for read success.          
*/

ErrorCode IO_enumReadU8(FILE *fp, uint8_t *out_val);
ErrorCode IO_enumReadU16(FILE *fp, uint16_t *out_val);
ErrorCode IO_enumReadU32(FILE *fp, uint32_t *out_val);
ErrorCode IO_enumReadU64(FILE *fp, uint64_t *out_val);
ErrorCode IO_enumReadBytes(FILE *fp, void *buffer, size_t size);
ErrorCode IO_charReadString(FILE *fp, uint16_t length, char **out_str);

/*============================================================================================================*/

/*<<<<<<<<<<<<<<<<<<<<<<<<<<<<<  * Navigation Helpers * >>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>>
* These functions provide additional file navigation capabilities.
*  Main Functionalities: File position tracking and seeking within a file.
*  Mechanism: Utilizing standard C functions like ftell and fseek to manage file pointers.
*/

/* Returns the current position of the file cursor in bytes from the start of the file */
uint64_t IO_u64Tell(FILE *fp);

/* Moves the file cursor to an absolute byte offset from the beginning of the file */
ErrorCode IO_enumSeek(FILE *fp, uint64_t offset);


#endif /* BIN_IO_H */