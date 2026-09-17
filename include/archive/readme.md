# archive/
Project's core.\
Code used to manage an archive and its entities.

---
# Archive Interface

## Entities
- Archive (struct)
- Group (struct)
- Entry (struct)
- Field (struct)

### Archive
_declared in:_ `archive.h`\
Contains archive info (name, description, creation & modification dates, write status, etc...) and pointers to it's childs.

### Group
_declared in:_ `group.h`\
Contains group info (name, creation & modification dates, etc...) and pointers to it's childs.

### Entry
_declared in:_ `entry.h`\
Contains entry info (name, creation & modification dates, etc...) and pointers to it's childs.

### Field
_declared in:_ `field.h`\
Contains field info (name, creation & modification dates, size, etc...).\
Also holds field content's type which should be member of enum: `FeildType` _declared in:_ `field.h`.

## flow
An existing archive should be read using "functions to read archive". Those functions parse binary data into memory entities (Archive, Group, Entry and field).

You should do operations like (creating new archive, printing field content, modifying archive content, renaming an entity, changing archive description, deleting an entity, adding an entity, etc...) using "functions to interact with archive".
Those functions do modifications in the memory entities, but they are not written yet to the archive file.

to write your updates or to create an archive file from scratch use "functions to write changes".

### functions to read archive:
to parse archive using `FILE` pointer use `Archive *read_archive(FILE *fp)` declared in `archive.h`.
This function reads the file content and returns an `Archive` entity.

### functions to interact with archive:

#### Archive operations
_declared in_: `archive.h`

_**To create archive entity:**_ `archive_create(name, description, file)`. This function returns an empty archive entity ready to be used.\
_**Destroy archive entity:**_ `void archive_destroy(archive)`. This function frees the memory from the archive entity **and from all of it's childs**. It does **NOT** touch the file itself.\
_**Set archive name:**_ `archive_set_name(archive, name)`.\
_**Set archive description:**_ `archive_set_description(archive, description)`.\
_**get string of brief archive info:**_ `archive_to_string(archive)`.

#### Group operations

_declared in_: `group.h`

_**To create group entity**:_ `group_create(archive, name)`. This function returns an empty group entity with `archive` as parent and `name` as name string.\
_**Destroy group entity:**_ `void group_destroy(group)`. This function frees the memory from the group entity **and from all of it's childs**. It does **NOT** touch the file itself.\
_**Set group name:**_ `group_set_name(group, name)`.\
**_get string of brief group info_:** `group_to_string(group)`.

#### Entry operations

_declared in_: `entry.h`

_**To create entry entity:**_ `entry_create(group, name)`. This function returns an empty entry entity with `group` as parent and `name` as name.\
_**Destroy entry entity:**_ `void entry_destroy(entry)`. This function frees the memory from the entry entity **and from all of it's childs**. It does **NOT** touch the file itself.\
_**Set entry name:**_ `entry_set_name(entry, name)`.\
**_set group:_** `entry_set_group(entry, new_group)` move entry from its parent group to `new_group`.\
_**get string of brief entry info:**_ `entry_to_string(entry)`.

#### Field operations

_declared in_: `field.h`

_**To create field entity:**_ `field_create(entry, type, name)`. This function returns an empty field entity of type `type` with `entry` as parent and `name` as name.\
**_To delete field:_** `field_delete(Field *field)`. This function frees field from memory and informs parents that the field was deleted.\
**DO NOT USE `field_destroy`, this function does NOT update parents and causes problems.**\
_**Set field name:**_ `field_set_name(field, name)`.\
_**get string of brief field info:**_ `field_to_string(field)`.\
**_To set field content:_** `field_set_content(field, type,  data)`.\
_**set entry:**_ `field_set_entry(field, new_entry)` move field from entry to `new_entry`.
**_set compression type_**: `field_set_compression(field, compression_type)` change field compression type.


### functions to write changes:
**_update existing archive file or create new archive file:_** `write_archive(archive)`.\
**_write a clean version of archive (a new file with erased history):_** `write_archive_clean(archive, new_file_name)` writes the archive data into a new file with name `new_file_name`.

---