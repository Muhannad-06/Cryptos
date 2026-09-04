# Archive Format Specification

## Structure

![project structure](structure.png)

### Layout

─────────────────────────
        Fields           
                         
   Local Field Header    
   Field Data            
                         
   Local Field Header    
   Field Data            
          ...            
─────────────────────────
        Directory        
                         
  All archive info.      
  Groups                 
  Entries                
  Field Headers          
                         
─────────────────────────

#### Archive Header
contains only magic number of 4 bytes.

↓

#### Fields
A field consists of 
    1- field local header 
    2- field data.


#### Directory
Directory contains fields headers(which contain their offsets in the file) and logical hierarchy (Groups and Entries) information.

Groups and Entries sizes are defined in their blocks.

### Blocks structure

## Group

| Offset | Size | Name                    | Description                                |
| -----: | ---: | ----------------------- | ------------------------------------------ |
|      0 |    4 | Magic                   | Magic number identifying a Group structure |
|      4 |    4 | Group ID                | Unique identifier of the group             |
|      8 |    4 | Group Creation Time     | Group creation timestamp                   |
|     12 |    4 | Group Last Modification | Group last modification timestamp          |
|     16 |    2 | Group Name Length       | Number of bytes in the group name          |
|     18 |    n | Group Name              | UTF-8 encoded group name                   |

## Entry

| Offset | Size | Name                    | Description                                 |
| -----: | ---: | ----------------------- | ------------------------------------------- |
|      0 |    4 | Magic                   | Magic number identifying an Entry structure |
|      4 |    4 | Group ID                | ID of the group containing this entry       |
|      8 |    4 | Entry ID                | Unique identifier of the entry              |
|     12 |    4 | Entry Creation Time     | Entry creation timestamp                    |
|     16 |    4 | Entry Last Modification | Entry last modification timestamp           |
|     20 |    2 | Entry Name Length       | Number of bytes in the entry name           |
|     22 |    n | Entry Name              | UTF-8 encoded entry name                    |

## Directory

| Offset | Size | Name | Description |
| --- | --- | --- | --- |
| 0 | 4 | Magic number | Magic number identifying the Directory structure |
| 4 | 2 | File Version | Version of the archive format |
| 6 | 8 | Archive size | Total size of the archive in bytes |
| 14 | 4 | Archive creation date | Archive creation timestamp |
| 18 | 4 | Archive last modification date | Archive last modification timestamp |
| 22 | 4 | number of directories (number of changes) | Number of directories or changes tracked |
| 26 | 8 | Offset of the previous directory | Byte offset pointing to the previous directory |
| 34 | 4 | Number of groups | Number of groups in the archive |
| 38 | n | Groups | Serialized Group structures |
| - | 4 | Number of entries | Number of entries in the archive |
| - | n | Entries | Serialized Entry structures |
| - | 4 | Number of fields | Number of fields in the archive |
| - | n | Fields (Directory) Headers | Serialized Field Directory Header structures |
| - | 2 | Archive name length (m) | Number of bytes in the archive name |
| - | m | Archive name | UTF-8 encoded archive name |
| - | 2 | Archive description length (n) | Number of bytes in the archive description |
| - | n | Archive description | UTF-8 encoded archive description |

## Field Directory Header

| Offset | Size | Name                    | Description                                       |
| -----: | ---: | ----------------------- | ------------------------------------------------- |
|      0 |    4 | Magic                   | Magic number identifying a Directory Field Header |
|      4 |    4 | Entry ID                | ID of the entry containing this field             |
|      8 |    2 | Compression Method      | Identifier of the compression algorithm           |
|     10 |    8 | Compressed Field Size   | Size of the compressed field data in bytes        |
|     18 |    8 | Uncompressed Field Size | Size of the original field data in bytes          |
|     26 |    8 | Field Offset            | Absolute offset to the Local Field Header         |
|     34 |    4 | CRC                     | CRC/hash of the field data                        |
|     38 |    4 | Field Creation Time     | Field creation timestamp                          |
|     42 |    4 | Field Last Modification | Field last modification timestamp                 |
|     46 |    2 | Field Type              | Identifier describing the type of field           |
|     48 |    2 | Field Name Length       | Number of bytes in the field name                 |
|     50 |    n | Field Name              | UTF-8 encoded field name                          |

## Archive

| Offset | Size | Name             | Description                                             |
| -----: | ---: | ---------------- | ------------------------------------------------------- |
|      0 |    4 | Magic            | Magic number identifying the archive                    |
|      4 |    8 | Directory Offset | Absolute offset to the Directory structure              |
|     12 |    n | Fields           | Serialized Local Field Header and Field Data structures |

## Local Field Header

| Offset | Size | Name                    | Description                                   |
| -----: | ---: | ----------------------- | --------------------------------------------- |
|      0 |    4 | Magic                   | Magic number identifying a Local Field Header |
|      4 |    2 | Entry ID                | ID of the entry containing this field         |
|      6 |    2 | Compression Method      | Identifier of the compression algorithm       |
|      8 |    8 | Compressed Field Size   | Size of the compressed field data in bytes    |
|     16 |    8 | Uncompressed Field Size | Size of the original field data in bytes      |
|     24 |    4 | CRC                     | CRC/hash of the field data                    |
|     28 |    4 | Field Creation Time     | Field creation timestamp                      |
|     32 |    4 | Field Last Modification | Field last modification timestamp             |
|     36 |    2 | Field Type              | Identifier describing the type of field       |
|     38 |    2 | Field Name Length       | Number of bytes in the field name             |
|     40 |    n | Field Name              | UTF-8 encoded field name                      |

    

## magic number

`0x4D4D3333` ("MM33")

## Endianness

All integers are saved in `Little Endian` format.

## Integer sizes

| Type   | Size    |
| ------ | ------- |
| uint16 | 2 bytes |
| uint32 | 4 bytes |
| uint64 | 8 bytes |

## Encoding

All Strings are encoded in `UTF-8`

## Version

Current version is: 0x0000

Readers should reject versions greater than the highest version they support.

## Compression methods

0x0000 = none
0x0001 = DEFLATE

## Field types

0x0000 = Binary
0x0001 = Text (UTF-8)
0x0002 = Password

## Boundaries

max archive size = 1.8447 * 10^19 byte = 1.71799 * 10^10 GiB.
max number of groups, entries, fields =~ 4.29*10^9.
max changes = 2^32