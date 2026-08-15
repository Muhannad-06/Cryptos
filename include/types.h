#ifndef TYPES_H
#define TYPES_H

#define MAGIC_ARCHIVE_HEADER  0x4D4D3333  /* MM33 */

typedef enum
{
    Valid = 0,
    Invalid,
    BUFFER_OVERFLOW,
    TIMEOUT,
    NULL_POINTER,
} ErrorCode;


#endif /* TYPES_H */