/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

#if 0
#define enc_ensure(env, enc, size) do { \
        printf("%s:%d:%s()\r\n", __FILE__, __LINE__, __func__); \
        _enc_ensure(env, enc, size);    \
} while(0)
#endif

#define enc_ensure_default(env, enc) enc_ensure(env, enc, MAX_UINT64_ENCODED_SIZE)

static inline ep_enc_t *
enc_ensure(ErlNifEnv *env, ep_enc_t *enc, size_t size)
{
    if (enc->p + size > enc->end) {

        enc->size *= 2;
        while (enc->mem + enc->size < enc->p + size) {
            enc->size *= 2;
        }

        enc->tmp = _realloc(enc->mem, enc->size);
        if (enc->tmp == NULL) {
            return NULL;
        }

        enc->sentinel = (enc->sentinel - enc->mem) + enc->tmp;
        enc->p = (enc->p - enc->mem) + enc->tmp;
        enc->mem = enc->tmp;
        enc->end = enc->tmp + enc->size;
    }

    return enc;
}

static inline void
do_pack_uint32(ErlNifEnv *env, uint32_t val, ep_enc_t *enc)
{
    if (val >= 0x80) {
        *(enc->p)++ = val | 0x80;
        val >>= 7;

        if (val >= 0x80) {
            *(enc->p)++ = val | 0x80;
            val >>= 7;

            if (val >= 0x80) {
                *(enc->p)++ = val | 0x80;
                val >>= 7;

                if (val >= 0x80) {
                    *(enc->p)++ = val | 0x80;
                    val >>= 7;
                }
            }
        }
    }

    *(enc->p)++ = val;
}

static inline ERL_NIF_TERM
pack_uint32(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    uint32_t    val;

    if (!enif_get_uint(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    do_pack_uint32(env, val, enc);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_sint32(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    int32_t     val;

    if (!enif_get_int(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    if (val < 0) {
        val = (-val) * 2 - 1;

    } else {
        val = val * 2;
    }

    do_pack_uint32(env, val, enc);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_int32(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    int32_t     val;

    if (!enif_get_int(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    if (val < 0) {
        *(enc->p)++ = val | 0x80;
        *(enc->p)++ = (val >> 7) | 0x80;
        *(enc->p)++ = (val >> 14) | 0x80;
        *(enc->p)++ = (val >> 21) | 0x80;
        *(enc->p)++ = (val >> 28) | 0x80;
        *((int32_t *) (enc->p)) = 0xffffffff;
        enc->p += sizeof(int32_t);
        *(enc->p)++ = 0x01;

    } else {
        do_pack_uint32(env, val, enc);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_fixed32(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    int32_t     val;

    if (!enif_get_int(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure(env, enc, sizeof(int32_t));

    //*((int32_t *) (enc->p)) = swap_int32(val);
    *((int32_t *) (enc->p)) = val;
    enc->p += sizeof(int32_t);

    return RET_OK;
}

static inline void
do_pack_uint64(ErlNifEnv *env, uint64_t val, ep_enc_t *enc)
{
    uint32_t hi = (uint32_t) (val >> 32);
    uint32_t lo = (uint32_t) val;

    if (hi == 0) {
        do_pack_uint32(env, (uint32_t) lo, enc);
        return;
    }

    *(enc->p)++ = (lo) | 0x80;
    *(enc->p)++ = (lo >> 7) | 0x80;
    *(enc->p)++ = (lo >> 14) | 0x80;
    *(enc->p)++ = (lo >> 21) | 0x80;

    if (hi < 8) {
        *(enc->p)++ = (hi << 4) | (lo >> 28);
        return;

    } else {
        *(enc->p)++ = ((hi & 7) << 4) | (lo >> 28) | 0x80;
        hi >>= 3;
    }

    while (hi >= 128) {
        *(enc->p)++ = hi | 0x80;
        hi >>= 7;
    }
    *(enc->p)++ = hi;
}

static inline ERL_NIF_TERM
pack_uint64(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifUInt64    val;

    if (!enif_get_uint64(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    do_pack_uint64(env, val, enc);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_sint64(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifSInt64    val;

    if (!enif_get_int64(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    if (val < 0) {
        val = (-val) * 2 - 1;

    } else {
        val = val * 2;
    }

    do_pack_uint64(env, val, enc);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_int64(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifSInt64    val;

    if (!enif_get_int64(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    do_pack_uint64(env, (uint64_t) val, enc);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_fixed64(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifSInt64    val;

    if (!enif_get_int64(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure(env, enc, sizeof(int64_t));

    //*((int64_t *) (enc->p)) = swap_int64(val);
    *((int64_t *) (enc->p)) = val;
    enc->p += sizeof(int64_t);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_boolean(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    int32_t         val;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    if (term == state->atom_true) {
        val = 1;

    } else if (term == state->atom_false) {
        val = 0;

    } else if (!enif_get_int(env, term, &val) || (val != 1 && val != 0)) {
        raise_exception(env, term);
    }

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure(env, enc, sizeof(int8_t));

    *(enc->p)++ = (int8_t) (val & 0x000000ff);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_float(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    float           val;
    double          d_val;
    int32_t         i_val;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    if (enif_get_double(env, term, &d_val)) {
        val = (float) d_val;

    } else if (enif_get_int(env, term, &i_val)) {
        val = (float) i_val;

    } else if (enif_is_atom(env, term)) {

        if (term == state->atom_infinity) {
            val = INFINITY;

        } else if (term == state->atom_min_infinity) {
            val = -INFINITY;

        } else if (term == state->atom_nan) {
            val = _NAN;

        } else {
            raise_exception(env, term);
        }

    } else {
        raise_exception(env, term);
    }

    if (val == 0.0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure(env, enc, sizeof(int32_t));

    *((int32_t *) (enc->p)) = *((int32_t *) &val);
    enc->p += sizeof(int32_t);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_double(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    double          val;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ErlNifSInt64    l_val;

    if (!enif_get_double(env, term, &val)) {

        if (enif_get_int64(env, term, &l_val)) {
            val = (double) l_val;

        } else if (enif_is_atom(env, term)) {

            if (term == state->atom_infinity) {
                val = INFINITY;

            } else if (term == state->atom_min_infinity) {
                val = -INFINITY;

            } else if (term == state->atom_nan) {
                val = _NAN;

            } else {
                raise_exception(env, term);
            }

        } else {
            raise_exception(env, term);
        }
    }

    if (val == 0.0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure(env, enc, sizeof(int64_t));

    *((int64_t *) (enc->p)) = *((int64_t *) &val);

    enc->p += sizeof(int64_t);

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_utf8(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    uint32_t    val;

    if (!enif_get_uint(env, term, &val)) {
        raise_exception(env, term);
    }

    if (val < 0x80) {
        *(enc->p)++ = (uint8_t) val;

    } else if (val < 0x800) {
        *(enc->p)++ = (uint8_t) (0xC0 + (val >> 6));
        *(enc->p)++ = (uint8_t) (0x80 + (val & 0x3F));

    } else if (val == 0xFFFF) {
        *(enc->p)++ = (uint8_t) 0xFF;

    } else if (val == 0xFFFE) {
        *(enc->p)++ = (uint8_t) 0xFE;

    } else if (val < 0x10000) {
        *(enc->p)++ = (uint8_t) (0xE0 + (val >> 12));
        *(enc->p)++ = (uint8_t) (0x80 + ((val >> 6) & 0x3F));
        *(enc->p)++ = (uint8_t) (0x80 + (val & 0x3F));

    } else if (val < 0x110000) {
        *(enc->p)++ = (uint8_t) (0xF0 + (val >> 18));
        *(enc->p)++ = (uint8_t) (0x80 + ((val >> 12) & 0x3F));
        *(enc->p)++ = (uint8_t) (0x80 + ((val >> 6) & 0x3F));
        *(enc->p)++ = (uint8_t) (0x80 + (val & 0x3F));

    } else {
        raise_exception(env, term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_bytes(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifBinary    bin;

    if (!enif_inspect_iolist_as_binary(env, term, &bin)) {
        raise_exception(env, term);
    }

    if (bin.size == 0) {
        if (field->proto_v == 3 && field->o_type == occurrence_defaulty) {
            return RET_OK;
        }

        enc_ensure(env, enc, 1);
        *(enc->p++) = 0;
        return RET_OK;
    }

    enc_ensure(env, enc, MAX_UINT64_ENCODED_SIZE + bin.size);

    do_pack_uint32(env, (uint32_t) bin.size, enc);

    memcpy(enc->p, bin.data, bin.size);
    enc->p += bin.size;

    return RET_OK;
}

static inline int64_t
to_integer(unsigned char *str, size_t size) {
    int32_t c;
    int32_t sign;
    int32_t offset;
    int64_t n;

    if (str[0] == '-') {  // Handle negative integers
        sign = -1;
    } else {
        sign = 0;
    }

    if (sign == -1) {  // Set starting position to convert
        offset = 1;
    }
    else {
        offset = 0;
    }

    n = 0;
    for (c = offset; c < size; c++) {
        n = n * 10 + str[c] - '0';
    }

    if (sign == -1) {
        n = -n;
    }

    return n;
}


static inline int64_t
pack_bin_as_int64(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t* field)
{
    ErlNifBinary    bin;    // ErlNifBinary need not be released according to nif docs.
    if (!enif_inspect_iolist_as_binary(env, term, &bin)) {
        return_error(env, term);
    }

    int64_t     val;
    val = to_integer(bin.data, bin.size);

    if (val == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);
    if (val < 0) {
        return_error(env, term);
    } else {
        do_pack_uint64(env, (uint64_t) val, enc);
    }

    return RET_OK;
}


static inline ERL_NIF_TERM
pack_string_list(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ErlNifBinary    bin;
    ERL_NIF_TERM    head, tail, ret;

    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (enif_is_list(env, head)) {
            check_ret(ret, pack_string_list(env, head, enc, field));
        } else if (enif_is_binary(env, head) && enif_inspect_binary(env, head, &bin)) {
            if (bin.size > 0) {
                enc_ensure(env, enc, bin.size);
                memcpy(enc->p, bin.data, bin.size);
                enc->p += bin.size;
            }
        } else {
            enc_ensure(env, enc, 4);
            check_ret(ret, pack_utf8(env, head, enc, field));
        }
        term = tail;
    }
    return RET_OK;
}

static inline ERL_NIF_TERM
pack_string(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    size_t          payload_len;
    uint8_t        *p, *end;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ErlNifBinary    bin;
    ERL_NIF_TERM    ret;

    if (!state->opts.with_utf8) {
        return pack_bytes(env, term, enc, field);
    }

    if (enif_is_list(env, term)) {
        if (enif_is_empty_list(env, term)) {
            if (field->proto_v == 3 && field->o_type == occurrence_defaulty) {
                return RET_OK;
            }

            enc_ensure(env, enc, 1);
            *(enc->p++) = 0;
            return RET_OK;
        }

        enc_ensure(env, enc, MAX_UINT64_ENCODED_SIZE);
        enc->p += MAX_UINT64_ENCODED_SIZE;
        enc->tmp = enc->p;

        if (enif_inspect_iolist_as_binary(env, term, &bin)) {
            p = bin.data;
            end = bin.data + bin.size;
            while (p < end) {
                if (*p < 0x80) {
                    *(enc->p)++ = *p;
                } else {
                    *(enc->p)++ = (uint8_t) (0xC0 + (*p >> 6));
                    *(enc->p)++ = (uint8_t) (0x80 + (*p & 0x3F));
                }
                p++;
            }
        } else {
            check_ret(ret, pack_string_list(env, term, enc, field));
        }

        payload_len = enc->p - enc->tmp;
        enc->p = enc->tmp - MAX_UINT64_ENCODED_SIZE;
        if (payload_len == 0 && field->proto_v == 3 && field->o_type == occurrence_defaulty) {
            return RET_OK;
        }
        do_pack_uint32(env, (uint32_t) payload_len, enc);
        memmove(enc->p, enc->tmp, payload_len);
        enc->p += payload_len;
    } else if (enif_is_binary(env, term) && enif_inspect_binary(env, term, &bin)) {

        if (bin.size == 0) {
            if (field->proto_v == 3 && field->o_type == occurrence_defaulty) {
                return RET_OK;
            }

            enc_ensure(env, enc, 1);
            *(enc->p++) = 0;
            return RET_OK;
        }

        enc_ensure(env, enc, MAX_UINT64_ENCODED_SIZE + bin.size);
        do_pack_uint32(env, (uint32_t) bin.size, enc);
        memcpy(enc->p, bin.data, bin.size);
        enc->p += bin.size;
    } else {
        raise_exception(env, term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_enum(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    int32_t             val;
    ep_node_t          *node = field->sub_node;
    ep_enum_field_t    *sub_field;

    if (enif_is_atom(env, term)) {
        sub_field = bsearch(&term, node->fields, node->size, sizeof(ep_enum_field_t), get_enum_compare_name);
        if (sub_field == NULL) {
            raise_exception(env, term);
        }
        val = sub_field->value;
    } else if (!enif_get_int(env, term, &val)) {
        raise_exception(env, term);
    }

    sub_field = node->fields;
    if (sub_field->value == val && sub_field->proto_v == 3 && field->o_type == occurrence_defaulty) {
        return RET_OK;
    }

    enc_ensure_default(env, enc);

    if (val < 0) {
        *(enc->p)++ = val | 0x80;
        *(enc->p)++ = (val >> 7) | 0x80;
        *(enc->p)++ = (val >> 14) | 0x80;
        *(enc->p)++ = (val >> 21) | 0x80;
        *(enc->p)++ = (val >> 28) | 0x80;
        *((int32_t *) (enc->p)) = 0xffffffff;
        enc->p += sizeof(int32_t);
        *(enc->p)++ = 0x01;
    } else {
        do_pack_uint32(env, val, enc);
    };

    return RET_OK;
}

static inline ERL_NIF_TERM
pack_tag(ErlNifEnv *env, uint32_t id, ep_enc_t *enc)
{
    enc_ensure_default(env, enc);

    if (id < (1UL << (32 - 3))) {
        do_pack_uint32(env, id << 3, enc);

    } else {

        do_pack_uint64(env, ((uint64_t) id) << 3, enc);
    }

    return RET_OK;
}

static inline ep_field_t *
get_oneof_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node, ERL_NIF_TERM *out)
{
    int32_t         arity;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        return NULL;
    }

    unsigned size = 100;
    char buf[100];
    char* output = get_atom(env, array[0], buf, size);

    if (output == NULL) {
        return NULL;
    }

    // We try and fetch the specific field that was set as part of the oneof field.
    // oneof field values are set directly to the actual record if the value is a new message.
    // If the value is one of the basic types - then it is a tuple with the variable name followed by the value.
    // Here, we first check if the value is another message by checking if the prefix is set to pb_.
    // If not then we expect the value to be one of the basic types and compare the variable name.

    if(strncmp(buf, "pb_", 3) == 0) {
        *out = term;
        return bsearch((&(array[0])), node->fields, node->size, sizeof(ep_field_t), get_field_compare_sub_name);
    } else {
        *out = array[1];
        for (int i = 0; i < node->size; ++i) {
            ep_field_t *result = node->fields + i*sizeof(ep_field_t);
            if (get_field_compare_name((&(array[0])), result) == 0) {
                return result;
            }
        }
        return NULL;
    }
}

static ERL_NIF_TERM
pack_element_packed(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    ERL_NIF_TERM    ret;

    switch (field->type) {
    case field_sint32:
        check_ret(ret, pack_sint32(env, term, enc, field));
        break;

    case field_enum:
        check_ret(ret, pack_enum(env, term, enc, field));
        break;

    case field_int32:
        check_ret(ret, pack_int32(env, term, enc, field));
        break;

    case field_uint32:
        check_ret(ret, pack_uint32(env, term, enc, field));
        break;

    case field_sint64:
        check_ret(ret, pack_sint64(env, term, enc, field));
        break;

    case field_int64:
        if (field->ebin == TRUE) {
            check_ret(ret, pack_bin_as_int64(env, term, enc, field));
        } else {
            check_ret(ret, pack_int64(env, term, enc, field));
        }
        break;

    case field_uint64:
        check_ret(ret, pack_uint64(env, term, enc, field));
        break;

    case field_sfixed32:
    case field_fixed32:
        check_ret(ret, pack_fixed32(env, term, enc, field));
        break;

    case field_float:
        check_ret(ret, pack_float(env, term, enc, field));
        break;

    case field_sfixed64:
    case field_fixed64:
        check_ret(ret, pack_fixed64(env, term, enc, field));
        break;

    case field_double:
        check_ret(ret, pack_double(env, term, enc, field));
        break;

    case field_bool:
        check_ret(ret, pack_boolean(env, term, enc, field));
        break;

    default:
        raise_exception(env, term);
    }

    return RET_OK;
}

static ERL_NIF_TERM
pack_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_enc_t *enc, ep_field_t *field)
{
    char           *sentinel;
    ERL_NIF_TERM    ret;

    enc->sentinel = enc->p;
    pack_tag(env, field->fnum, enc);
    sentinel = enc->p;

    switch (field->type) {
    case field_sint32:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_sint32(env, term, enc, field));
        break;

    case field_enum:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_enum(env, term, enc, field));
        break;

    case field_int32:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_int32(env, term, enc, field));
        break;

    case field_uint32:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_uint32(env, term, enc, field));
        break;

    case field_sint64:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_sint64(env, term, enc, field));
        break;

    case field_int64:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        if (field->ebin == TRUE) {
            check_ret(ret, pack_bin_as_int64(env, term, enc, field));
        } else {
            check_ret(ret, pack_int64(env, term, enc, field));
        }
        break;

    case field_uint64:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_uint64(env, term, enc, field));
        break;

    case field_sfixed32:
    case field_fixed32:
        *(enc->sentinel) |= WIRE_TYPE_32BIT;
        check_ret(ret, pack_fixed32(env, term, enc, field));
        break;

    case field_float:
        *(enc->sentinel) |= WIRE_TYPE_32BIT;
        check_ret(ret, pack_float(env, term, enc, field));
        break;

    case field_sfixed64:
    case field_fixed64:
        *(enc->sentinel) |= WIRE_TYPE_64BIT;
        check_ret(ret, pack_fixed64(env, term, enc, field));
        break;

    case field_double:
        *(enc->sentinel) |= WIRE_TYPE_64BIT;
        check_ret(ret, pack_double(env, term, enc, field));
        break;

    case field_bool:
        *(enc->sentinel) |= WIRE_TYPE_VARINT;
        check_ret(ret, pack_boolean(env, term, enc, field));
        break;

    case field_string:
        *(enc->sentinel) |= WIRE_TYPE_LENGTH_PREFIXED;
        check_ret(ret, pack_string(env, term, enc, field));
        break;

    case field_bytes:
        *(enc->sentinel) |= WIRE_TYPE_LENGTH_PREFIXED;
        check_ret(ret, pack_bytes(env, term, enc, field));
        break;

    default:
        raise_exception(env, term);
    }

    if (sentinel == enc->p) {
        enc->p = enc->sentinel;
    }

    return RET_OK;
}

ERL_NIF_TERM
encode(ErlNifEnv *env, ERL_NIF_TERM term, ep_tdata_t *tdata)
{
    size_t          i, payload_len;
    int32_t         arity;
    ep_enc_t       *enc;
    ep_spot_t      *spot;
    ep_stack_t     *stack;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ep_field_t     *field;
    ErlNifBinary    bin;
    ERL_NIF_TERM    head, tail, ret;

    stack = &(tdata->stack);
    enc = &(tdata->enc);

    spot = stack->spots;
    if (!enif_get_tuple(env, term, &arity, to_const(spot->array))) {
        raise_exception(env, term);
    }

    spot->node = get_node_by_name(spot->array[0], state->cache);
    if (spot->node == NULL || (arity - 1) != spot->node->size) {
        raise_exception(env, spot->array[0]);
    }
    (spot->array)++;

    spot->type = spot_tuple;
    spot->pos = 0;

    while (spot >= stack->spots) {

        if (spot->type == spot_tuple) {
            if (spot->pos == spot->node->size) {
                if (spot->need_length) {
                    spot->need_length = FALSE;

                    payload_len = enc->p - (enc->mem + spot->sentinel_size + MAX_UINT64_ENCODED_SIZE);
                    enc->p = enc->mem + spot->sentinel_size;
                    do_pack_uint32(env, (uint32_t) payload_len, enc);
                    memmove(enc->p, enc->mem + spot->sentinel_size + MAX_UINT64_ENCODED_SIZE, payload_len);
                    enc->p += payload_len;
                }
                spot->pos = 0;
                spot--;
                continue;
            }

            for (i = spot->pos; i < (size_t) (spot->node->size); i++) {
                spot->pos = i + 1;
                term = spot->array[i];
                field = ((ep_field_t *) (spot->node->fields)) + i;

                if (field->o_type == occurrence_optional
                    || field->o_type == occurrence_defaulty
                    || field->type == field_oneof
                ) {
                    if (term == state->atom_undefined) {
                        continue;
                    }
                }

                if (field->o_type == occurrence_repeated && field->packed == TRUE) {
                    if (!enif_is_list(env, term)) {
                        raise_exception(env, spot->array[i]);
                    }

                    if (enif_is_empty_list(env, term)) {
                        continue;
                    }

                    enc->sentinel = enc->p;
                    pack_tag(env, field->fnum, enc);
                    *(enc->sentinel) |= WIRE_TYPE_LENGTH_PREFIXED;

                    enc->sentinel = enc->p;
                    enc_ensure_default(env, enc);
                    enc->p += MAX_UINT64_ENCODED_SIZE;

                    while (enif_get_list_cell(env, term, &head, &tail)) {
                        check_ret(ret, pack_element_packed(env, head, enc, field));
                        term = tail;
                    }

                    payload_len = enc->p - enc->sentinel - MAX_UINT64_ENCODED_SIZE;
                    enc->p = enc->sentinel;
                    do_pack_uint32(env, (uint32_t) payload_len, enc);
                    memmove(enc->p, enc->sentinel + MAX_UINT64_ENCODED_SIZE, payload_len);
                    enc->p += payload_len;

                } else if (field->o_type == occurrence_repeated) {

                    if (!enif_is_list(env, term)) {
                        raise_exception(env, term);
                    }

                    if (enif_is_empty_list(env, term)) {
                        continue;
                    }

                    if (field->type == field_msg || field->type == field_map) {

                        spot++;
                        spot->node = field->sub_node;
                        if (spot->node == NULL) {
                            raise_exception(env, term);
                        }

                        spot->field = field;
                        spot->type = spot_list;
                        spot->list = term;
                        break;

                    } else {
                        while (enif_get_list_cell(env, term, &head, &tail)) {
                            check_ret(ret, pack_field(env, head, enc, field));
                            term = tail;
                        }
                    }

                } else {
                    if (field->type == field_oneof) {
                        field = get_oneof_field(env, spot->array[i], field->sub_node, &term);
                        if (field == NULL) {
                            raise_exception(env, spot->array[i]);
                        }
                    }

                    if (field->type == field_msg || field->type == field_map) {
                        enc->sentinel = enc->p;
                        pack_tag(env, field->fnum, enc);
                        *(enc->sentinel) |= WIRE_TYPE_LENGTH_PREFIXED;

                        spot++;

                        if (!enif_get_tuple(env, term, &arity, to_const(spot->array))) {
                            raise_exception(env, term);
                        }

                        (spot->array)++;
                        arity--;

                        spot->node = field->sub_node;
                        if (spot->node == NULL || arity != spot->node->size) {
                            raise_exception(env, term);
                        }

                        spot->type = spot_tuple;
                        spot->pos = 0;

                        spot->need_length = TRUE;
                        spot->sentinel_size = enc->p - enc->mem;
                        enc_ensure_default(env, enc);
                        enc->p += MAX_UINT64_ENCODED_SIZE;

                        break;
                    } else {
                        check_ret(ret, pack_field(env, term, enc, field));
                    }
                }
            }

        } else if (spot->type == spot_list) {

            if (!enif_get_list_cell(env, spot->list, &head, &tail)) {
                spot--;
                continue;
            }

            spot->list = tail;

            enc->sentinel = enc->p;
            pack_tag(env, spot->field->fnum, enc);
            *(enc->sentinel) |= WIRE_TYPE_LENGTH_PREFIXED;

            spot++;
            spot->node = (spot - 1)->node;
            spot->type = spot_tuple;
            spot->pos = 0;

            spot->need_length = TRUE;
            spot->sentinel_size = enc->p - enc->mem;
            enc_ensure_default(env, enc);
            enc->p += MAX_UINT64_ENCODED_SIZE;

            if (!enif_get_tuple(env, head, &arity, to_const(spot->array))) {
                raise_exception(env, head);
            }

            if (spot->node->n_type != node_map) {
                (spot->array)++;
            }
        }
    }

    if (enc->p > enc->mem) {

        if (!enif_alloc_binary(enc->p - enc->mem, &bin)) {
            raise_exception(env, make_atom(env, "alloc_binary"));
        }

        memcpy(bin.data, enc->mem, bin.size);

    } else {
        if (!enif_alloc_binary(0, &bin)) {
            raise_exception(env, make_atom(env, "alloc_binary"));
        }
    }
    enc->result = enif_make_binary(env, &bin);

    return RET_OK;
}
