#ifndef PBWIRE_INTERNAL_H
#define PBWIRE_INTERNAL_H

#include "pbwire.h"

#include <stdlib.h>
#include <string.h>

/* Limits mirrored from the protobuf reference implementations and from
 * enif_protobuf's codec: unbounded nesting is a stack-overflow vector. */
#define PBW_MAX_DEPTH 100
#define PBW_MAX_FIELD_NUMBER 536870911u /* 2^29 - 1 */
#define PBW_FNUM_MAP_LIMIT 512

/* Wire types. */
#define PBW_WIRE_VARINT 0
#define PBW_WIRE_64BIT 1
#define PBW_WIRE_LEN 2
#define PBW_WIRE_START_GROUP 3
#define PBW_WIRE_END_GROUP 4
#define PBW_WIRE_32BIT 5

typedef struct {
    uint8_t *data;
    size_t len;
    size_t cap;
    int oom;
} pbw_buf;

/* Bump allocator. Everything a caller decodes with a pbw_ctx lives in one of
 * these, so resetting a context is a pointer move and freeing it is a handful
 * of free() calls instead of one per field value. */
typedef struct pbw_arena_chunk {
    struct pbw_arena_chunk *next;
    size_t used;
    size_t cap;
} pbw_arena_chunk;

typedef struct {
    pbw_arena_chunk *head;
    size_t chunk_size;
    int readonly; /* 1: messages in this arena reject mutation (decoded trees) */
} pbw_arena;

struct pbw_field {
    char *name;
    char *type_name;
    pbw_schema *schema;
    uint32_t number;
    uint32_t packed;
    uint32_t oneof_index;
    pbw_type type;
    pbw_label label;
    pbw_descriptor *sub;
};

struct pbw_enum_value {
    char *name;
    int32_t number;
};

struct pbw_descriptor {
    char *name;
    int is_enum;
    pbw_schema *schema;
    size_t field_count;
    pbw_field *fields; /* sorted by number after link */
    size_t enum_count;
    struct pbw_enum_value *enum_values;
    uint32_t oneof_count;
    uint32_t *fnum_map; /* field number -> field index + 1, NULL when sparse */
    uint32_t fnum_map_size;
};

struct pbw_schema {
    pbw_descriptor **descs;
    size_t count;
    size_t cap;
    int linked;
    char error[192];
};

struct pbw_value {
    pbw_value_kind kind;
    pbw_arena *arena; /* non-NULL: owned by a context, not freed individually */
    union {
        uint64_t u64;
        struct {
            uint8_t *data;
            size_t len;
        } bytes;
        pbw_message *msg;
        struct {
            pbw_value **items;
            size_t len;
            size_t cap;
        } list;
    } as;
};

struct pbw_message {
    const pbw_descriptor *desc;
    pbw_arena *arena;     /* non-NULL: owned by a context, not freed individually */
    pbw_value **values;   /* parallel to desc->fields */
    uint32_t *oneof_case; /* 1-based oneof index -> field number, 0 = unset */
    /* Fields the schema does not know, kept as raw wire bytes in the order they
     * were read so they can be written back unchanged. */
    uint8_t *unknown;
    size_t unknown_len;
    size_t unknown_cap;
};

/* Appends raw wire bytes to msg->unknown; PBW_OK or PBW_ERR_NOMEM. */
int pbw_message_add_unknown(pbw_message *msg, const uint8_t *data, size_t len);

/* wire.c */
static inline size_t
pbw_varint_size(uint64_t value)
{
    size_t size = 1;
    while (value >= 0x80) {
        value >>= 7;
        size++;
    }
    return size;
}
size_t pbw_varint_write(uint8_t *out, uint64_t value);
int pbw_varint_read(const uint8_t *p, const uint8_t *end, uint64_t *value,
                    size_t *used);
/* Reads a field tag. A tag is a varint32: at most 5 bytes, and the value is
 * truncated to 32 bits exactly like the reference implementations. */
int pbw_tag_read(const uint8_t *p, const uint8_t *end, uint32_t *tag,
                 size_t *used);
void pbw_buf_init(pbw_buf *buf);
void pbw_buf_free(pbw_buf *buf);
/* Grows buf so that `extra` more bytes fit; capacity survives across calls. */
int pbw_buf_reserve(pbw_buf *buf, size_t extra);
void pbw_write_le32(uint8_t *out, uint32_t value);
void pbw_write_le64(uint8_t *out, uint64_t value);
uint32_t pbw_read_le32(const uint8_t *in);
uint64_t pbw_read_le64(const uint8_t *in);

/* arena.c */
void *pbw_arena_alloc_slow(pbw_arena *arena, size_t size);

static inline void *
pbw_arena_alloc(pbw_arena *arena, size_t size)
{
    pbw_arena_chunk *chunk = arena->head;
    void *ptr;

    size = (size + 7u) & ~(size_t)7u;
    if (chunk == NULL || chunk->cap - chunk->used < size) {
        return pbw_arena_alloc_slow(arena, size);
    }
    ptr = (char *)(chunk + 1) + chunk->used;
    chunk->used += size;
    return ptr;
}
void *pbw_arena_calloc(pbw_arena *arena, size_t size);
void pbw_arena_reset(pbw_arena *arena);
void pbw_arena_free(pbw_arena *arena);
size_t pbw_arena_used(const pbw_arena *arena);

/* decode.c / encode.c */
pbw_status pbw_decode_arena(pbw_arena *arena, const pbw_descriptor *desc,
                            const void *buf, size_t len, pbw_message **out);
pbw_status pbw_encode_buf(pbw_buf *buf, const pbw_message *msg);

/* schema.c */
/* Hot path of pbw_descriptor_find_field(): inlined on purpose, it runs once per
 * field while decoding. */
static inline const pbw_field *
pbw_find_field(const pbw_descriptor *desc, uint32_t number)
{
    size_t lo;
    size_t hi;

    if (desc->fnum_map != NULL) {
        if (number < desc->fnum_map_size && desc->fnum_map[number] != 0) {
            return &desc->fields[desc->fnum_map[number] - 1];
        }
        return NULL;
    }
    lo = 0;
    hi = desc->field_count;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (desc->fields[mid].number < number) {
            lo = mid + 1;
        } else {
            hi = mid;
        }
    }
    if (lo < desc->field_count && desc->fields[lo].number == number) {
        return &desc->fields[lo];
    }
    return NULL;
}

void pbw_descriptor_free(pbw_descriptor *desc);
pbw_field *pbw_descriptor_field_mut(const pbw_descriptor *desc, size_t index);

/* message.c */
pbw_value *pbw_value_new_list_a(pbw_arena *arena);
pbw_value *pbw_value_new_uint_a(pbw_arena *arena, uint64_t bits);
pbw_value *pbw_value_new_bytes_a(pbw_arena *arena, const void *data,
                                 size_t len);
pbw_value *pbw_value_new_message_a(pbw_arena *arena, pbw_message *msg);
pbw_message *pbw_message_new_a(pbw_arena *arena, const pbw_descriptor *desc);
pbw_value *pbw_message_list_for(pbw_message *msg, const pbw_field *field);
pbw_value *pbw_value_new_list(void);
int pbw_value_list_push(pbw_value *list, pbw_value *item); /* takes ownership */
void pbw_value_clear(pbw_value *value);
int pbw_message_store(pbw_message *msg, const pbw_field *field,
                      pbw_value *value);
pbw_value *pbw_message_list_for(pbw_message *msg, const pbw_field *field);

/* encode.c / decode.c */
int pbw_is_packable(pbw_type type);
uint32_t pbw_expected_wire_type(pbw_type type);

#endif /* PBWIRE_INTERNAL_H */
