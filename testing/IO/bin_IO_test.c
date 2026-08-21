#include <stdio.h>

#include "../../include/Bin_IO.h"
#include "../../include/types.h"

int main()
{
    FILE *myfile = fopen("myBinaryFile.bin", "w+b");

    if( myfile == NULL)
    {   
        printf("File Not Found");
        return 0;
    }

    uint8_t myTest = 8;

    // 1. Write the byte (file pointer moves forward by 1)
    IO_enumWriteU8(myfile, myTest);

    uint8_t myResult = 0;
    
    // 2. Get the current position
    uint32_t pos = IO_u32Tell(myfile); 

    // 3. Seek back exactly 1 byte to the start of the data we just wrote
    IO_enumSeek(myfile, (pos - 1));

    // Alternatively, since it's the very beginning of the file, you could just do:
    // IO_enumSeek(myfile, 0);

    // 4. Read the byte
    IO_enumReadU8(myfile, &myResult);

    // Should now print '8'
    printf("%u\n", myResult);

    fclose(myfile); // Don't forget to close your file!

    return 0;
}