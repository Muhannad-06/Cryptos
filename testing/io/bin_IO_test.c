#include <stdarg.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../../include/io/Bin_IO.h"
#include "../../include/types.h"
#include "../../include/utils/error.h"
#include "../../include/utils/assertion.h"

FILE *wfp;
FILE *rfp;
FILE *nfp;

void set_up(){
    wfp = fopen("test_bin_io_write.bin", "wb+");
    rfp = fopen("test_bin_io_read.bin", "wb+");
    nfp = fopen("test_bin_io_nav.bin", "wb+");
    
    if(!(wfp && rfp && nfp)){
        error("Couldn't create test files.");
    }
    
    // writing content for read test.
    uint8_t read_data[] = {
            0xAA, 0xBB, 0xCC, 0xDD,                         /* U32: 0xDDCCBBAA */
            0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, /* U64: 0x1122334455667788ULL */
            0x01, 0x02, 0x03, 0x04,
            'S', 'i', 'm', 'p', 'l', 'e', ' ',              /* String: "Simple Archive" */
            'A', 'r', 'c', 'h', 'i', 'v', 'e'
        };
    fwrite(read_data, 1, sizeof(read_data), rfp);
    rewind(rfp);

    // writing content for navigation test.
    uint8_t nav_data[] = {
            0x00, 0x00, 0x00, 0x00, 
            0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11 
        };
    fwrite(nav_data, 1, sizeof(nav_data), nfp);
    rewind(nfp);
}

void clean(){
    fclose(wfp);
    fclose(rfp);
    fclose(nfp);
    remove("test_bin_io_write.bin");
    remove("test_bin_io_read.bin");
    remove("test_bin_io_nav.bin");
}

/* Testing write functions. */
ErrorCode test_bin_write(){
    ErrorCode status = SUCCESS;

    uint8_t buffer[4] = {0x01, 0x02, 0x03, 0x04};
    FILE *src_tmp = tmpfile();
    
    status += assert("Write U8 test", IO_enumWriteU8(wfp, 0xAA) == SUCCESS);
    status += assert("Write U16 test", IO_enumWriteU16(wfp, 0xBBAA) == SUCCESS);
    status += assert("Write U32 test", IO_enumWriteU32(wfp, 0xDDCCBBAA) == SUCCESS);
    status += assert("Write U64 test", IO_enumWriteU64(wfp, 0x1122334455667788ULL) == SUCCESS);
    status += assert("Write Bytes test", IO_enumWriteBytes(wfp, buffer, 4) == SUCCESS);
    
    status += assert("Write String test", IO_enumWriteString(wfp, "Simple Archive") == SUCCESS);
    
    status += assert("Write File test", IO_enumWriteFile(wfp, src_tmp) == SUCCESS);
    
    fclose(src_tmp);

    fflush(wfp); // clean buffer.
    rewind(wfp); // seek to the beginning of the file.
    
    uint8_t expected_data[] = {
        0xAA,                                           /* U8 */
        0xAA, 0xBB,                                     /* U16 */
        0xAA, 0xBB, 0xCC, 0xDD,                         /* U32 */
        0x88, 0x77, 0x66, 0x55, 0x44, 0x33, 0x22, 0x11, /* U64 */
        0x01, 0x02, 0x03, 0x04,                         /* Bytes */
        'S', 'i', 'm', 'p', 'l', 'e', ' ',              /* String: "Simple Archive"[cite: 2] */
        'A', 'r', 'c', 'h', 'i', 'v', 'e'
    };

    // check if written data is same as expected data.
    uint8_t read_buffer[64] = {0};
    fread(read_buffer, 1, sizeof(expected_data), wfp);
    status += assert("Verify raw serialized bytes", memcmp(read_buffer, expected_data, sizeof(expected_data)) == 0);
    
    return (status == SUCCESS) ? SUCCESS : FAILURE;
}

/* Testing read functions. */
ErrorCode test_bin_read(){
    int status = SUCCESS;

    uint8_t val8 = 0;
    uint16_t val16 = 0;
    uint32_t val32 = 0;
    uint64_t val64 = 0;
    uint8_t buffer[4] = {0};
    uint8_t expected_buffer[4] = {0x01, 0x02, 0x03, 0x04};
    char *str = NULL;
    FILE *dst_tmp = tmpfile();

    status += assert("Read U8 test", IO_enumReadU8(rfp, &val8) == SUCCESS && val8 == 0xAA);// printf("%X\n", val8);
    rewind(rfp);
    status += assert("Read U16 test", IO_enumReadU16(rfp, &val16) == SUCCESS && val16 == 0xBBAA);// printf("%X\n", val16);
    rewind(rfp);
    status += assert("Read U32 test", IO_enumReadU32(rfp, &val32) == SUCCESS && val32 == 0xDDCCBBAA);

    status += assert("Read U64 test", IO_enumReadU64(rfp, &val64) == SUCCESS && val64 == 0x1122334455667788ULL);

    status += assert("Read Bytes test", IO_enumReadBytes(rfp, buffer, 4) == SUCCESS && !memcmp(buffer, expected_buffer, 4));
    status += assert("Read String test", IO_charReadString(rfp, 14, &str) == SUCCESS && strcmp(str, "Simple Archive") == 0);// printf("%s\n", str);

    status += assert("Print File test", IO_enumPrintFile(dst_tmp, 0, 4, rfp) == SUCCESS);
    
    free(str);
    fclose(dst_tmp);

    return (status == SUCCESS) ? SUCCESS : FAILURE;
}

/* Testing navigation. */
ErrorCode test_bin_nav(){
    int status = SUCCESS;

    status += assert("Initial position check", IO_u64Tell(nfp) == 0);
    
    /* Seeking 8 bytes to test cursor movement. */
    status += assert("Seek to offset 8", IO_enumSeek(nfp, 8) == SUCCESS);
    status += assert("Verify position after seek", IO_u64Tell(nfp) == 8);

    return (status == SUCCESS) ? SUCCESS : FAILURE;
}

int main()
{
    set_up();
    
    assert("** write functions test **", test_bin_write() == SUCCESS);
    assert("** read functions test **", test_bin_read() == SUCCESS);
    assert("** navigation functions test **", test_bin_nav() == SUCCESS);

    clean();
}