/*
 * pbwire - a dependency-free Protobuf runtime in C with dynamic schema support.
 *
 * Schemas are built at runtime (see pbw_schema_*); messages are dynamic value
 * trees (see pbw_message_*); wire encoding/decoding follows the standard
 * Protobuf wire format (see pbw_encode / pbw_decode).
 *
 * Conventions:
 *   - Every pointer returned by this library is owned by the library unless
 * documented otherwise; free with pbw_free() / pbw_message_free() /
 * pbw_value_free().
 *   - Numeric values are stored as raw bit patterns: PBW_TYPE_FLOAT and
 *     PBW_TYPE_FIXED32/PBW_TYPE_SFIXED32 live in the low 32 bits,
 * PBW_TYPE_DOUBLE and the 64-bit fixed types use all 64 bits. Signed types are
 * sign extended.
 *   - Byte payloads are always NUL terminated so pbw_value_get_string() is safe
 * on decoded string fields (the extra byte is not part of the field length).
 *   - No defaults are filled in: a field that was not present on the wire is
 * simply absent (pbw_message_get() returns NULL). Explicit presence per field.
 */
#ifndef PBWIRE_H
#define PBWIRE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    PBW_OK = 0,
    PBW_ERR_INVALID = 1,   /* malformed wire data or invalid argument */
    PBW_ERR_NOMEM = 2,     /* allocation failure */
    PBW_ERR_NOT_FOUND = 3, /* unknown field number / name */
    PBW_ERR_TYPE = 4,      /* value does not match the field type */
    PBW_ERR_LIMIT = 5      /* nesting or size limit exceeded */
} pbw_status;

/* Values match protobuf FieldDescriptorProto.Type numbering. */
typedef enum {
    PBW_TYPE_DOUBLE = 1,
    PBW_TYPE_FLOAT = 2,
    PBW_TYPE_INT64 = 3,
    PBW_TYPE_UINT64 = 4,
    PBW_TYPE_INT32 = 5,
    PBW_TYPE_FIXED64 = 6,
    PBW_TYPE_FIXED32 = 7,
    PBW_TYPE_BOOL = 8,
    PBW_TYPE_STRING = 9,
    PBW_TYPE_GROUP = 10,
    PBW_TYPE_MESSAGE = 11,
    PBW_TYPE_BYTES = 12,
    PBW_TYPE_UINT32 = 13,
    PBW_TYPE_ENUM = 14,
    PBW_TYPE_SFIXED32 = 15,
    PBW_TYPE_SFIXED64 = 16,
    PBW_TYPE_SINT32 = 17,
    PBW_TYPE_SINT64 = 18
} pbw_type;

/* Values match protobuf FieldDescriptorProto.Label numbering. */
typedef enum {
    PBW_LABEL_OPTIONAL = 1,
    PBW_LABEL_REQUIRED = 2,
    PBW_LABEL_REPEATED = 3
} pbw_label;

typedef enum {
    PBW_VALUE_UINT = 0,  /* any numeric type, raw bit pattern */
    PBW_VALUE_BYTES = 1, /* string or bytes */
    PBW_VALUE_MESSAGE = 2,
    PBW_VALUE_LIST = 3 /* repeated field storage */
} pbw_value_kind;

typedef struct pbw_schema pbw_schema;
typedef struct pbw_ctx pbw_ctx;
typedef struct pbw_descriptor pbw_descriptor;
typedef struct pbw_field pbw_field;
typedef struct pbw_value pbw_value;
typedef struct pbw_message pbw_message;

/* ---------------------------------------------------------------- schema */

pbw_schema *pbw_schema_new(void);
void pbw_schema_free(pbw_schema *schema);

/* Create a message or enum descriptor. Names must be unique within a schema. */
pbw_descriptor *pbw_schema_add_message(pbw_schema *schema, const char *name);
pbw_descriptor *pbw_schema_add_enum(pbw_schema *schema, const char *name);
pbw_descriptor *pbw_schema_find(const pbw_schema *schema, const char *name);

/* Append an enum value. Enum descriptors are only usable after
 * pbw_schema_link(). */
void pbw_enum_add_value(pbw_descriptor *enum_desc, const char *name,
                        int32_t number);
int pbw_enum_find_name(const pbw_descriptor *enum_desc, int32_t number,
                       const char **name);
int pbw_enum_find_value(const pbw_descriptor *enum_desc, const char *name,
                        int32_t *number);

/* Append a field. Field numbers must be unique within the message. */
pbw_field *pbw_message_add_field(pbw_descriptor *msg, const char *name,
                                 uint32_t number, pbw_type type,
                                 pbw_label label);

/* Point a MESSAGE/ENUM field at another descriptor by name; resolved by
 * pbw_schema_link(). */
void pbw_field_set_type_name(pbw_field *field, const char *type_name);
/* Emit repeated numeric fields packed (proto3 default). */
void pbw_field_set_packed(pbw_field *field, int packed);
/* 1-based oneof group index; 0 means the field is not in a oneof. */
void pbw_field_set_oneof(pbw_field *field, uint32_t oneof_index);

/* Resolve type names, sort fields by number and build lookup tables.
 * Idempotent. */
pbw_status pbw_schema_link(pbw_schema *schema);
/* Last link failure reason (empty when the last link succeeded). */
const char *pbw_schema_error(const pbw_schema *schema);

/* Load a standard FileDescriptorSet (protoc --descriptor_set_out, buf, ...)
 * into a new schema, so a schema can come from a .proto without generated code.
 * Every referenced type must be present in the set (use --include_imports).
 * Descriptors are named fully qualified without the leading dot:
 * "pkg.Outer.Inner". Field type names resolve the same way.
 * *out is always written; on failure it holds the partially built schema so
 * pbw_schema_error() explains why, and the caller frees it as usual. */
pbw_status pbw_descriptor_set_load(const void *data, size_t len,
                                   pbw_schema **out);

const char *pbw_descriptor_name(const pbw_descriptor *desc);
int pbw_descriptor_is_enum(const pbw_descriptor *desc);
size_t pbw_descriptor_field_count(const pbw_descriptor *desc);
const pbw_field *pbw_descriptor_field_at(const pbw_descriptor *desc,
                                         size_t index);
const pbw_field *pbw_descriptor_find_field(const pbw_descriptor *desc,
                                           uint32_t number);
/* Position of a field in the linked descriptor, or (size_t)-1. */
size_t pbw_descriptor_field_index(const pbw_descriptor *desc, uint32_t number);
const pbw_field *pbw_descriptor_find_field_name(const pbw_descriptor *desc,
                                                const char *name);

const char *pbw_field_name(const pbw_field *field);
uint32_t pbw_field_number(const pbw_field *field);
pbw_type pbw_field_type(const pbw_field *field);
pbw_label pbw_field_label(const pbw_field *field);
int pbw_field_is_packed(const pbw_field *field);
uint32_t pbw_field_oneof_index(const pbw_field *field);
const pbw_descriptor *pbw_field_message_type(const pbw_field *field);

/* --------------------------------------------------------------- message */

/* A message keeps pointing at its descriptor: the schema must outlive it. */
pbw_message *pbw_message_new(const pbw_descriptor *desc);
void pbw_message_free(pbw_message *msg);
const pbw_descriptor *pbw_message_descriptor(const pbw_message *msg);

/* Present value of a field, or NULL. Repeated fields expose a PBW_VALUE_LIST.
 */
pbw_value *pbw_message_get(const pbw_message *msg, uint32_t number);
/* Same as pbw_message_get(), but with a field handle obtained while building
 * the schema. Skips the field-number lookup; the field must belong to msg. */
pbw_value *pbw_message_get_field(const pbw_message *msg, const pbw_field *field);
/* Index-based variant: the index is the field's position in the linked
 * descriptor (stable for the life of the schema). */
pbw_value *pbw_message_get_index(const pbw_message *msg, size_t index);
/* Number of fields with a value. */
size_t pbw_message_field_count(const pbw_message *msg);
/* Reflection over present fields; returns NULL past the end. */
const pbw_field *pbw_message_field_at(const pbw_message *msg, size_t index,
                                      pbw_value **value);
/* Drop a field value (and, for a oneof, the group case). */
void pbw_message_clear(pbw_message *msg, uint32_t number);

/* Store a value: appends for repeated fields, replaces otherwise. Takes
 * ownership of value on success only. */
pbw_status pbw_message_set(pbw_message *msg, uint32_t number, pbw_value *value);
/* Same as pbw_message_set(), but with a field handle (see
 * pbw_message_get_field). */
pbw_status pbw_message_set_field(pbw_message *msg, const pbw_field *field,
                                 pbw_value *value);
pbw_status pbw_message_set_index(pbw_message *msg, size_t index,
                                 pbw_value *value);
/* Create (or return the existing) nested message value for a MESSAGE field. */
pbw_message *pbw_message_set_message(pbw_message *msg, uint32_t number);

/* ----------------------------------------------------------------- value */

pbw_value *pbw_value_new_uint(uint64_t bits);
pbw_value *pbw_value_new_int(int64_t value);
pbw_value *pbw_value_new_bool(int value);
pbw_value *pbw_value_new_float(float value);
pbw_value *pbw_value_new_double(double value);
pbw_value *pbw_value_new_bytes(const void *data, size_t len);
pbw_value *pbw_value_new_string(const char *str);
pbw_value *pbw_value_new_message(pbw_message *msg); /* takes ownership of msg */
void pbw_value_free(pbw_value *value);

pbw_value_kind pbw_value_get_kind(const pbw_value *value);
uint64_t pbw_value_get_uint(const pbw_value *value);
int64_t pbw_value_get_int(const pbw_value *value);
int32_t pbw_value_get_int32(const pbw_value *value);
uint32_t pbw_value_get_uint32(const pbw_value *value);
int pbw_value_get_bool(const pbw_value *value);
float pbw_value_get_float(const pbw_value *value);
double pbw_value_get_double(const pbw_value *value);
const void *pbw_value_get_bytes(const pbw_value *value, size_t *len);
const char *pbw_value_get_string(const pbw_value *value);
pbw_message *pbw_value_get_message(const pbw_value *value);
size_t pbw_value_count(const pbw_value *value);
pbw_value *pbw_value_at(const pbw_value *value, size_t index);

/* -------------------------------------------------------------- workspace */

/*
 * A context owns a bump allocator and a reusable encode buffer, so a program
 * that decodes or encodes in a loop stops calling malloc after the first few
 * messages. One context per thread/goroutine; never share one concurrently.
 *
 * Messages decoded with a context live in its allocator:
 *   - pbw_message_free() on them is a no-op (they die with pbw_ctx_reset)
 *   - they are read-only: pbw_message_set/set_message fail, clear does nothing
 *   - pbw_ctx_reset() invalidates every message decoded with it so far
 * Use the plain pbw_decode/pbw_encode when ownership is per-message.
 */
pbw_ctx *pbw_ctx_new(void);
void pbw_ctx_free(pbw_ctx *ctx);
/* Release every allocation made since the last reset, keeping the buffers. */
void pbw_ctx_reset(pbw_ctx *ctx);
/* Bytes currently held in the context (for sizing/debugging). */
size_t pbw_ctx_used(const pbw_ctx *ctx);

/* Build a message tree in the context's build arena: allocations are bump
 * pointer moves, pbw_ctx_reset() releases the whole tree at once. These
 * messages are writable (unlike pbw_decode_ctx() results). */
pbw_message *pbw_message_new_ctx(pbw_ctx *ctx, const pbw_descriptor *desc);
pbw_value *pbw_value_new_uint_ctx(pbw_ctx *ctx, uint64_t bits);
pbw_value *pbw_value_new_int_ctx(pbw_ctx *ctx, int64_t value);
pbw_value *pbw_value_new_bool_ctx(pbw_ctx *ctx, int flag);
pbw_value *pbw_value_new_float_ctx(pbw_ctx *ctx, float number);
pbw_value *pbw_value_new_double_ctx(pbw_ctx *ctx, double number);
pbw_value *pbw_value_new_bytes_ctx(pbw_ctx *ctx, const void *data,
                                   size_t len);
pbw_value *pbw_value_new_message_ctx(pbw_ctx *ctx, pbw_message *msg);

/* Decode into the context instead of one allocation per value. */
pbw_status pbw_decode_ctx(pbw_ctx *ctx, const pbw_descriptor *desc,
                          const void *buf, size_t len, pbw_message **out);
/* Encode into the context's buffer. *out stays valid until the next
 * pbw_encode_ctx()/pbw_ctx_reset()/pbw_ctx_free() on the same context. */
pbw_status pbw_encode_ctx(pbw_ctx *ctx, const pbw_message *msg,
                          const uint8_t **out, size_t *len);

/* ------------------------------------------------------------------ wire */

/* Encode msg; *out is replaced with a malloc'd buffer owned by the caller
 * (pbw_free). */
pbw_status pbw_encode(const pbw_message *msg, uint8_t **out, size_t *len);
/* Decode buf into a new message of the given type. Unknown fields are skipped.
 */
pbw_status pbw_decode(const pbw_descriptor *desc, const void *buf, size_t len,
                      pbw_message **out);
void pbw_free(void *ptr);
const char *pbw_strerror(pbw_status status);

#ifdef __cplusplus
}
#endif

#endif /* PBWIRE_H */
