/* Wire decoder: Protobuf binary -> dynamic message tree. Unknown fields are
 * skipped. */

#include "internal.h"

typedef struct {
    const uint8_t *p;
    const uint8_t *end;
} reader;

static uint64_t
zigzag_decode(uint64_t value, unsigned width)
{
    uint64_t mask = width == 32 ? 0xffffffffull : ~0ull;
    uint64_t u = value & mask;

    return ((u >> 1) ^ (0ull - (u & 1))) & mask;
}

static pbw_status skip_group(reader *r, uint32_t number, uint32_t depth);
static pbw_status skip_field(reader *r, pbw_message *msg, uint32_t number,
                             uint32_t wire_type, uint32_t depth,
                             const uint8_t *start);

static pbw_status
skip_value(reader *r, uint32_t number, uint32_t wire_type,
           uint32_t depth)
{
    uint64_t len;
    size_t used;

    switch (wire_type) {
    case PBW_WIRE_VARINT:
        return pbw_varint_read(r->p, r->end, &len, &used) ? (r->p += used, PBW_OK)
                                                          : PBW_ERR_INVALID;
    case PBW_WIRE_64BIT:
        if ((size_t)(r->end - r->p) < 8) {
            return PBW_ERR_INVALID;
        }
        r->p += 8;
        return PBW_OK;
    case PBW_WIRE_32BIT:
        if ((size_t)(r->end - r->p) < 4) {
            return PBW_ERR_INVALID;
        }
        r->p += 4;
        return PBW_OK;
    case PBW_WIRE_LEN:
        if (!pbw_varint_read(r->p, r->end, &len, &used)) {
            return PBW_ERR_INVALID;
        }
        r->p += used;
        if (len > (uint64_t)(r->end - r->p)) {
            return PBW_ERR_INVALID;
        }
        r->p += (size_t)len;
        return PBW_OK;
    case PBW_WIRE_START_GROUP:
        return skip_group(r, number, depth);
    default:
        return PBW_ERR_INVALID;
    }
}

/* Skips a legacy group, matching its END_GROUP tag. Group skipping is
 * recursive, so nesting is bounded by PBW_MAX_DEPTH. */
static pbw_status
skip_group(reader *r, uint32_t number, uint32_t depth)
{
    if (depth >= PBW_MAX_DEPTH) {
        return PBW_ERR_LIMIT;
    }
    for (;;) {
        uint32_t tag;
        size_t used;
        uint32_t wire_type;

        if (r->p >= r->end) {
            return PBW_ERR_INVALID;
        }
        if (!pbw_tag_read(r->p, r->end, &tag, &used)) {
            return PBW_ERR_INVALID;
        }
        r->p += used;
        wire_type = tag & 7;
        /* Field number 0 is invalid, even inside a skipped group: the reference
         * parser validates every tag it walks past. */
        if ((tag >> 3) == 0) {
            return PBW_ERR_INVALID;
        }
        if (wire_type == PBW_WIRE_END_GROUP) {
            return (tag >> 3) == number ? PBW_OK : PBW_ERR_INVALID;
        }
        {
            pbw_status status = skip_value(r, tag >> 3, wire_type, depth + 1);
            if (status != PBW_OK) {
                return status;
            }
        }
    }
}

/* Skips one field and keeps its bytes. `start` points at the tag. */
static pbw_status
skip_field(reader *r, pbw_message *msg, uint32_t number, uint32_t wire_type,
           uint32_t depth, const uint8_t *start)
{
    pbw_status status = skip_value(r, number, wire_type, depth);

    if (status != PBW_OK) {
        return status;
    }
    return (pbw_status)pbw_message_add_unknown(msg, start,
                                               (size_t)(r->p - start));
}

static pbw_status
read_varint_value(reader *r, uint64_t *out)
{
    size_t used;

    if (!pbw_varint_read(r->p, r->end, out, &used)) {
        return PBW_ERR_INVALID;
    }
    r->p += used;
    return PBW_OK;
}

static pbw_status
read_fixed(reader *r, size_t size, uint64_t *out)
{
    if ((size_t)(r->end - r->p) < size) {
        return PBW_ERR_INVALID;
    }
    *out = size == 4 ? (uint64_t)pbw_read_le32(r->p) : pbw_read_le64(r->p);
    r->p += size;
    return PBW_OK;
}

static pbw_status
read_length(reader *r, const uint8_t **data, size_t *len)
{
    uint64_t value;
    size_t used;

    if (!pbw_varint_read(r->p, r->end, &value, &used)) {
        return PBW_ERR_INVALID;
    }
    r->p += used;
    if (value > (uint64_t)(r->end - r->p)) {
        return PBW_ERR_INVALID;
    }
    *data = r->p;
    *len = (size_t)value;
    r->p += (size_t)value;
    return PBW_OK;
}

/* Reads one scalar of the given type. Strings/bytes/messages are handled by the
 * caller. */
static pbw_status
read_scalar(reader *r, pbw_type type, uint64_t *out)
{
    pbw_status status;
    uint64_t raw;

    switch (type) {
    case PBW_TYPE_INT32:
    case PBW_TYPE_ENUM:
        status = read_varint_value(r, &raw);
        *out = (uint64_t)(int64_t)(int32_t)raw;
        return status;
    case PBW_TYPE_INT64:
    case PBW_TYPE_UINT64:
        return read_varint_value(r, out);
    case PBW_TYPE_UINT32:
        status = read_varint_value(r, &raw);
        *out = (uint32_t)raw;
        return status;
    case PBW_TYPE_SINT32:
        status = read_varint_value(r, &raw);
        *out = (uint64_t)(int64_t)(int32_t)zigzag_decode(raw, 32);
        return status;
    case PBW_TYPE_SINT64:
        status = read_varint_value(r, &raw);
        *out = zigzag_decode(raw, 64);
        return status;
    case PBW_TYPE_BOOL:
        status = read_varint_value(r, &raw);
        *out = raw != 0 ? 1u : 0u;
        return status;
    case PBW_TYPE_FIXED32:
    case PBW_TYPE_SFIXED32:
    case PBW_TYPE_FLOAT:
        return read_fixed(r, 4, out);
    case PBW_TYPE_FIXED64:
    case PBW_TYPE_SFIXED64:
    case PBW_TYPE_DOUBLE:
        return read_fixed(r, 8, out);
    default:
        return PBW_ERR_INVALID;
    }
}

static int
store_value(pbw_message *msg, const pbw_field *field, pbw_value *value)
{
    size_t index = (size_t)(field - msg->desc->fields);

    if (field->oneof_index == 0 && msg->values[index] == NULL) {
        msg->values[index] = value;
        return 1;
    }
    return pbw_message_store(msg, field, value);
}

static int
append_value(pbw_message *msg, const pbw_field *field,
             pbw_value *value)
{
    pbw_value *list;

    if (field->label == PBW_LABEL_REPEATED) {
        list = pbw_message_list_for(msg, field);
        if (list == NULL) {
            pbw_value_free(value);
            return 0;
        }
        if (!pbw_value_list_push(list, value)) {
            pbw_value_free(value);
            return 0;
        }
        return 1;
    }
    return store_value(msg, field, value);
}

static pbw_status decode_fields(pbw_message *msg, const uint8_t *buf,
                                size_t len, uint32_t depth);
static pbw_status decode_message(pbw_arena *arena, const pbw_descriptor *desc,
                                 const uint8_t *buf, size_t len,
                                 pbw_message **out, uint32_t depth);

/* Fast path for the most common case: a scalar field, first occurrence, not in
 * a oneof, wire type as expected. Returns 0 when the general path must run. */
static int
read_scalar_fast(reader *r, const pbw_field *field, pbw_message *msg,
                 uint32_t wire_type, pbw_status *status)
{
    size_t index;
    pbw_value *value;
    uint64_t bits;

    if (field->label == PBW_LABEL_REPEATED || field->oneof_index != 0) {
        return 0;
    }
    switch (field->type) {
    case PBW_TYPE_INT32:
    case PBW_TYPE_INT64:
    case PBW_TYPE_UINT32:
    case PBW_TYPE_UINT64:
    case PBW_TYPE_BOOL:
    case PBW_TYPE_ENUM:
    case PBW_TYPE_SINT32:
    case PBW_TYPE_SINT64:
        if (wire_type != PBW_WIRE_VARINT) {
            return 0;
        }
        break;
    case PBW_TYPE_FIXED32:
    case PBW_TYPE_SFIXED32:
    case PBW_TYPE_FLOAT:
        if (wire_type != PBW_WIRE_32BIT) {
            return 0;
        }
        break;
    case PBW_TYPE_FIXED64:
    case PBW_TYPE_SFIXED64:
    case PBW_TYPE_DOUBLE:
        if (wire_type != PBW_WIRE_64BIT) {
            return 0;
        }
        break;
    default:
        return 0;
    }
    index = (size_t)(field - msg->desc->fields);
    if (msg->values[index] != NULL) {
        return 0;
    }
    *status = read_scalar(r, field->type, &bits);
    if (*status != PBW_OK) {
        return 1;
    }
    value = pbw_value_new_uint_a(msg->arena, bits);
    if (value == NULL) {
        *status = PBW_ERR_NOMEM;
        return 1;
    }
    msg->values[index] = value;
    return 1;
}

static pbw_status
read_field(reader *r, const pbw_field *field,
           pbw_message *msg, uint32_t wire_type,
           uint32_t depth, const uint8_t *start)
{
    pbw_status status;
    uint64_t bits;
    const uint8_t *data;
    size_t data_len;
    pbw_arena *arena = msg->arena;

    /* Numeric repeated fields may arrive packed regardless of the descriptor
     * flag. */
    if (wire_type == PBW_WIRE_LEN && field->label == PBW_LABEL_REPEATED &&
        pbw_is_packable(field->type)) {
        reader sub;
        status = read_length(r, &data, &data_len);
        if (status != PBW_OK) {
            return status;
        }
        pbw_value *list = pbw_message_list_for(msg, field);

        if (list == NULL) {
            return PBW_ERR_NOMEM;
        }
        sub.p = data;
        sub.end = data + data_len;
        while (sub.p < sub.end) {
            pbw_value *value;
            status = read_scalar(&sub, field->type, &bits);
            if (status != PBW_OK) {
                return status;
            }
            value = pbw_value_new_uint_a(arena, bits);
            if (value == NULL || !pbw_value_list_push(list, value)) {
                pbw_value_free(value);
                return PBW_ERR_NOMEM;
            }
        }
        return PBW_OK;
    }

    if (wire_type != pbw_expected_wire_type(field->type)) {
        /* Wire type mismatch: treat as an unknown field, like the reference
         * decoders. */
        return skip_field(r, msg, field->number, wire_type, depth, start);
    }

    switch (field->type) {
    case PBW_TYPE_STRING:
    case PBW_TYPE_BYTES: {
        pbw_value *value;
        status = read_length(r, &data, &data_len);
        if (status != PBW_OK) {
            return status;
        }
        value = pbw_value_new_bytes_a(arena, data, data_len);
        if (value == NULL || !append_value(msg, field, value)) {
            pbw_value_free(value);
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    case PBW_TYPE_MESSAGE: {
        pbw_message *nested = NULL;
        pbw_value *value;
        size_t index = (size_t)(field - msg->desc->fields);

        if (depth >= PBW_MAX_DEPTH) {
            return PBW_ERR_LIMIT;
        }
        status = read_length(r, &data, &data_len);
        if (status != PBW_OK) {
            return status;
        }
        /* A singular message field that appears more than once is merged into
         * the existing submessage, not replaced (the wire format spec requires
         * it; reference implementations do the same). */
        if (field->label != PBW_LABEL_REPEATED &&
            (field->oneof_index == 0 ||
             msg->oneof_case[field->oneof_index] == field->number) &&
            msg->values[index] != NULL &&
            msg->values[index]->kind == PBW_VALUE_MESSAGE &&
            msg->values[index]->as.msg != NULL &&
            msg->values[index]->as.msg->desc == field->sub) {
            return decode_fields(msg->values[index]->as.msg, data, data_len,
                                 depth + 1);
        }
        status =
            decode_message(arena, field->sub, data, data_len, &nested, depth + 1);
        if (status != PBW_OK) {
            return status;
        }
        value = pbw_value_new_message_a(arena, nested);
        if (value == NULL || !append_value(msg, field, value)) {
            pbw_value_free(value);
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    default: {
        pbw_value *value;
        status = read_scalar(r, field->type, &bits);
        if (status != PBW_OK) {
            return status;
        }
        value = pbw_value_new_uint_a(arena, bits);
        if (value == NULL || !append_value(msg, field, value)) {
            pbw_value_free(value);
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    }
}

static pbw_status
decode_fields(pbw_message *msg, const uint8_t *buf, size_t len, uint32_t depth)
{
    reader r;
    const pbw_descriptor *desc = msg->desc;

    r.p = buf;
    r.end = buf + len;
    while (r.p < r.end) {
        uint32_t tag;
        size_t used;
        uint32_t wire_type;
        uint32_t number;
        const pbw_field *field;
        const uint8_t *start = r.p;
        pbw_status status;

        if (!pbw_tag_read(r.p, r.end, &tag, &used)) {
            return PBW_ERR_INVALID;
        }
        r.p += used;
        wire_type = tag & 7;
        number = tag >> 3;
        if (number == 0 || number > PBW_MAX_FIELD_NUMBER ||
            wire_type > PBW_WIRE_32BIT) {
            return PBW_ERR_INVALID;
        }
        if (wire_type == PBW_WIRE_END_GROUP) {
            return PBW_ERR_INVALID;
        }
        field = pbw_find_field(desc, number);
        if (field == NULL) {
            status = skip_field(&r, msg, number, wire_type, depth, start);
        } else if (read_scalar_fast(&r, field, msg, wire_type, &status)) {
            /* handled (or failed) without touching the general path */
        } else {
            status = read_field(&r, field, msg, wire_type, depth, start);
        }
        if (status != PBW_OK) {
            return status;
        }
    }
    return PBW_OK;
}

static pbw_status
decode_message(pbw_arena *arena, const pbw_descriptor *desc, const uint8_t *buf,
               size_t len, pbw_message **out, uint32_t depth)
{
    pbw_message *msg = pbw_message_new_a(arena, desc);
    pbw_status status;

    if (msg == NULL) {
        return PBW_ERR_NOMEM;
    }
    status = decode_fields(msg, buf, len, depth);
    if (status != PBW_OK) {
        pbw_message_free(msg);
        return status;
    }
    *out = msg;
    return PBW_OK;
}

pbw_status
pbw_decode_arena(pbw_arena *arena, const pbw_descriptor *desc, const void *buf,
                 size_t len, pbw_message **out)
{
    if (desc == NULL || out == NULL || (buf == NULL && len > 0)) {
        return PBW_ERR_INVALID;
    }
    if (desc->is_enum) {
        return PBW_ERR_TYPE;
    }
    if (desc->schema != NULL && !desc->schema->linked) {
        return PBW_ERR_INVALID;
    }
    return decode_message(arena, desc, (const uint8_t *)buf, len, out, 0);
}

pbw_status
pbw_decode(const pbw_descriptor *desc, const void *buf, size_t len,
           pbw_message **out)
{
    return pbw_decode_arena(NULL, desc, buf, len, out);
}
