#ifndef ARCHIVE_INTERFACE_H
#define ARCHIVE_INTERFACE_H
#include "../types.h"
#include "../utils/vector.h"
#include "archive.h"
#include "group.h"
#include "entry.h"
#include"field.h"

ErrorCode write_archive(Archive * archive); /* create or update archive content. */


#endif