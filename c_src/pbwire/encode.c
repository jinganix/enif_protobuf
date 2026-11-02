/* Wire encoder: dynamic message tree -> Protobuf binary.
 *
 * Single pass. The output buffer grows on demand, length-delimited payloads are
 * written first with a one-byte length placeholder and patched afterwards (the
 * payload is shifted right when the length needs more than one byte). No sizing
 * pass, no temporary buffer per nested message, no allocation per field.
 */

#include "internal.h"

typedef struct {
    pbw_buf *buf;
    size_t pos; /* write offset; buf->data may move on growth */
    pbw_status status;
} writer;

int
pbw_is_packable(pbw_type type)
{
    switch (type) {
    case PBW_TYPE_STRING:
    case PBW_TYPE_BYTES:
    case PBW_TYPE_MESSAGE:
    case PBW_TYPE_GROUP:
        return 0;
    default:
        return 1;
    }
}

uint32_t
pbw_expected_wire_type(pbw_type type)
{
    switch (type) {
    case PBW_TYPE_DOUBLE:
    case PBW_TYPE_FIXED64:
    case PBW_TYPE_SFIXED64:
        return PBW_WIRE_64BIT;
    case PBW_TYPE_FLOAT:
    case PBW_TYPE_FIXED32:
    case PBW_TYPE_SFIXED32:
        return PBW_WIRE_32BIT;
    case PBW_TYPE_STRING:
    case PBW_TYPE_BYTES:
    case PBW_TYPE_MESSAGE:
        return PBW_WIRE_LEN;
    case PBW_TYPE_GROUP:
        return PBW_WIRE_START_GROUP;
    default:
        return PBW_WIRE_VARINT;
    }
}

static uint64_t
zigzag_encode(uint64_t value, unsigned width)
{
    uint64_t mask = width == 32 ? 0xffffffffull : ~0ull;
    uint64_t sign = width == 32 ? 31 : 63;
    uint64_t u = value & mask;

    return ((u << 1) ^ (0ull - (u >> sign))) & mask;
}

/* Value as it goes on the wire, before tag and length. */
static uint64_t
wire_value(pbw_type type, uint64_t u)
{
    switch (type) {
    case PBW_TYPE_INT32:
    case PBW_TYPE_ENUM:
        return (uint64_t)(int64_t)(int32_t)u;
    case PBW_TYPE_UINT32:
        return (uint32_t)u;
    case PBW_TYPE_SINT32:
        return zigzag_encode(u, 32);
    case PBW_TYPE_SINT64:
        return zigzag_encode(u, 64);
    case PBW_TYPE_BOOL:
        return u != 0 ? 1u : 0u;
    default:
        return u;
    }
}

/* ---------------------------------------------------------------- writing */

static int
w_reserve(writer *w, size_t extra)
{
    if (w->buf->cap - w->pos >= extra) {
        return 1;
    }
    w->buf->len = w->pos;
    if (!pbw_buf_reserve(w->buf, extra)) {
        w->status = PBW_ERR_NOMEM;
        return 0;
    }
    return 1;
}

static void
w_bytes(writer *w, const void *data, size_t len)
{
    if (len == 0) {
        return;
    }
    if (!w_reserve(w, len)) {
        return;
    }
    memcpy(w->buf->data + w->pos, data, len);
    w->pos += len;
}

static void
w_varint_at(uint8_t *out, uint64_t value)
{
    while (value >= 0x80) {
        *out++ = (uint8_t)((value & 0x7f) | 0x80);
        value >>= 7;
    }
    *out = (uint8_t)value;
}

static void
w_varint(writer *w, uint64_t value)
{
    uint8_t *out;
    size_t len = 0;

    /* Reserve the maximum width so one loop writes the value directly: encoding
     * only appends, so the slack past w->pos is never read. */
    if (!w_reserve(w, 10)) {
        return;
    }
    out = w->buf->data + w->pos;
    while (value >= 0x80) {
        out[len++] = (uint8_t)((value & 0x7f) | 0x80);
        value >>= 7;
    }
    out[len++] = (uint8_t)value;
    w->pos += len;
}

static void
w_tag(writer *w, uint32_t number, uint32_t wire_type)
{
    w_varint(w, ((uint64_t)number << 3) | wire_type);
}

static void
w_fixed(writer *w, uint64_t value, size_t size)
{
    uint8_t *out;
    size_t i;

    if (!w_reserve(w, size)) {
        return;
    }
    out = w->buf->data + w->pos;
    for (i = 0; i < size; i++) {
        out[i] = (uint8_t)((value >> (8 * i)) & 0xff);
    }
    w->pos += size;
}

static void
w_scalar(writer *w, pbw_type type, uint64_t bits)
{
    switch (type) {
    case PBW_TYPE_FLOAT:
    case PBW_TYPE_FIXED32:
    case PBW_TYPE_SFIXED32:
        w_fixed(w, bits, 4);
        return;
    case PBW_TYPE_DOUBLE:
    case PBW_TYPE_FIXED64:
    case PBW_TYPE_SFIXED64:
        w_fixed(w, bits, 8);
        return;
    default:
        w_varint(w, wire_value(type, bits));
        return;
    }
}

/* A length-delimited payload needs its length before its content, but the
 * length is only known afterwards: write a one-byte placeholder, encode, then
 * patch it, shifting the payload right when the length varint is longer. */
static void
w_len_begin(writer *w, size_t *mark)
{
    *mark = w->pos;
    if (!w_reserve(w, 1)) {
        return;
    }
    w->buf->data[w->pos] = 0;
    w->pos += 1;
}

static void
w_len_end(writer *w, size_t mark)
{
    size_t payload = w->pos - mark - 1;
    size_t len_bytes = pbw_varint_size(payload);
    size_t shift = len_bytes - 1;
    uint8_t *base;

    if (shift > 0) {
        if (!w_reserve(w, shift)) {
            return;
        }
        base = w->buf->data;
        memmove(base + mark + 1 + shift, base + mark + 1, payload);
        w->pos += shift;
    }
    w_varint_at(w->buf->data + mark, payload);
}

static void encode_message(writer *w, const pbw_message *msg, uint32_t depth);

static void
write_element(writer *w, const pbw_field *field, const pbw_value *value,
              uint32_t depth)
{
    if (depth > PBW_MAX_DEPTH) {
        w->status = PBW_ERR_LIMIT;
        return;
    }
    switch (field->type) {
    case PBW_TYPE_STRING:
    case PBW_TYPE_BYTES:
        /* Length is known before the payload: write it directly. */
        w_tag(w, field->number, PBW_WIRE_LEN);
        w_varint(w, value->as.bytes.len);
        w_bytes(w, value->as.bytes.data, value->as.bytes.len);
        return;
    case PBW_TYPE_MESSAGE: {
        size_t mark;

        if (value->as.msg == NULL) {
            w->status = PBW_ERR_INVALID;
            return;
        }
        if (depth == PBW_MAX_DEPTH) {
            w->status = PBW_ERR_LIMIT;
            return;
        }
        w_tag(w, field->number, PBW_WIRE_LEN);
        w_len_begin(w, &mark);
        encode_message(w, value->as.msg, depth + 1);
        if (w->status != PBW_OK) {
            return;
        }
        w_len_end(w, mark);
        return;
    }
    default:
        w_tag(w, field->number, pbw_expected_wire_type(field->type));
        w_scalar(w, field->type, value->as.u64);
        return;
    }
}

static void
write_packed(writer *w, const pbw_field *field, const pbw_value *list)
{
    size_t mark;
    size_t i;

    w_tag(w, field->number, PBW_WIRE_LEN);
    w_len_begin(w, &mark);
    for (i = 0; i < list->as.list.len; i++) {
        w_scalar(w, field->type, list->as.list.items[i]->as.u64);
        if (w->status != PBW_OK) {
            return;
        }
    }
    w_len_end(w, mark);
}

static void
encode_message(writer *w, const pbw_message *msg, uint32_t depth)
{
    size_t i;

    for (i = 0; i < msg->desc->field_count; i++) {
        const pbw_field *field = &msg->desc->fields[i];
        const pbw_value *value = msg->values[i];
        size_t j;

        if (value == NULL) {
            continue;
        }
        if (field->label == PBW_LABEL_REPEATED) {
            if (value->kind != PBW_VALUE_LIST || value->as.list.len == 0) {
                continue;
            }
            if (field->packed && pbw_is_packable(field->type)) {
                write_packed(w, field, value);
                continue;
            }
            for (j = 0; j < value->as.list.len; j++) {
                write_element(w, field, value->as.list.items[j], depth);
                if (w->status != PBW_OK) {
                    return;
                }
            }
            continue;
        }
        write_element(w, field, value, depth);
        if (w->status != PBW_OK) {
            return;
        }
    }
    /* Unknown fields go last, in the order they were read: the reference
     * implementations do the same, so a re-encoded message stays byte-identical
     * for a peer that only knows part of the schema. */
    if (msg->unknown_len > 0) {
        w_bytes(w, msg->unknown, msg->unknown_len);
    }
}

/* -------------------------------------------------------------- entry points */

/* Encodes into a caller-provided buffer whose capacity is reused across calls. */
pbw_status
pbw_encode_buf(pbw_buf *buf, const pbw_message *msg)
{
    writer w;

    if (msg == NULL || buf == NULL) {
        return PBW_ERR_INVALID;
    }
    buf->len = 0;
    w.buf = buf;
    w.pos = 0;
    w.status = PBW_OK;
    encode_message(&w, msg, 0);
    buf->len = w.pos;
    return w.status;
}

pbw_status
pbw_encode(const pbw_message *msg, uint8_t **out, size_t *len)
{
    pbw_buf buf;
    writer w;

    if (msg == NULL || out == NULL || len == NULL) {
        return PBW_ERR_INVALID;
    }
    pbw_buf_init(&buf);
    w.buf = &buf;
    w.pos = 0;
    w.status = PBW_OK;
    encode_message(&w, msg, 0);
    if (w.status != PBW_OK) {
        pbw_buf_free(&buf);
        return w.status;
    }
    if (buf.data == NULL) {
        /* Empty message: hand out a valid buffer the caller can free. */
        buf.data = (uint8_t *)malloc(1);
        if (buf.data == NULL) {
            return PBW_ERR_NOMEM;
        }
    }
    /* Transfer the buffer instead of copying: it is already exactly the size
     * the message needed (plus at most the last growth step). */
    *out = buf.data;
    *len = w.pos;
    return PBW_OK;
}
