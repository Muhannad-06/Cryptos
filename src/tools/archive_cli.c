/*
 * archive-cli
 * -----------
 * A small, diskpart-style interactive CLI used to exercise the Cryptos
 * archive format's read and write abilities.
 *
 * Launch:   archive-cli <file-name>
 *
 * NOTE: crypto/ is intentionally not used here (see Makefile). Fields can
 * only be TEXT or PASSWORD in this build; BINARY fields are not writable
 * (IO_enumWriteFile is a stub) and are not readable either (content is not
 * loaded for BINARY fields), so this CLI does not expose the binary type.
 */

/* On MinGW, printf() otherwise routes through the old msvcrt.dll
 * implementation, which doesn't understand %zu or %llu at all (it doesn't
 * just warn - it silently misreads the format string and argument list).
 * Defining this before <stdio.h> makes MinGW use its own C99-compliant
 * printf instead. No effect on Linux/glibc builds. */
#define __USE_MINGW_ANSI_STDIO 1

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <errno.h>

#include "../../include/types.h"
#include "../../include/archive/archive.h"
#include "../../include/archive/group.h"
#include "../../include/archive/entry.h"
#include "../../include/archive/field.h"
#include "../../include/utils/vector.h"
#include "../../include/utils/bst.h"

#define LINE_MAX_LEN 4096

/* -------------------------------------------------------------------- */
/*  Global interactive state                                            */
/* -------------------------------------------------------------------- */

typedef enum {
    CTX_ARCHIVE,
    CTX_GROUP,
    CTX_ENTRY,
    CTX_FIELD
} Context;

static Archive *g_archive = NULL;
static Context  g_ctx = CTX_ARCHIVE;
static void    *g_current = NULL;   /* Group*, Entry* or Field*, depending on g_ctx */

/* items shown by the last "list"/"search" command, selectable via "select N" */
typedef struct {
    EntityType type;
    void *entity;
} ListItem;

static Vector *g_last_list = NULL;

/* -------------------------------------------------------------------- */
/*  Small helpers                                                       */
/* -------------------------------------------------------------------- */

static int str_ieq(const char *a, const char *b) {
    if (!a || !b) return 0;
    while (*a && *b) {
        if (tolower((unsigned char)*a) != tolower((unsigned char)*b)) return 0;
        a++; b++;
    }
    return *a == '\0' && *b == '\0';
}

static int ci_contains(const char *hay, const char *needle) {
    if (!hay || !needle) return 0;
    size_t hn = strlen(needle);
    if (hn == 0) return 1;
    for (const char *p = hay; *p; p++) {
        size_t i = 0;
        while (i < hn && p[i] && tolower((unsigned char)p[i]) == tolower((unsigned char)needle[i])) i++;
        if (i == hn) return 1;
    }
    return 0;
}

static char *trim(char *s) {
    while (*s && isspace((unsigned char)*s)) s++;
    char *end = s + strlen(s);
    while (end > s && isspace((unsigned char)*(end - 1))) *(--end) = '\0';
    return s;
}

/* pulls the next whitespace-delimited token out of *cursor, destructively
 * null-terminating it in place. advances *cursor past it. returns NULL if
 * there is nothing left. Whatever remains in *cursor after a call keeps its
 * original inner spacing, which lets callers grab "the rest of the line"
 * verbatim (used for free-text like search terms or field content). */
static char *next_token(char **cursor) {
    char *s = *cursor;
    while (*s && isspace((unsigned char)*s)) s++;
    if (!*s) { *cursor = s; return NULL; }
    char *start = s;
    while (*s && !isspace((unsigned char)*s)) s++;
    if (*s) { *s = '\0'; s++; }
    *cursor = s;
    return start;
}

static const char *field_type_str(FieldType t) {
    switch (t) {
        case TEXT:     return "text";
        case PASSWORD: return "password";
        case BINARY:   return "binary(unsupported)";
        default:       return "unknown";
    }
}

static int parse_field_type(const char *s, FieldType *out) {
    if (str_ieq(s, "text"))     { *out = TEXT;     return 1; }
    if (str_ieq(s, "password")) { *out = PASSWORD; return 1; }
    return 0;
}

static int confirm(const char *prompt) {
    char buf[16];
    printf("%s [y/N]: ", prompt);
    fflush(stdout);
    if (!fgets(buf, sizeof(buf), stdin)) return 0;
    return buf[0] == 'y' || buf[0] == 'Y';
}

/* -------------------------------------------------------------------- */
/*  Entity lookup helpers                                               */
/* -------------------------------------------------------------------- */

static Group *find_group_by_name(const char *name) {
    for (size_t i = 0; i < g_archive->groups->size; i++) {
        Group *g = vector_at(g_archive->groups, i);
        if (strcmp(g->name, name) == 0) return g;
    }
    return NULL;
}

static Entry *find_entry_by_name(const char *name) {
    for (size_t i = 0; i < g_archive->entries->size; i++) {
        Entry *e = vector_at(g_archive->entries, i);
        if (strcmp(e->name, name) == 0) return e;
    }
    return NULL;
}

static Field *find_field_by_name(const char *name) {
    for (size_t i = 0; i < g_archive->fields->size; i++) {
        Field *f = vector_at(g_archive->fields, i);
        if (strcmp(f->name, name) == 0) return f;
    }
    return NULL;
}

/* names are shared across one search tree in the library; a duplicate
 * silently breaks searching for one of them, and entry_create() will flat
 * out terminate the whole process on a duplicate name. Check first. */
static int name_taken(const char *name) {
    return find_group_by_name(name) || find_entry_by_name(name) || find_field_by_name(name);
}

/* -------------------------------------------------------------------- */
/*  Selection list management                                           */
/* -------------------------------------------------------------------- */

static void reset_last_list(void) {
    if (g_last_list) {
        for (size_t i = 0; i < g_last_list->size; i++) free(vector_at(g_last_list, i));
        vector_destroy(g_last_list);
    }
    g_last_list = vector_create();
}

static void push_list_item(EntityType type, void *entity) {
    ListItem *it = malloc(sizeof(ListItem));
    it->type = type;
    it->entity = entity;
    vector_push_back(g_last_list, it);
}

/* -------------------------------------------------------------------- */
/*  Printing entities                                                   */
/* -------------------------------------------------------------------- */

static void print_group_line(int idx, Group *g) {
    printf("  [%d] GROUP  \"%s\"  (id=%u, entries=%zu)\n",
           idx, g->name, g->group_id, g->entries->size);
}

static void print_entry_line(int idx, Entry *e) {
    printf("  [%d] ENTRY  \"%s\"  (id=%u, group=\"%s\", fields=%zu)\n",
           idx, e->name, e->entry_id, e->group->name, e->fields->size);
}

static void print_field_line(int idx, Field *f) {
    printf("  [%d] FIELD  \"%s\"  (type=%s, entry=\"%s\", size=%llu bytes)\n",
           idx, f->name, field_type_str(f->type), f->entry->name,
           (unsigned long long) f->size);
}

/* -------------------------------------------------------------------- */
/*  Prompt                                                              */
/* -------------------------------------------------------------------- */

static void print_prompt(void) {
    switch (g_ctx) {
        case CTX_ARCHIVE:
            printf("%s >", g_archive->name);
            break;
        case CTX_GROUP:
            printf("GROUP %s>", ((Group *) g_current)->name);
            break;
        case CTX_ENTRY:
            printf("ENTRY %s>", ((Entry *) g_current)->name);
            break;
        case CTX_FIELD:
            printf("FIELD %s>", ((Field *) g_current)->name);
            break;
    }
    printf(" ");
    fflush(stdout);
}

/* -------------------------------------------------------------------- */
/*  Help                                                                */
/* -------------------------------------------------------------------- */

static void help_archive(void) {
    printf(
        "Available commands:\n"
        "  help                                   show this help\n"
        "  info                                   show archive info\n"
        "  list groups                            list all groups\n"
        "  list entries [group-name]               list entries (optionally by group)\n"
        "  list fields [entry-name]                list fields (optionally by entry)\n"
        "  search <text>                          search names & text-field content\n"
        "  select <index>                         select an item from the last list/search\n"
        "  add group <name>                       create a group\n"
        "  add entry <group-name> <name>          create an entry inside a group\n"
        "  add field <entry-name> <name> [type]   create a field (type: text|password, default text)\n"
        "  save                                   write pending changes to the file\n"
        "  exit / quit                            leave archive-cli\n"
    );
}

static void help_group(void) {
    printf(
        "Available commands (GROUP context):\n"
        "  help                    show this help\n"
        "  info                    show group info\n"
        "  list                    list entries in this group\n"
        "  select <index>          select an entry from the last list\n"
        "  set-name <name>         rename this group\n"
        "  add entry <name>        create an entry inside this group\n"
        "  delete                  delete this group (and all its entries/fields)\n"
        "  back                    return to the archive prompt\n"
        "  exit / quit             leave archive-cli\n"
    );
}

static void help_entry(void) {
    printf(
        "Available commands (ENTRY context):\n"
        "  help                       show this help\n"
        "  info                       show entry info\n"
        "  list                       list fields in this entry\n"
        "  select <index>             select a field from the last list\n"
        "  set-name <name>            rename this entry\n"
        "  set-group <group-name>     move this entry to another group\n"
        "  add field <name> [type]    create a field (type: text|password, default text)\n"
        "  delete                     delete this entry (and all its fields)\n"
        "  back                       return to the archive prompt\n"
        "  exit / quit                leave archive-cli\n"
    );
}

static void help_field(void) {
    printf(
        "Available commands (FIELD context):\n"
        "  help                       show this help\n"
        "  info                       show field info\n"
        "  show                       print field content\n"
        "  set-name <name>            rename this field\n"
        "  set-content <text>         set the field's content\n"
        "  set-type <text|password>   change the field's type (keeps content)\n"
        "  set-entry <entry-name>     move this field to another entry\n"
        "  delete                     delete this field\n"
        "  back                       return to the archive prompt\n"
        "  exit / quit                leave archive-cli\n"
    );
}

/* -------------------------------------------------------------------- */
/*  "list" / "search"                                                   */
/* -------------------------------------------------------------------- */

static void do_list_groups(void) {
    reset_last_list();
    if (g_archive->groups->size == 0) {
        printf("(no groups)\n");
        return;
    }
    for (size_t i = 0; i < g_archive->groups->size; i++) {
        Group *g = vector_at(g_archive->groups, i);
        push_list_item(GROUP, g);
        print_group_line((int) i + 1, g);
    }
}

static void do_list_entries(const char *group_filter) {
    reset_last_list();
    int shown = 0;
    for (size_t i = 0; i < g_archive->entries->size; i++) {
        Entry *e = vector_at(g_archive->entries, i);
        if (group_filter && strcmp(e->group->name, group_filter) != 0) continue;
        push_list_item(ENTRY, e);
        print_entry_line(++shown, e);
    }
    if (!shown) printf("(no matching entries)\n");
}

static void do_list_fields(const char *entry_filter) {
    reset_last_list();
    int shown = 0;
    for (size_t i = 0; i < g_archive->fields->size; i++) {
        Field *f = vector_at(g_archive->fields, i);
        if (entry_filter && strcmp(f->entry->name, entry_filter) != 0) continue;
        push_list_item(FIELD, f);
        print_field_line(++shown, f);
    }
    if (!shown) printf("(no matching fields)\n");
}

static void do_list_group_entries(Group *g) {
    reset_last_list();
    if (g->entries->size == 0) {
        printf("(this group has no entries)\n");
        return;
    }
    for (size_t i = 0; i < g->entries->size; i++) {
        Entry *e = vector_at(g->entries, i);
        push_list_item(ENTRY, e);
        print_entry_line((int) i + 1, e);
    }
}

static void do_list_entry_fields(Entry *e) {
    reset_last_list();
    if (e->fields->size == 0) {
        printf("(this entry has no fields)\n");
        return;
    }
    for (size_t i = 0; i < e->fields->size; i++) {
        Field *f = vector_at(e->fields, i);
        push_list_item(FIELD, f);
        print_field_line((int) i + 1, f);
    }
}

static void do_search(const char *text) {
    if (!text || !*text) {
        printf("usage: search <text>\n");
        return;
    }
    reset_last_list();
    int shown = 0;

    for (size_t i = 0; i < g_archive->groups->size; i++) {
        Group *g = vector_at(g_archive->groups, i);
        if (ci_contains(g->name, text)) {
            push_list_item(GROUP, g);
            print_group_line(++shown, g);
        }
    }
    for (size_t i = 0; i < g_archive->entries->size; i++) {
        Entry *e = vector_at(g_archive->entries, i);
        if (ci_contains(e->name, text)) {
            push_list_item(ENTRY, e);
            print_entry_line(++shown, e);
        }
    }
    for (size_t i = 0; i < g_archive->fields->size; i++) {
        Field *f = vector_at(g_archive->fields, i);
        int name_hit = ci_contains(f->name, text);
        int content_hit = 0;
        if ((f->type == TEXT || f->type == PASSWORD) && f->content) {
            content_hit = ci_contains((char *) f->content, text);
        }
        if (name_hit || content_hit) {
            push_list_item(FIELD, f);
            print_field_line(++shown, f);
        }
    }

    if (!shown) printf("(no matches for \"%s\")\n", text);
}

static void do_select(const char *idx_str) {
    if (!idx_str || !*idx_str) {
        printf("usage: select <index>\n");
        return;
    }
    if (!g_last_list || g_last_list->size == 0) {
        printf("nothing to select from. run a \"list\" or \"search\" command first.\n");
        return;
    }
    char *end;
    long idx = strtol(idx_str, &end, 10);
    if (*end != '\0' || idx < 1 || (size_t) idx > g_last_list->size) {
        printf("invalid index. valid range: 1-%zu\n", g_last_list->size);
        return;
    }
    ListItem *it = vector_at(g_last_list, (size_t) (idx - 1));
    switch (it->type) {
        case GROUP: g_ctx = CTX_GROUP; g_current = it->entity; break;
        case ENTRY: g_ctx = CTX_ENTRY; g_current = it->entity; break;
        case FIELD: g_ctx = CTX_FIELD; g_current = it->entity; break;
    }
    reset_last_list();
}

/* -------------------------------------------------------------------- */
/*  Command handlers: ARCHIVE context                                   */
/* -------------------------------------------------------------------- */

static int handle_archive_cmd(char *cmd, char *rest) {
    if (str_ieq(cmd, "help") || str_ieq(cmd, "?")) {
        help_archive();
    } else if (str_ieq(cmd, "info")) {
        char *s = archive_to_string(g_archive);
        printf("%s\n", s);
        free(s);
    } else if (str_ieq(cmd, "list")) {
        char *sub = next_token(&rest);
        if (!sub) {
            printf("usage: list groups | list entries [group-name] | list fields [entry-name]\n");
        } else if (str_ieq(sub, "groups")) {
            do_list_groups();
        } else if (str_ieq(sub, "entries")) {
            char *filter = next_token(&rest);
            do_list_entries(filter);
        } else if (str_ieq(sub, "fields")) {
            char *filter = next_token(&rest);
            do_list_fields(filter);
        } else {
            printf("unknown list target \"%s\". try: groups | entries | fields\n", sub);
        }
    } else if (str_ieq(cmd, "search")) {
        do_search(trim(rest));
    } else if (str_ieq(cmd, "select")) {
        do_select(next_token(&rest));
    } else if (str_ieq(cmd, "add")) {
        char *type = next_token(&rest);
        if (!type) {
            printf("usage: add group|entry|field ...\n");
        } else if (str_ieq(type, "group")) {
            char *name = next_token(&rest);
            if (!name) {
                printf("usage: add group <name>\n");
            } else if (name_taken(name)) {
                printf("error: the name \"%s\" is already in use.\n", name);
            } else {
                Group *g = group_create(g_archive, strdup(name));
                printf("created group \"%s\" (id=%u).\n", g->name, g->group_id);
            }
        } else if (str_ieq(type, "entry")) {
            char *group_name = next_token(&rest);
            char *name = next_token(&rest);
            if (!group_name || !name) {
                printf("usage: add entry <group-name> <name>\n");
            } else {
                Group *g = find_group_by_name(group_name);
                if (!g) {
                    printf("error: no group named \"%s\".\n", group_name);
                } else if (name_taken(name)) {
                    printf("error: the name \"%s\" is already in use.\n", name);
                } else {
                    Entry *e = entry_create(g, strdup(name));
                    printf("created entry \"%s\" (id=%u) in group \"%s\".\n", e->name, e->entry_id, g->name);
                }
            }
        } else if (str_ieq(type, "field")) {
            char *entry_name = next_token(&rest);
            char *name = next_token(&rest);
            char *type_str = next_token(&rest);
            if (!entry_name || !name) {
                printf("usage: add field <entry-name> <name> [text|password]\n");
            } else {
                Entry *e = find_entry_by_name(entry_name);
                FieldType ftype = TEXT;
                if (type_str && !parse_field_type(type_str, &ftype)) {
                    printf("error: unknown field type \"%s\". use \"text\" or \"password\".\n", type_str);
                    return 1;
                }
                if (!e) {
                    printf("error: no entry named \"%s\".\n", entry_name);
                } else if (name_taken(name)) {
                    printf("error: the name \"%s\" is already in use.\n", name);
                } else {
                    Field *f = field_create(e, ftype, strdup(name));
                    /* the library leaves content NULL after field_create(),
                     * and write_field() will crash on a NULL TEXT/PASSWORD
                     * content. Default to empty content until the user sets
                     * something real with "set-content". */
                    field_set_content(f, ftype, strdup(""));
                    printf("created field \"%s\" (type=%s) in entry \"%s\".\n", f->name, field_type_str(f->type), e->name);
                }
            }
        } else {
            printf("unknown entity type \"%s\". try: group | entry | field\n", type);
        }
    } else if (str_ieq(cmd, "save")) {
        if (g_archive->written) {
            printf("nothing to save, archive is already up to date.\n");
        } else if (write_archive(g_archive) == SUCCESS) {
            printf("archive saved. size=%llu bytes, changes=%u.\n",
                   (unsigned long long) g_archive->size, g_archive->num_of_changes);
        } else {
            printf("error: failed to save archive.\n");
        }
    } else if (str_ieq(cmd, "exit") || str_ieq(cmd, "quit")) {
        return 0; /* signal caller to stop */
    } else {
        printf("unknown command \"%s\". type \"help\" for a list of commands.\n", cmd);
    }
    return 1;
}

/* -------------------------------------------------------------------- */
/*  Command handlers: GROUP context                                     */
/* -------------------------------------------------------------------- */

static int handle_group_cmd(char *cmd, char *rest) {
    Group *g = (Group *) g_current;

    if (str_ieq(cmd, "help") || str_ieq(cmd, "?")) {
        help_group();
    } else if (str_ieq(cmd, "info")) {
        char *s = group_to_string(g);
        printf("%s\n", s);
        free(s);
    } else if (str_ieq(cmd, "list")) {
        do_list_group_entries(g);
    } else if (str_ieq(cmd, "select")) {
        do_select(next_token(&rest));
    } else if (str_ieq(cmd, "set-name")) {
        char *name = trim(rest);
        if (!*name) {
            printf("usage: set-name <name>\n");
        } else if (name_taken(name)) {
            printf("error: the name \"%s\" is already in use.\n", name);
        } else {
            group_set_name(g, strdup(name));
            printf("group renamed to \"%s\".\n", g->name);
        }
    } else if (str_ieq(cmd, "add")) {
        char *type = next_token(&rest);
        if (!type || !str_ieq(type, "entry")) {
            printf("usage: add entry <name>\n");
        } else {
            char *name = next_token(&rest);
            if (!name) {
                printf("usage: add entry <name>\n");
            } else if (name_taken(name)) {
                printf("error: the name \"%s\" is already in use.\n", name);
            } else {
                Entry *e = entry_create(g, strdup(name));
                printf("created entry \"%s\" (id=%u) in group \"%s\".\n", e->name, e->entry_id, g->name);
            }
        }
    } else if (str_ieq(cmd, "delete")) {
        char msg[256];
        snprintf(msg, sizeof(msg), "delete group \"%s\" and all its entries/fields?", g->name);
        if (confirm(msg)) {
            group_delete(g);
            printf("group deleted.\n");
            g_ctx = CTX_ARCHIVE;
            g_current = NULL;
            reset_last_list();
        } else {
            printf("cancelled.\n");
        }
    } else if (str_ieq(cmd, "back")) {
        g_ctx = CTX_ARCHIVE;
        g_current = NULL;
        reset_last_list();
    } else if (str_ieq(cmd, "exit") || str_ieq(cmd, "quit")) {
        return 0;
    } else {
        printf("unknown command \"%s\". type \"help\" for a list of commands.\n", cmd);
    }
    return 1;
}

/* -------------------------------------------------------------------- */
/*  Command handlers: ENTRY context                                     */
/* -------------------------------------------------------------------- */

static int handle_entry_cmd(char *cmd, char *rest) {
    Entry *e = (Entry *) g_current;

    if (str_ieq(cmd, "help") || str_ieq(cmd, "?")) {
        help_entry();
    } else if (str_ieq(cmd, "info")) {
        char *s = entry_to_string(e);
        printf("%s\n", s);
        free(s);
    } else if (str_ieq(cmd, "list")) {
        do_list_entry_fields(e);
    } else if (str_ieq(cmd, "select")) {
        do_select(next_token(&rest));
    } else if (str_ieq(cmd, "set-name")) {
        char *name = trim(rest);
        if (!*name) {
            printf("usage: set-name <name>\n");
        } else if (name_taken(name)) {
            printf("error: the name \"%s\" is already in use.\n", name);
        } else {
            entry_set_name(e, strdup(name));
            printf("entry renamed to \"%s\".\n", e->name);
        }
    } else if (str_ieq(cmd, "set-group")) {
        char *group_name = trim(rest);
        if (!*group_name) {
            printf("usage: set-group <group-name>\n");
        } else {
            Group *ng = find_group_by_name(group_name);
            if (!ng) {
                printf("error: no group named \"%s\".\n", group_name);
            } else {
                entry_set_group(e, ng);
                printf("entry moved to group \"%s\".\n", ng->name);
            }
        }
    } else if (str_ieq(cmd, "add")) {
        char *type = next_token(&rest);
        if (!type || !str_ieq(type, "field")) {
            printf("usage: add field <name> [text|password]\n");
        } else {
            char *name = next_token(&rest);
            char *type_str = next_token(&rest);
            FieldType ftype = TEXT;
            if (!name) {
                printf("usage: add field <name> [text|password]\n");
            } else if (type_str && !parse_field_type(type_str, &ftype)) {
                printf("error: unknown field type \"%s\". use \"text\" or \"password\".\n", type_str);
            } else if (name_taken(name)) {
                printf("error: the name \"%s\" is already in use.\n", name);
            } else {
                Field *f = field_create(e, ftype, strdup(name));
                field_set_content(f, ftype, strdup(""));
                printf("created field \"%s\" (type=%s) in entry \"%s\".\n", f->name, field_type_str(f->type), e->name);
            }
        }
    } else if (str_ieq(cmd, "delete")) {
        char msg[256];
        snprintf(msg, sizeof(msg), "delete entry \"%s\" and all its fields?", e->name);
        if (confirm(msg)) {
            entry_delete(e);
            printf("entry deleted.\n");
            g_ctx = CTX_ARCHIVE;
            g_current = NULL;
            reset_last_list();
        } else {
            printf("cancelled.\n");
        }
    } else if (str_ieq(cmd, "back")) {
        g_ctx = CTX_ARCHIVE;
        g_current = NULL;
        reset_last_list();
    } else if (str_ieq(cmd, "exit") || str_ieq(cmd, "quit")) {
        return 0;
    } else {
        printf("unknown command \"%s\". type \"help\" for a list of commands.\n", cmd);
    }
    return 1;
}

/* -------------------------------------------------------------------- */
/*  Command handlers: FIELD context                                     */
/* -------------------------------------------------------------------- */

static int handle_field_cmd(char *cmd, char *rest) {
    Field *f = (Field *) g_current;

    if (str_ieq(cmd, "help") || str_ieq(cmd, "?")) {
        help_field();
    } else if (str_ieq(cmd, "info")) {
        char *s = field_to_string(f);
        printf("%s\n", s);
        free(s);
    } else if (str_ieq(cmd, "show")) {
        if (f->type == BINARY) {
            printf("(binary field content is not supported in this build)\n");
        } else if (!f->content) {
            printf("(empty)\n");
        } else {
            printf("%s\n", (char *) f->content);
        }
    } else if (str_ieq(cmd, "set-name")) {
        char *name = trim(rest);
        if (!*name) {
            printf("usage: set-name <name>\n");
        } else if (name_taken(name)) {
            printf("error: the name \"%s\" is already in use.\n", name);
        } else {
            field_set_name(f, strdup(name));
            printf("field renamed to \"%s\".\n", f->name);
        }
    } else if (str_ieq(cmd, "set-content")) {
        char *content = trim(rest);
        if (f->type == BINARY) {
            printf("error: binary field content is not supported in this build.\n");
        } else {
            field_set_content(f, f->type, strdup(content));
            printf("field content updated (%zu bytes).\n", strlen(content));
        }
    } else if (str_ieq(cmd, "set-type")) {
        char *type_str = next_token(&rest);
        FieldType ftype;
        if (!type_str || !parse_field_type(type_str, &ftype)) {
            printf("usage: set-type <text|password>\n");
        } else {
            /* a field loaded from disk with zero size never had its content
             * loaded (still NULL); writing NULL content would crash on
             * save, so fall back to empty content in that case. */
            void *content = f->content ? f->content : strdup("");
            field_set_content(f, ftype, content);
            printf("field type set to \"%s\".\n", field_type_str(f->type));
        }
    } else if (str_ieq(cmd, "set-entry")) {
        char *entry_name = trim(rest);
        if (!*entry_name) {
            printf("usage: set-entry <entry-name>\n");
        } else {
            Entry *ne = find_entry_by_name(entry_name);
            if (!ne) {
                printf("error: no entry named \"%s\".\n", entry_name);
            } else {
                field_set_entry(f, ne);
                printf("field moved to entry \"%s\".\n", ne->name);
            }
        }
    } else if (str_ieq(cmd, "delete")) {
        char msg[256];
        snprintf(msg, sizeof(msg), "delete field \"%s\"?", f->name);
        if (confirm(msg)) {
            field_delete(f);
            printf("field deleted.\n");
            g_ctx = CTX_ARCHIVE;
            g_current = NULL;
            reset_last_list();
        } else {
            printf("cancelled.\n");
        }
    } else if (str_ieq(cmd, "back")) {
        g_ctx = CTX_ARCHIVE;
        g_current = NULL;
        reset_last_list();
    } else if (str_ieq(cmd, "exit") || str_ieq(cmd, "quit")) {
        return 0;
    } else {
        printf("unknown command \"%s\". type \"help\" for a list of commands.\n", cmd);
    }
    return 1;
}

/* -------------------------------------------------------------------- */
/*  REPL                                                                 */
/* -------------------------------------------------------------------- */

static void repl(void) {
    char line[LINE_MAX_LEN];

    for (;;) {
        print_prompt();
        if (!fgets(line, sizeof(line), stdin)) {
            printf("\n");
            break;
        }
        char *trimmed = trim(line);
        if (!*trimmed) continue;

        char *cursor = trimmed;
        char *cmd = next_token(&cursor);
        if (!cmd) continue;

        int keep_going;
        switch (g_ctx) {
            case CTX_ARCHIVE: keep_going = handle_archive_cmd(cmd, cursor); break;
            case CTX_GROUP:   keep_going = handle_group_cmd(cmd, cursor);   break;
            case CTX_ENTRY:   keep_going = handle_entry_cmd(cmd, cursor);   break;
            case CTX_FIELD:   keep_going = handle_field_cmd(cmd, cursor);   break;
            default:          keep_going = 0; break;
        }

        if (!keep_going) {
            if (!g_archive->written) {
                if (confirm("you have unsaved changes. save before exiting?")) {
                    if (write_archive(g_archive) == SUCCESS) {
                        printf("archive saved.\n");
                    } else {
                        printf("error: failed to save archive.\n");
                    }
                }
            }
            break;
        }
    }
}

/* -------------------------------------------------------------------- */
/*  Magic number check & startup                                        */
/* -------------------------------------------------------------------- */

static int check_magic(FILE *fp, const char *filename) {
    unsigned char buf[MAGIC_SIZE];
    size_t n = fread(buf, 1, MAGIC_SIZE, fp);
    rewind(fp);
    if (n < MAGIC_SIZE || buf[0] != 'M' || buf[1] != 'M' || buf[2] != '3' || buf[3] != '3') {
        fprintf(stderr,
                "error: \"%s\" is not a valid Cryptos archive (magic number mismatch, expected \"MM33\").\n",
                filename);
        return 0;
    }
    return 1;
}

/* strips directory components and a trailing extension, used only to name
 * a brand new archive after the file the user passed on the command line. */
static char *derive_default_name(const char *path) {
    const char *base = strrchr(path, '/');
    base = base ? base + 1 : path;
    char *name = strdup(base);
    char *dot = strrchr(name, '.');
    if (dot && dot != name) *dot = '\0';
    return name;
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <file-name>\n", argv[0]);
        return 1;
    }

    const char *filename = argv[1];
    FILE *fp = fopen(filename, "rb+");

    if (!fp) {
        if (errno != ENOENT) {
            perror("fopen");
            return 1;
        }

        printf("File \"%s\" does not exist.\n", filename);
        if (!confirm("Create a new, empty archive with this name?")) {
            printf("aborted.\n");
            return 1;
        }

        fp = fopen(filename, "wb+");
        if (!fp) {
            perror("fopen");
            return 1;
        }

        char *default_name = derive_default_name(filename);
        g_archive = archive_create(default_name, strdup(""), fp);
        if (write_archive(g_archive) != SUCCESS) {
            fprintf(stderr, "error: could not initialize new archive file.\n");
            archive_destroy(g_archive);
            fclose(fp);
            return 1;
        }
        printf("created new archive \"%s\".\n", g_archive->name);

    } else {
        if (!check_magic(fp, filename)) {
            fclose(fp);
            return 1;
        }

        g_archive = archive_create(strdup(""), strdup(""), fp);
        if (read_archive(g_archive, fp) != SUCCESS) {
            fprintf(stderr, "error: failed to read archive \"%s\".\n", filename);
            archive_destroy(g_archive);
            fclose(fp);
            return 1;
        }
        printf("archive \"%s\" loaded. (%zu groups, %zu entries, %zu fields, %u change(s))\n",
               g_archive->name, g_archive->groups->size, g_archive->entries->size,
               g_archive->fields->size, g_archive->num_of_changes);
    }

    printf("type \"help\" for a list of commands.\n");

    reset_last_list();
    repl();

    if (g_last_list) {
        for (size_t i = 0; i < g_last_list->size; i++) free(vector_at(g_last_list, i));
        vector_destroy(g_last_list);
    }
    archive_destroy(g_archive);
    fclose(fp);

    return 0;
}
