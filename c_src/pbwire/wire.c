/* Wire format primitives: varints, tags, little-endian fixed values, output
 * buffer. */

#include "internal.h"

size_t
pbw_varint_write(uint8_t *out, uint64_t value)
{
    size_t len = 0;
    while (value >= 0x80) {
        out[len++] = (uint8_t)((value & 0x7f) | 0x80);
        value >>= 7;
    }
    out[len++] = (uint8_t)value;
    return len;
}

/* Returns 1 on success, 0 on truncation or overflow. Rejects varints longer
 * than 10 bytes, which no valid encoder produces. */
int
pbw_varint_read(const uint8_t *p, const uint8_t *end, uint64_t *value,
                size_t *used)
{
    uint64_t result = 0;
    size_t i;

    for (i = 0; i < 10 && p + i < end; i++) {
        uint8_t byte = p[i];
        result |= (uint64_t)(byte & 0x7f) << (7 * i);
        if ((byte & 0x80) == 0) {
            *value = result;
            *used = i + 1;
            return 1;
        }
    }
    return 0;
}

int
pbw_tag_read(const uint8_t *p, const uint8_t *end, uint32_t *tag, size_t *used)
{
    uint32_t result = 0;
    size_t i;

    for (i = 0; i < 5 && p + i < end; i++) {
        uint8_t byte = p[i];
        /* A tag is a varint32: its fifth byte may only carry the four low bits.
         * Silently truncating would turn an out-of-range tag into a valid one. */
        if (i == 4 && (byte & 0x7f) > 0x0f) {
            return 0;
        }
        result |= (uint32_t)(byte & 0x7f) << (7 * i);
        if ((byte & 0x80) == 0) {
            *tag = result;
            *used = i + 1;
            return 1;
        }
    }
    return 0;
}

void
pbw_buf_init(pbw_buf *buf)
{
    buf->data = NULL;
    buf->len = 0;
    buf->cap = 0;
    buf->oom = 0;
}

void
pbw_buf_free(pbw_buf *buf)
{
    free(buf->data);
    pbw_buf_init(buf);
}

int
pbw_buf_reserve(pbw_buf *buf, size_t extra)
{
    size_t need;
    size_t cap;
    uint8_t *data;

    if (buf->oom) {
        return 0;
    }
    need = buf->len + extra;
    if (need <= buf->cap) {
        return 1;
    }
    cap = buf->cap ? buf->cap : 64;
    while (cap < need) {
        cap *= 2;
    }
    data = (uint8_t *)realloc(buf->data, cap);
    if (data == NULL) {
        buf->oom = 1;
        return 0;
    }
    buf->data = data;
    buf->cap = cap;
    return 1;
}

void
pbw_write_le32(uint8_t *out, uint32_t value)
{
    out[0] = (uint8_t)(value & 0xff);
    out[1] = (uint8_t)((value >> 8) & 0xff);
    out[2] = (uint8_t)((value >> 16) & 0xff);
    out[3] = (uint8_t)((value >> 24) & 0xff);
}

void
pbw_write_le64(uint8_t *out, uint64_t value)
{
    pbw_write_le32(out, (uint32_t)(value & 0xffffffffu));
    pbw_write_le32(out + 4, (uint32_t)(value >> 32));
}

uint32_t
pbw_read_le32(const uint8_t *in)
{
    return (uint32_t)in[0] | ((uint32_t)in[1] << 8) | ((uint32_t)in[2] << 16) |
           ((uint32_t)in[3] << 24);
}

uint64_t
pbw_read_le64(const uint8_t *in)
{
    return (uint64_t)pbw_read_le32(in) | ((uint64_t)pbw_read_le32(in + 4) << 32);
}

void
pbw_free(void *ptr)
{
    free(ptr);
}

const char *
pbw_strerror(pbw_status status)
{
    switch (status) {
    case PBW_OK:
        return "ok";
    case PBW_ERR_INVALID:
        return "invalid serialized data";
    case PBW_ERR_NOMEM:
        return "out of memory";
    case PBW_ERR_NOT_FOUND:
        return "field not found";
    case PBW_ERR_TYPE:
        return "type mismatch";
    case PBW_ERR_LIMIT:
        return "limit exceeded";
    default:
        return "unknown error";
    }
}
