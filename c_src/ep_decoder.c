#include "enif_protobuf.h"

static inline ERL_NIF_TERM
skip_field(ErlNifEnv *env, ep_dec_t *dec, wire_type_e wire_type);
static inline ERL_NIF_TERM
skip_group(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum);
static inline ERL_NIF_TERM
skip_unknown_field(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, wire_type_e wire_type);

static inline ERL_NIF_TERM
group_content_bounds(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum, char **end_tag);

static inline ERL_NIF_TERM
consume_group_end(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum);

static inline ERL_NIF_TERM
collect_unknown_field(ErlNifEnv *env, ep_dec_t *dec, uint32_t *fnum, wire_type_e *wire_type, ERL_NIF_TERM *term);

static inline ERL_NIF_TERM
fill_default(ErlNifEnv *env, ep_spot_t *spot);

static inline ERL_NIF_TERM
unpack_varint_u64(ErlNifEnv *env, ep_dec_t *dec, uint64_t *val)
{
    uint32_t shift = 0, left = 10;
    uint64_t tmp = 0;
    char *p = dec->p;
    char *end = dec->end;

    while (left && p < end) {
        uint8_t c = (uint8_t)(*p++);
        tmp |= ((uint64_t)(c & 0x7f) << shift);
        if ((c & 0x80) == 0) {
            dec->p = p;
            *val = tmp;
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    enif_raise_exception(env, dec->term);
    return RET_ERROR;
}

static inline ERL_NIF_TERM
do_unpack_uint32(ErlNifEnv *env, ep_dec_t *dec, uint32_t *val)
{
    uint64_t tmp;
    ERL_NIF_TERM ret;

    check_ret(ret, unpack_varint_u64(env, dec, &tmp));
    *val = (uint32_t)tmp;
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_uint32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *term = enif_make_uint(env, (uint32_t)val);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_sint32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int64_t v;
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    if (val & 1) {
        v = -(val >> 1) - 1;
    } else {
        v = (val >> 1);
    }

    if (val > 4294967295) {
        v = val & 1 ? -2147483648 : 0;
    }

    *term = enif_make_int(env, v);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_int32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *term = enif_make_int(env, (int)val);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_fixed32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t val = 0;

    if (dec->p + sizeof(uint32_t) <= dec->end) {

        val = *(uint32_t *)(dec->p);
        *term = enif_make_uint(env, val);
        dec->p += sizeof(uint32_t);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sfixed32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int32_t val = 0;

    if (dec->p + sizeof(int32_t) <= dec->end) {

        val = *(int32_t *)(dec->p);
        *term = enif_make_int(env, val);
        dec->p += sizeof(int32_t);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_uint64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *term = enif_make_uint64(env, (ErlNifUInt64)val);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_sint64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int64_t v;
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    if (val & 1) {
        v = -(int64_t)(val >> 1) - 1;

    } else {
        v = val >> 1;
    }

    *term = enif_make_int64(env, (ErlNifSInt64)v);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_int64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ERL_NIF_TERM ret;
    uint64_t val = 0;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *term = enif_make_int64(env, (ErlNifSInt64)val);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_fixed64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint64_t val;

    if (dec->p + sizeof(uint64_t) <= dec->end) {

        val = *(uint64_t *)(dec->p);
        *term = enif_make_uint64(env, (ErlNifUInt64)val);
        dec->p += sizeof(uint64_t);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sfixed64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int64_t val;

    if (dec->p + sizeof(int64_t) <= dec->end) {

        val = *(int64_t *)(dec->p);
        *term = enif_make_int64(env, (ErlNifSInt64)val);
        dec->p += sizeof(int64_t);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_boolean(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);

    if (dec->p + sizeof(int8_t) <= dec->end) {

        if (*(dec->p) == 1) {
            *term = state->atom_true;

        } else if (*(dec->p) == 0) {
            *term = state->atom_false;

        } else {
            raise_exception(env, dec->term);
        }

        (dec->p)++;
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_float(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    float val;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);

    if (dec->p + sizeof(float) <= dec->end) {

        val = *(float *)(dec->p);

        if (val == INFINITY) {
            *term = state->atom_infinity;

        } else if (val == -INFINITY) {
            *term = state->atom_min_infinity;

        } else if (val != val) {
            *term = state->atom_nan;

        } else {
            *term = enif_make_double(env, (double)val);
        }

        dec->p += sizeof(float);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_double(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    double val;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);

    if (dec->p + sizeof(double) <= dec->end) {

        val = *(double *)(dec->p);

        if (val == INFINITY) {
            *term = state->atom_infinity;

        } else if (val == -INFINITY) {
            *term = state->atom_min_infinity;

        } else if (val != val) {
            *term = state->atom_nan;

        } else {
            *term = enif_make_double(env, val);
        }

        dec->p += sizeof(double);
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_utf8(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t size = (uint32_t)(dec->end - dec->p);
    uint32_t val;

    if ((*(dec->p) & 0x80) == 0) {
        val = *(dec->p)++;

    } else if ((*(dec->p) & 0xE0) == 0xC0 && size >= 2) {
        val = ((dec->p[0] & 0x1F) << 6) | ((dec->p[1] & 0x3F));
        dec->p += 2;

    } else if ((dec->p[0] & 0xF0) == 0xE0 && size >= 3) {
        val = ((dec->p[0] & 0x0F) << 12) | ((dec->p[1] & 0x3F) << 6) | ((dec->p[2] & 0x3F));
        dec->p += 3;

    } else if ((dec->p[0] & 0xF8) == 0xF0 && size >= 4) {
        val = ((dec->p[0] & 0x07) << 18) | ((dec->p[1] & 0x3F) << 12) | ((dec->p[2] & 0x3F) << 6) | ((dec->p[3] & 0x3F));
        dec->p += 4;

    } else {
        raise_exception(env, dec->term);
    }

    *term = enif_make_uint(env, val);

    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_bytes(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t len;
    ErlNifSInt64 offset;
    ERL_NIF_TERM ret;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (dec->p + len <= dec->end) {
        offset = (ErlNifSInt64)(dec->p - (char *)(dec->bin.data));
        *term = enif_make_sub_binary(env, dec->term, (size_t)offset, len);
        dec->p += len;
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_string(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    char *end;
    uint32_t len;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM ret, r_term = enif_make_int(env, 0);

    if (!(state->opts.string_as_list)) {
        return unpack_bytes(env, dec, term);
    }

    check_ret(ret, do_unpack_uint32(env, dec, &len));
    end = dec->p + len;

    if (len == 0) {
        *term = enif_make_string(env, "", ERL_NIF_LATIN1);
        return RET_OK;
    }

    if (end > dec->end) {
        raise_exception(env, dec->term);
    }

    *term = enif_make_list(env, 0);
    if (!(state->opts.with_utf8)) {
        while (dec->p < end) {
            *term = enif_make_list_cell(env, enif_make_int(env, *(dec->p)++), *term);
        }

        enif_make_reverse_list(env, *term, term);
        return RET_OK;
    }

    while (dec->p < end) {
        check_ret(ret, unpack_utf8(env, dec, &r_term));
        *term = enif_make_list_cell(env, r_term, *term);
    }

    enif_make_reverse_list(env, *term, term);
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_enum(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term, ep_node_t *node)
{
    int32_t val;
    ERL_NIF_TERM ret;
    ep_enum_field_t *field;

    check_ret(ret, do_unpack_uint32(env, dec, (uint32_t *)&val));

    field = bsearch(&val, node->v_fields, node->v_size, sizeof(ep_enum_field_t), get_enum_compare_value);
    if (field == NULL) {
        *term = enif_make_int(env, val);
    } else {
        *term = field->name;
    }
    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_tag(ErlNifEnv *env, ep_dec_t *dec, uint32_t *tag, wire_type_e *wire_type)
{
    uint64_t val;
    ERL_NIF_TERM ret;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *wire_type = (wire_type_e)(val & 7);
    *tag = (uint32_t)(val >> 3);

    /*
     * Reject malformed tags early:
     * - field number 0 is invalid in protobuf wire format
     * - wire types 6 and 7 are reserved/invalid
     */
    if (*tag == 0 || *wire_type > WIRE_TYPE_32BIT) {
        enif_raise_exception(env, dec->term);
        return RET_ERROR;
    }
    return RET_OK;
}

static inline int
is_packed(ep_field_t *field, int wire_type)
{
    switch (field->type) {
    case field_sint32:
    case field_enum:
    case field_int32:
    case field_uint32:
    case field_sint64:
    case field_int64:
    case field_uint64:
    case field_bool:
    case field_fixed64:
    case field_sfixed64:
    case field_double:
    case field_fixed32:
    case field_sfixed32:
    case field_float:
        if (wire_type == WIRE_TYPE_LENGTH_PREFIXED) {
            return TRUE;
        }
    default:
        return FALSE;
    }
}

static ERL_NIF_TERM
unpack_element_packed(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term, ep_field_t *field)
{
    char *end_sentinel;
    uint32_t len;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM ret, head = state->atom_undefined;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (!enif_is_list(env, *term)) {
        *term = enif_make_list(env, 0);
    }

    if (len == 0) {
        return RET_OK;
    }

    end_sentinel = dec->p + len;
    while (dec->p < end_sentinel) {

        switch (field->type) {

        case field_sint32:
            check_ret(ret, unpack_sint32(env, dec, &head));
            break;

        case field_enum:
            check_ret(ret, unpack_enum(env, dec, &head, field->sub_node));
            break;

        case field_int32:
            check_ret(ret, unpack_int32(env, dec, &head));
            break;

        case field_uint32:
            check_ret(ret, unpack_uint32(env, dec, &head));
            break;

        case field_sint64:
            check_ret(ret, unpack_sint64(env, dec, &head));
            break;

        case field_int64:
            check_ret(ret, unpack_int64(env, dec, &head));
            break;

        case field_uint64:
            check_ret(ret, unpack_uint64(env, dec, &head));
            break;

        case field_fixed32:
            check_ret(ret, unpack_fixed32(env, dec, &head));
            break;

        case field_sfixed32:
            check_ret(ret, unpack_sfixed32(env, dec, &head));
            break;

        case field_float:
            check_ret(ret, unpack_float(env, dec, &head));
            break;

        case field_fixed64:
            check_ret(ret, unpack_fixed64(env, dec, &head));
            break;

        case field_sfixed64:
            check_ret(ret, unpack_sfixed64(env, dec, &head));
            break;

        case field_double:
            check_ret(ret, unpack_double(env, dec, &head));
            break;

        case field_bool:
            check_ret(ret, unpack_boolean(env, dec, &head));
            break;

        default:
            raise_exception(env, dec->term);
        }

        *term = enif_make_list_cell(env, head, *term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_field(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term, wire_type_e wire_type, ep_field_t *field)
{
    ERL_NIF_TERM ret;

    *term = 0;
    switch (field->type) {
    case field_sint32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sint32(env, dec, term));
        break;

    case field_enum:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_enum(env, dec, term, field->sub_node));
        break;

    case field_int32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_int32(env, dec, term));
        break;

    case field_uint32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_uint32(env, dec, term));
        break;

    case field_sint64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sint64(env, dec, term));
        break;

    case field_int64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_int64(env, dec, term));
        break;

    case field_uint64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_uint64(env, dec, term));
        break;

    case field_bool:
        if (wire_type != WIRE_TYPE_VARINT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_boolean(env, dec, term));
        break;

    case field_sfixed32:
        if (wire_type != WIRE_TYPE_32BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sfixed32(env, dec, term));
        break;

    case field_fixed32:
        if (wire_type != WIRE_TYPE_32BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_fixed32(env, dec, term));
        break;

    case field_float:
        if (wire_type != WIRE_TYPE_32BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_float(env, dec, term));
        break;

    case field_sfixed64:
        if (wire_type != WIRE_TYPE_64BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sfixed64(env, dec, term));
        break;

    case field_fixed64:
        if (wire_type != WIRE_TYPE_64BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_fixed64(env, dec, term));
        break;

    case field_double:
        if (wire_type != WIRE_TYPE_64BIT) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_double(env, dec, term));
        break;

    case field_string:
        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_string(env, dec, term));
        break;

    case field_bytes:
        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
            return skip_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_bytes(env, dec, term));
        break;

    default:
        raise_exception(env, dec->term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
skip_varint(ErlNifEnv *env, ep_dec_t *dec)
{
    uint32_t left = 10;
    char *p = dec->p;
    char *end = dec->end;

    while (left && p < end) {
        if ((((uint8_t)(*p++)) & 0x80) == 0) {
            dec->p = p;
            return RET_OK;
        }
        left--;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
skip_32bit(ErlNifEnv *env, ep_dec_t *dec)
{
    if (dec->p + 4 <= dec->end) {
        dec->p += 4;
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
skip_64bit(ErlNifEnv *env, ep_dec_t *dec)
{
    if (dec->p + 8 <= dec->end) {
        dec->p += 8;
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
skip_length_prefixed(ErlNifEnv *env, ep_dec_t *dec)
{
    uint32_t len;
    ERL_NIF_TERM ret;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (dec->p + len <= dec->end) {
        dec->p += len;
        return RET_OK;
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
skip_group(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum)
{
    uint32_t fnum;
    wire_type_e wire_type;
    ERL_NIF_TERM ret;

    while (dec->p < dec->end) {
        check_ret(ret, unpack_tag(env, dec, &fnum, &wire_type));
        if (wire_type == WIRE_TYPE_START_GROUP) {
            check_ret(ret, skip_group(env, dec, fnum));
        } else if (wire_type == WIRE_TYPE_END_GROUP && fnum == group_fnum) {
            return RET_OK;
        } else {
            check_ret(ret, skip_field(env, dec, wire_type));
        }
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
skip_field(ErlNifEnv *env, ep_dec_t *dec, wire_type_e wire_type)
{
    ERL_NIF_TERM ret;

    switch (wire_type) {
    case WIRE_TYPE_VARINT:
        check_ret(ret, skip_varint(env, dec));
        break;

    case WIRE_TYPE_64BIT:
        check_ret(ret, skip_64bit(env, dec));
        break;

    case WIRE_TYPE_LENGTH_PREFIXED:
        check_ret(ret, skip_length_prefixed(env, dec));
        break;

    case WIRE_TYPE_32BIT:
        check_ret(ret, skip_32bit(env, dec));
        break;

    default:
        raise_exception(env, dec->term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
skip_unknown_field(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, wire_type_e wire_type)
{
    if (wire_type == WIRE_TYPE_START_GROUP) {
        return skip_group(env, dec, fnum);
    }

    return skip_field(env, dec, wire_type);
}

static inline ep_field_t *
get_unknown_storage_field(ep_node_t *node, ep_state_t *state)
{
    uint32_t i;

    for (i = 0; i < node->size; i++) {
        ep_field_t *candidate = (ep_field_t *)node->fields + i;

        if (candidate->type == field_unknown && candidate->name == state->atom_d_unknown) {
            return candidate;
        }
    }

    return NULL;
}

static inline ERL_NIF_TERM
collect_unknown_varint(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    uint64_t val;
    ERL_NIF_TERM ret;

    check_ret(ret, unpack_varint_u64(env, dec, &val));
    *term = enif_make_tuple3(env, state->atom_varint, enif_make_uint(env, fnum), enif_make_int64(env, (int64_t)val));
    return RET_OK;
}

static inline ERL_NIF_TERM
collect_unknown_fixed64(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    uint64_t val;

    if (dec->p + 8 > dec->end) {
        raise_exception(env, dec->term);
    }
    memcpy(&val, dec->p, 8);
    dec->p += 8;
    *term = enif_make_tuple3(env, state->atom_fixed64, enif_make_uint(env, fnum), enif_make_uint64(env, val));
    return RET_OK;
}

static inline ERL_NIF_TERM
collect_unknown_fixed32(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    uint32_t val;

    if (dec->p + 4 > dec->end) {
        raise_exception(env, dec->term);
    }
    memcpy(&val, dec->p, 4);
    dec->p += 4;
    *term = enif_make_tuple3(env, state->atom_fixed32, enif_make_uint(env, fnum), enif_make_uint(env, val));
    return RET_OK;
}

static inline ERL_NIF_TERM
collect_unknown_length(ErlNifEnv *env, ep_dec_t *dec, uint32_t fnum, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    uint32_t len;
    ErlNifBinary bin;
    ERL_NIF_TERM ret;

    check_ret(ret, do_unpack_uint32(env, dec, &len));
    if (len > (uint32_t)(dec->end - dec->p)) {
        raise_exception(env, dec->term);
    }
    if (!enif_alloc_binary(len, &bin)) {
        raise_exception(env, dec->term);
    }
    if (len > 0) {
        memcpy(bin.data, dec->p, len);
    }
    dec->p += len;
    *term = enif_make_tuple3(env, state->atom_length_delimited, enif_make_uint(env, fnum), enif_make_binary(env, &bin));
    return RET_OK;
}

static inline ERL_NIF_TERM
collect_unknown_group_fields(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum, ERL_NIF_TERM *term)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    char *saved_end = dec->end;
    char *end_tag = NULL;
    ERL_NIF_TERM list = enif_make_list(env, 0);
    ERL_NIF_TERM item, ret;

    check_ret(ret, group_content_bounds(env, dec, group_fnum, &end_tag));
    dec->end = end_tag;

    while (dec->p < dec->end) {
        uint32_t fn;
        wire_type_e wt;

        check_ret(ret, collect_unknown_field(env, dec, &fn, &wt, &item));
        list = enif_make_list_cell(env, item, list);
    }

    dec->p = end_tag;
    dec->end = saved_end;
    check_ret(ret, consume_group_end(env, dec, group_fnum));
    enif_make_reverse_list(env, list, term);
    *term = enif_make_tuple3(env, state->atom_group, enif_make_uint(env, group_fnum), *term);
    return RET_OK;
}

static inline ERL_NIF_TERM
collect_unknown_field(ErlNifEnv *env, ep_dec_t *dec, uint32_t *fnum, wire_type_e *wire_type, ERL_NIF_TERM *term)
{
    ERL_NIF_TERM ret;

    check_ret(ret, unpack_tag(env, dec, fnum, wire_type));
    switch (*wire_type) {
    case WIRE_TYPE_VARINT:
        return collect_unknown_varint(env, dec, *fnum, term);
    case WIRE_TYPE_64BIT:
        return collect_unknown_fixed64(env, dec, *fnum, term);
    case WIRE_TYPE_LENGTH_PREFIXED:
        return collect_unknown_length(env, dec, *fnum, term);
    case WIRE_TYPE_START_GROUP:
        return collect_unknown_group_fields(env, dec, *fnum, term);
    case WIRE_TYPE_32BIT:
        return collect_unknown_fixed32(env, dec, *fnum, term);
    default:
        raise_exception(env, dec->term);
    }
}

static inline ERL_NIF_TERM
group_content_bounds(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum, char **end_tag)
{
    ep_dec_t scan;
    ERL_NIF_TERM ret;

    scan.p = dec->p;
    scan.end = dec->end;
    scan.term = dec->term;

    while (scan.p < scan.end) {
        char *tag_start = scan.p;
        uint32_t fn;
        wire_type_e wt;

        check_ret(ret, unpack_tag(env, &scan, &fn, &wt));
        if (wt == WIRE_TYPE_START_GROUP) {
            check_ret(ret, skip_group(env, &scan, fn));
        } else if (wt == WIRE_TYPE_END_GROUP && fn == group_fnum) {
            *end_tag = tag_start;
            return RET_OK;
        } else {
            check_ret(ret, skip_field(env, &scan, wt));
        }
    }

    raise_exception(env, dec->term);
}

static inline ERL_NIF_TERM
consume_group_end(ErlNifEnv *env, ep_dec_t *dec, uint32_t group_fnum)
{
    uint32_t fn;
    wire_type_e wt;
    ERL_NIF_TERM ret;

    check_ret(ret, unpack_tag(env, dec, &fn, &wt));
    if (wt != WIRE_TYPE_END_GROUP || fn != group_fnum) {
        raise_exception(env, dec->term);
    }
    return RET_OK;
}

static inline ERL_NIF_TERM
merge_group_values(ErlNifEnv *env, ERL_NIF_TERM prev, ERL_NIF_TERM newer, ep_node_t *group_node, ep_state_t *state)
{
    int prev_arity, new_arity, i;
    const ERL_NIF_TERM *prev_arr, *new_arr;
    ERL_NIF_TERM *merged;
    ERL_NIF_TERM result;

    if (!enif_get_tuple(env, prev, &prev_arity, &prev_arr) || !enif_get_tuple(env, newer, &new_arity, &new_arr) || prev_arity != new_arity || prev_arr[0] != new_arr[0]) {
        return newer;
    }

    merged = _calloc(sizeof(ERL_NIF_TERM), prev_arity);
    if (merged == NULL) {
        raise_exception(env, prev);
    }
    merged[0] = prev_arr[0];

    for (i = 1; i < prev_arity; i++) {
        ep_field_t *gf = (ep_field_t *)group_node->fields + (i - 1);

        if (gf->o_type == occurrence_repeated) {
            if (enif_is_list(env, new_arr[i])) {
                merged[i] = enif_make_list(env, 0);
                ERL_NIF_TERM head, tail = prev_arr[i];
                while (enif_get_list_cell(env, tail, &head, &tail)) {
                    merged[i] = enif_make_list_cell(env, head, merged[i]);
                }
                tail = new_arr[i];
                while (enif_get_list_cell(env, tail, &head, &tail)) {
                    merged[i] = enif_make_list_cell(env, head, merged[i]);
                }
                enif_make_reverse_list(env, merged[i], &merged[i]);
            } else {
                merged[i] = prev_arr[i];
            }
        } else if (new_arr[i] == state->atom_undefined) {
            merged[i] = prev_arr[i];
        } else if (gf->type == field_msg && prev_arr[i] != state->atom_undefined) {
            merged[i] = merge_group_values(env, prev_arr[i], new_arr[i], gf->sub_node, state);
        } else if (gf->type == field_group && prev_arr[i] != state->atom_undefined) {
            merged[i] = merge_group_values(env, prev_arr[i], new_arr[i], gf->sub_node, state);
        } else {
            merged[i] = new_arr[i];
        }
    }

    result = enif_make_tuple_from_array(env, merged, prev_arity);
    _free(merged);
    return result;
}

static inline ERL_NIF_TERM
begin_group_decode(ErlNifEnv *env, ep_dec_t *dec, ep_stack_t *stack, ep_spot_t **spot, ep_field_t *field, uint32_t fnum)
{
    char *end_tag;
    ERL_NIF_TERM ret;

    if (field->sub_node == NULL) {
        raise_exception(env, dec->term);
    }

    (*spot)->pos = field->rnum;
    (*spot)->field = field;
    (*spot)++;
    check_ret(ret, stack_ensure(env, stack, spot));
    (*spot)->saved_outer_end = dec->end;
    check_ret(ret, group_content_bounds(env, dec, fnum, &end_tag));
    dec->end = end_tag;
    (*spot)->end_sentinel = end_tag;
    (*spot)->group_field = field;
    (*spot)->field = field;
    (*spot)->node = field->sub_node;
    (*spot)->type = spot_tuple;
    (*spot)->pos = 0;
    (*spot)->t_used = field->sub_node->size + 1;
    (*spot)->end_sentinel = end_tag;
    return fill_default(env, *spot);
}

static inline ERL_NIF_TERM
assign_group_value(ErlNifEnv *env, ep_spot_t *spot, ep_field_t *field, ERL_NIF_TERM value, ep_state_t *state)
{
    ERL_NIF_TERM existing = spot->t_arr[field->rnum];

    if (field->o_type == occurrence_repeated) {
        spot->t_arr[field->rnum] = enif_make_list_cell(env, value, spot->t_arr[field->rnum]);
        return RET_OK;
    }

    if (existing != state->atom_undefined && field->sub_node != NULL) {
        spot->t_arr[field->rnum] = merge_group_values(env, existing, value, field->sub_node, state);
    } else {
        spot->t_arr[field->rnum] = value;
    }
    return RET_OK;
}

static inline ep_field_t *
get_field(ErlNifEnv *env, uint32_t fnum, ep_node_t *node)
{
    ep_fnum_field_t *ff;

    if (node->n_type == node_msg || node->n_type == node_map || node->n_type == node_group) {
        ff = bsearch(&fnum, node->v_fields, node->v_size, sizeof(ep_fnum_field_t), get_field_compare_fnum);
        if (ff != NULL) {
            return ff->field;
        }
    }
    return NULL;
}

static inline ERL_NIF_TERM
fill_default(ErlNifEnv *env, ep_spot_t *spot)
{
    ep_field_t *field;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM *t, *t_used_end;
    ErlNifBinary bin;

    t = spot->t_arr;
    *t++ = spot->node->name;
    t_used_end = spot->t_arr + spot->t_used;
    field = spot->node->fields;
    while (t < t_used_end) {
        if (field->o_type == occurrence_repeated) {
            *t++ = enif_make_list(env, 0);

        } else if (field->o_type == occurrence_required || (field->proto_v == 3 && field->o_type == occurrence_defaulty) || (spot->node->n_type == node_map)) {
            switch (field->type) {
            case field_int32:
            case field_int64:
            case field_uint32:
            case field_uint64:
            case field_sint32:
            case field_sint64:
            case field_fixed32:
            case field_fixed64:
            case field_sfixed32:
            case field_sfixed64:
                *t++ = enif_make_int(env, 0);
                break;

            case field_bool:
                *t++ = state->atom_false;
                break;

            case field_float:
            case field_double:
                *t++ = enif_make_double(env, 0.0);
                break;

            case field_string:
                *t++ = enif_make_string(env, "", ERL_NIF_LATIN1);
                break;

            case field_bytes: {
                if (!enif_alloc_binary(0, &bin)) {
                    return RET_ERROR;
                }
                *t++ = enif_make_binary(env, &bin);
                break;
            }

            case field_enum: {
                ep_enum_field_t *efield = field->sub_node->v_fields;
                int32_t target = 0;
                ep_enum_field_t *zfield = bsearch(&target, efield, field->sub_node->v_size, sizeof(ep_enum_field_t), get_enum_compare_value);
                if (zfield)
                    *t++ = zfield->name;
                else
                    *t++ = ((ep_enum_field_t *)field->sub_node->fields)->name;
                break;
            }

            default:
                *t++ = state->atom_undefined;
                break;
            }

        } else {
            *t++ = state->atom_undefined;
        }
        field++;
    }
    return RET_OK;
}

ERL_NIF_TERM
decode(ErlNifEnv *env, ep_tdata_t *tdata, ep_node_t *node)
{
    int32_t arity;
    uint32_t fnum, size, i, replaced = FALSE;
    ep_dec_t *dec;
    ep_spot_t *spot, *t_sp;
    ep_field_t *field = NULL, *map_field = NULL;
    ep_stack_t *stack;
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    wire_type_e wire_type;
    ERL_NIF_TERM *t, ret, head, tail, *t_used_end, term, *array;

    dec = &(tdata->dec);
    stack = &(tdata->stack);

    for (i = 0; i < stack->size; i++) {
        stack->spots[i].field = NULL;
        stack->spots[i].group_field = NULL;
        stack->spots[i].saved_outer_end = NULL;
        stack->spots[i].need_length = FALSE;
    }

    spot = stack->spots;

    spot->node = node;
    spot->t_used = spot->node->size + 1;
    check_ret(ret, fill_default(env, spot));

    spot->field = NULL;
    spot->group_field = NULL;
    spot->saved_outer_end = NULL;
    spot->type = spot_tuple;
    spot->pos = 0;
    spot->end_sentinel = dec->end;

    /*
     * Iterative decoder:
     * - stack spots track nested message/map contexts
     * - each length-delimited sub-message tightens dec->end with a sentinel
     * This avoids recursive calls and keeps allocations stable per scheduler.
     */
    dec->result = 0;
    while (spot >= stack->spots) {

        if (dec->p == dec->end) {
            if (spot->node->n_type != node_map) {
                t = spot->t_arr + 1;
                t_used_end = spot->t_arr + spot->t_used;
                field = (ep_field_t *)(spot->node->fields);

                while (t < t_used_end) {
                    if (field->o_type == occurrence_required && field->type != field_oneof && field->type != field_unknown && *t == state->atom_undefined) {
                        raise_exception(env, dec->term);
                    }

                    if (field->o_type == occurrence_repeated && enif_is_list(env, *t)) {
                        enif_make_reverse_list(env, *t, t);
                    }
                    t++;
                    field++;
                }
            }

            if (spot->group_field != NULL) {
                dec->end = spot->saved_outer_end;
                check_ret(ret, consume_group_end(env, dec, spot->group_field->fnum));
                spot->group_field = NULL;
                spot->saved_outer_end = NULL;
            }

            spot->pos = 0;
            if (spot->field && spot->field->is_oneof) {
                term = enif_make_tuple_from_array(env, spot->t_arr, (unsigned)(spot->t_used));
                term = enif_make_tuple2(env, spot->field->name, term);
            } else if (spot->node && spot->node->n_type == node_map) {
                term = enif_make_tuple_from_array(env, spot->t_arr + 1, (unsigned)(spot->t_used - 1));
            } else {
                term = enif_make_tuple_from_array(env, spot->t_arr, (unsigned)(spot->t_used));
            }
            spot->field = NULL;
            t_sp = spot;
            spot--;
            if (spot < stack->spots) {
                dec->result = term;
                break;
            }

            if (spot->field && spot->field->type == field_group) {
                check_ret(ret, assign_group_value(env, spot, spot->field, term, state));
            } else if (enif_is_list(env, spot->t_arr[spot->pos])) {
                if (spot->field && spot->field->type == field_map) {
                    map_field = (ep_field_t *)spot->field->sub_node->fields;
                    if (map_field[1].type == field_msg && t_sp->t_arr[2] == state->atom_undefined) {
                        raise_exception(env, spot->node->name);
                    }

                    head = spot->t_arr[spot->pos];
                    while (enif_get_list_cell(env, head, &head, &tail)) {
                        if (enif_get_tuple(env, head, &arity, to_const(array))) {
                            if (!enif_compare(array[0], t_sp->t_arr[1])) {
                                replaced = TRUE;
                                array[1] = t_sp->t_arr[2];
                                break;
                            }
                        }
                        head = tail;
                    }

                    if (replaced == TRUE) {
                        replaced = FALSE;
                    } else {
                        spot->t_arr[spot->pos] = enif_make_list_cell(env, term, spot->t_arr[spot->pos]);
                    }
                } else {
                    spot->t_arr[spot->pos] = enif_make_list_cell(env, term, spot->t_arr[spot->pos]);
                }

            } else {
                spot->t_arr[spot->pos] = term;
            }
            dec->end = spot->end_sentinel;
            continue;
        }

        if (spot->type == spot_tuple) {

            check_ret(ret, unpack_tag(env, dec, &fnum, &wire_type));
            field = get_field(env, fnum, spot->node);
            if (field == NULL) {
                ep_field_t *unknown_field = get_unknown_storage_field(spot->node, state);

                if (unknown_field != NULL) {
                    if (!enif_is_list(env, spot->t_arr[unknown_field->rnum])) {
                        spot->t_arr[unknown_field->rnum] = enif_make_list(env, 0);
                    }
                    switch (wire_type) {
                    case WIRE_TYPE_VARINT:
                        check_ret(ret, collect_unknown_varint(env, dec, fnum, &term));
                        break;
                    case WIRE_TYPE_64BIT:
                        check_ret(ret, collect_unknown_fixed64(env, dec, fnum, &term));
                        break;
                    case WIRE_TYPE_LENGTH_PREFIXED:
                        check_ret(ret, collect_unknown_length(env, dec, fnum, &term));
                        break;
                    case WIRE_TYPE_START_GROUP:
                        check_ret(ret, collect_unknown_group_fields(env, dec, fnum, &term));
                        break;
                    case WIRE_TYPE_32BIT:
                        check_ret(ret, collect_unknown_fixed32(env, dec, fnum, &term));
                        break;
                    default:
                        raise_exception(env, dec->term);
                    }
                    spot->t_arr[unknown_field->rnum] = enif_make_list_cell(env, term, spot->t_arr[unknown_field->rnum]);
                } else {
                    check_ret(ret, skip_unknown_field(env, dec, fnum, wire_type));
                }

            } else {
                if (field->o_type == occurrence_repeated && is_packed(field, wire_type)) {
                    check_ret(ret, unpack_element_packed(env, dec, &(spot->t_arr[field->rnum]), field));

                } else if (field->o_type == occurrence_repeated) {

                    if (!enif_is_list(env, spot->t_arr[field->rnum])) {
                        spot->t_arr[field->rnum] = enif_make_list(env, 0);
                    }

                    if (field->type == field_msg || field->type == field_map || field->type == field_group) {
                        if (field->type == field_group) {
                            if (wire_type != WIRE_TYPE_START_GROUP) {
                                raise_exception(env, dec->term);
                            }
                            check_ret(ret, begin_group_decode(env, dec, stack, &spot, field, fnum));
                        } else {
                            spot->pos = field->rnum;
                            if (spot->field && spot->field->is_oneof) {
                                spot++;
                                check_ret(ret, stack_ensure(env, stack, &spot));
                                spot->field = field;
                            } else {
                                spot->field = field;
                                spot++;
                                check_ret(ret, stack_ensure(env, stack, &spot));
                            }

                            spot->node = field->sub_node;
                            if (spot->node == NULL) {
                                raise_exception(env, dec->term);
                            }

                            spot->type = spot_tuple;

                            check_ret(ret, do_unpack_uint32(env, dec, &size));
                            if (size > (uint32_t)(dec->end - dec->p)) {
                                raise_exception(env, dec->term);
                            }
                            spot->end_sentinel = dec->p + size;
                            dec->end = spot->end_sentinel;

                            spot->t_used = spot->node->size + 1;
                            fill_default(env, spot);
                        }
                    } else {
                        check_ret(ret, unpack_field(env, dec, &head, wire_type, field));
                        if (head) {
                            spot->t_arr[field->rnum] = enif_make_list_cell(env, head, spot->t_arr[field->rnum]);
                        }
                    }

                } else {

                    if (field->type == field_group) {

                        if (wire_type != WIRE_TYPE_START_GROUP) {
                            raise_exception(env, dec->term);
                        }

                        check_ret(ret, begin_group_decode(env, dec, stack, &spot, field, fnum));

                    } else if (field->type == field_msg || field->type == field_map) {

                        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
                            raise_exception(env, dec->term);
                        }

                        t_sp = spot;
                        t_sp->pos = field->rnum;
                        spot++;
                        check_ret(ret, stack_ensure(env, stack, &spot));

                        spot->node = field->sub_node;
                        if (spot->node == NULL) {
                            raise_exception(env, dec->term);
                        }
                        spot->field = field;
                        spot->type = spot_tuple;

                        check_ret(ret, do_unpack_uint32(env, dec, &size));
                        if (size > (uint32_t)(dec->end - dec->p)) {
                            raise_exception(env, dec->term);
                        }
                        spot->end_sentinel = dec->p + size;
                        dec->end = spot->end_sentinel;

                        spot->t_used = spot->node->size + 1;
                        if (field->is_oneof && enif_is_tuple(env, t_sp->t_arr[t_sp->pos])) {
                            if (enif_get_tuple(env, t_sp->t_arr[t_sp->pos], &arity, to_const(array)) && field->name == array[0] && enif_get_tuple(env, array[1], &arity, to_const(array))) {

                                for (i = 0; i < (uint32_t)arity; i++) {
                                    spot->t_arr[i] = array[i];
                                }
                            }

                        } else {
                            fill_default(env, spot);
                        }

                    } else {
                        check_ret(ret, unpack_field(env, dec, &(term), wire_type, field));
                        if (term) {
                            if (field->is_oneof) {
                                spot->t_arr[field->rnum] = enif_make_tuple2(env, field->name, term);

                            } else {
                                spot->t_arr[field->rnum] = term;
                            }
                        }
                    }
                }
            }
        }
    }

    return RET_OK;
}

#if defined(EPB_UNIT_TEST)
static void
ep_unit_dec_init(ep_dec_t *dec, const unsigned char *buf, size_t len)
{
    memset(dec, 0, sizeof(*dec));
    dec->p = (char *)buf;
    dec->end = (char *)buf + len;
}

int
ep_unit_unpack_uint32(const unsigned char *buf, size_t len, uint32_t *out)
{
    ep_dec_t dec;

    ep_unit_dec_init(&dec, buf, len);
    if (do_unpack_uint32(NULL, &dec, out) != RET_OK) {
        return -1;
    }
    return 0;
}

int
ep_unit_unpack_sint32(const unsigned char *buf, size_t len, int32_t *out)
{
    uint32_t u;

    if (ep_unit_unpack_uint32(buf, len, &u) != 0) {
        return -1;
    }
    if (u & 1) {
        *out = -(int32_t)(u >> 1) - 1;
    } else {
        *out = (int32_t)(u >> 1);
    }
    return 0;
}

int
ep_unit_unpack_int32(const unsigned char *buf, size_t len, int32_t *out)
{
    ep_dec_t dec;
    uint32_t u;

    ep_unit_dec_init(&dec, buf, len);
    if (do_unpack_uint32(NULL, &dec, &u) != RET_OK) {
        return -1;
    }
    *out = (int32_t)u;
    return 0;
}

int
ep_unit_unpack_int64(const unsigned char *buf, size_t len, int64_t *out)
{
    ep_dec_t dec;
    int32_t shift = 0, left = 10;
    int64_t val = 0;

    ep_unit_dec_init(&dec, buf, len);
    while (left && dec.p < dec.end) {
        val |= ((uint64_t)(*(dec.p) & 0x7f) << shift);
        if ((*(dec.p)++ & 0x80) == 0) {
            *out = val;
            return 0;
        }
        shift += 7;
        left--;
    }
    return -1;
}

int
ep_unit_unpack_uint64(const unsigned char *buf, size_t len, uint64_t *out)
{
    ep_dec_t dec;
    uint32_t shift = 0, left = 10;
    uint64_t val = 0;

    ep_unit_dec_init(&dec, buf, len);
    while (left && dec.p < dec.end) {
        val |= ((uint64_t)(*(dec.p) & 0x7f) << shift);
        if ((*(dec.p)++ & 0x80) == 0) {
            *out = val;
            return 0;
        }
        shift += 7;
        left--;
    }
    return -1;
}

int
ep_unit_unpack_fixed32(const unsigned char *buf, size_t len, uint32_t *out)
{
    if (len < sizeof(uint32_t)) {
        return -1;
    }
    *out = *(const uint32_t *)buf;
    return 0;
}

int
ep_unit_unpack_fixed64(const unsigned char *buf, size_t len, uint64_t *out)
{
    if (len < sizeof(uint64_t)) {
        return -1;
    }
    *out = *(const uint64_t *)buf;
    return 0;
}

int
ep_unit_unpack_bool(const unsigned char *buf, size_t len, int *out)
{
    uint32_t u;

    if (ep_unit_unpack_uint32(buf, len, &u) != 0) {
        return -1;
    }
    *out = (int)u;
    return 0;
}

int
ep_unit_unpack_double(const unsigned char *buf, size_t len, double *out)
{
    if (len < sizeof(double)) {
        return -1;
    }
    *out = *(const double *)buf;
    return 0;
}
#endif
