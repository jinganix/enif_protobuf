/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

static inline ERL_NIF_TERM
pass_field(ErlNifEnv *env, ep_dec_t *dec, wire_type_e wire_type);

static inline ERL_NIF_TERM
do_unpack_uint32(ErlNifEnv *env, ep_dec_t *dec, uint32_t *val)
{
    uint32_t    shift = 0, left = 10;
    uint64_t    tmp = 0;

    while (left && dec->p < dec->end) {

        tmp |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            *val = (uint32_t) tmp;
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    *val = 0;
    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_uint32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t    shift = 0, left = 10;
    uint64_t    val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            *term = enif_make_uint(env, (uint32_t) val);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sint32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int32_t     v;
    uint32_t    shift = 0, left = 10;
    uint64_t    val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            if (val & 1) {
                v = -(int32_t) (val >> 1) - 1;

            } else {
                v = (int32_t) (val >> 1);
            }

            *term = enif_make_int(env, v);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_int32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int32_t     shift = 0, left = 10;
    uint64_t    val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            *term = enif_make_int(env, (int) val);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_fixed32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t    val = 0;

    if (dec->p + sizeof(uint32_t) <= dec->end) {

        val = *(uint32_t *) (dec->p);
        *term = enif_make_uint(env, val);
        dec->p += sizeof(uint32_t);
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sfixed32(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int32_t     val = 0;

    if (dec->p + sizeof(int32_t) <= dec->end) {

        val = *(int32_t *) (dec->p);
        *term = enif_make_int(env, val);
        dec->p += sizeof(int32_t);
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_uint64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t    shift = 0, left = 10;
    uint64_t    val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            *term = enif_make_ulong(env, (ErlNifUInt64) val);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sint64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int64_t     v;
    uint32_t    shift = 0, left = 10;
    uint64_t    val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            if (val & 1) {
                v = -(int64_t) (val >> 1) - 1;

            } else {
                v = val >> 1;
            }

            *term = enif_make_long(env, (ErlNifSInt64) v);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_int64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int32_t     shift = 0, left = 10;
    int64_t     val = 0;

    while (left && dec->p < dec->end) {

        val |= ((uint64_t) (*(dec->p) & 0x7f) << shift);
        if ((*(dec->p)++ & 0x80) == 0) {

            *term = enif_make_long(env, (ErlNifSInt64) val);
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_fixed64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint64_t        val;

    if (dec->p + sizeof(uint64_t) <= dec->end) {

        val = *(uint64_t *) (dec->p);
        *term = enif_make_ulong(env, (ErlNifUInt64) val);
        dec->p += sizeof(uint64_t);
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_sfixed64(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    int64_t         val;

    if (dec->p + sizeof(int64_t) <= dec->end) {

        val = *(int64_t *) (dec->p);
        *term = enif_make_long(env, (ErlNifSInt64) val);
        dec->p += sizeof(int64_t);
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_boolean(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    if (dec->p + sizeof(int8_t) <= dec->end) {

        if (*(dec->p) == 1) {
            *term = state->atom_true;

        } else if (*(dec->p) == 0) {
            *term = state->atom_false;

        } else {
            return_error(env, dec->term);
        }

        (dec->p)++;
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_float(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    float           val;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    if (dec->p + sizeof(float) <= dec->end) {

        val = *(float *) (dec->p);

        if (val == INFINITY) {
            *term = state->atom_infinity;

        } else if (val == -INFINITY) {
            *term = state->atom_min_infinity;

        } else if (val != val) {
            *term = state->atom_nan;

        } else {
            *term = enif_make_double(env, (double) val);
        }

        dec->p += sizeof(float);
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_double(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    double          val;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    if (dec->p + sizeof(double) <= dec->end) {

        val = *(double *) (dec->p);

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

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_utf8(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t    size = (uint32_t) (dec->end - dec->p);
    uint32_t    val;

    if ((*(dec->p) & 0x80) == 0) {
        val = *(dec->p)++;

    } else if ((*(dec->p) & 0xE0) == 0xC0 && size >= 2) {
        val = ((dec->p[0] & 0x1F) << 6)
                              | ((dec->p[1] & 0x3F));
        dec->p += 2;

    } else if((dec->p[0] & 0xF0) == 0xE0 && size >= 3) {
        val =  ((dec->p[0] & 0x0F) << 12)
                                | ((dec->p[1] & 0x3F) << 6)
                                | ((dec->p[2] & 0x3F));
        dec->p += 3;

    } else if ((dec->p[0] & 0xF8) == 0xF0 && size >= 4) {
        val =  ((dec->p[0] & 0x07) << 18)
                                | ((dec->p[1] & 0x3F) << 12)
                                | ((dec->p[2] & 0x3F) << 6)
                                | ((dec->p[3] & 0x3F));
        dec->p += 4;

    } else {
        return_error(env, dec->term);
    }

    *term = enif_make_uint(env, val);

    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_bytes(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    uint32_t        len;
    ErlNifBinary    bin;
    ERL_NIF_TERM    ret;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (dec->p + len <= dec->end) {

        if (!enif_alloc_binary(len, &bin)) {
            return_error(env, dec->term);
        }
        memcpy(bin.data, dec->p, len);
        *term = enif_make_binary(env, &bin);
        dec->p += len;
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
unpack_string(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term)
{
    char           *end;
    uint32_t        len;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM    ret, r_term = state->integer_zero;

    if (!(state->opts.string_as_list)) {
        return unpack_bytes(env, dec, term);
    }

    check_ret(ret, do_unpack_uint32(env, dec, &len));
    end = dec->p + len;

    if (len == 0) {
        *term = state->nil;
        return RET_OK;
    }

    if (end > dec->end) {
        return_error(env, dec->term);
    }

    *term = state->nil;
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
    int32_t             val;
    ERL_NIF_TERM        ret;
    ep_enum_field_t    *field;

    check_ret(ret, do_unpack_uint32(env, dec, (uint32_t *) &val));

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
    uint32_t        shift = 4, left = 4;

    *wire_type = (*(dec->p) & 7);

    *tag = (*(dec->p) & 0x7f) >> 3;
    if ((*(dec->p)++ & 0x80) == 0) {
        return RET_OK;
    }

    while (left && dec->p < dec->end) {

        if (*(dec->p) & 0x80) {
            *tag |= (*(dec->p)++ & 0x7f) << shift;
            shift += 7;

        } else {
            *tag |= *(dec->p)++ << shift;
            return RET_OK;
        }
        left--;
    }

    return_error(env, dec->term);
}

static inline int
is_packed(ep_field_t *field, int wire_type) {
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
    char           *end_sentinel;
    uint32_t        len;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM    ret, head = state->atom_undefined;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (!enif_is_list(env, *term)) {
        *term = state->nil;
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
            return_error(env, dec->term);
        }

        *term = enif_make_list_cell(env, head, *term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
unpack_field(ErlNifEnv *env, ep_dec_t *dec, ERL_NIF_TERM *term, wire_type_e wire_type, ep_field_t *field)
{
    ERL_NIF_TERM    ret;

    *term = 0;
    switch (field->type) {
    case field_sint32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sint32(env, dec, term));
        break;

    case field_enum:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_enum(env, dec, term, field->sub_node));
        break;

    case field_int32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_int32(env, dec, term));
        break;

    case field_uint32:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_uint32(env, dec, term));
        break;

    case field_sint64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sint64(env, dec, term));
        break;

    case field_int64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_int64(env, dec, term));
        break;

    case field_uint64:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_uint64(env, dec, term));
        break;

    case field_bool:
        if (wire_type != WIRE_TYPE_VARINT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_boolean(env, dec, term));
        break;

    case field_sfixed32:
        if (wire_type != WIRE_TYPE_32BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sfixed32(env, dec, term));
        break;

    case field_fixed32:
        if (wire_type != WIRE_TYPE_32BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_fixed32(env, dec, term));
        break;

    case field_float:
        if (wire_type != WIRE_TYPE_32BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_float(env, dec, term));
        break;

    case field_sfixed64:
        if (wire_type != WIRE_TYPE_64BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_sfixed64(env, dec, term));
        break;

    case field_fixed64:
        if (wire_type != WIRE_TYPE_64BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_fixed64(env, dec, term));
        break;

    case field_double:
        if (wire_type != WIRE_TYPE_64BIT) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_double(env, dec, term));
        break;

    case field_string:
        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_string(env, dec, term));
        break;

    case field_bytes:
        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
            return pass_field(env, dec, wire_type);
        }
        check_ret(ret, unpack_bytes(env, dec, term));
        break;

    default:
        return_error(env, dec->term);
    }

    return RET_OK;
}

static inline ERL_NIF_TERM
pass_varint(ErlNifEnv *env, ep_dec_t *dec)
{
    uint32_t        shift = 0, left = 10;

    while (left && dec->p < dec->end) {

        if ((*(dec->p)++ & 0x80) == 0) {
            return RET_OK;
        }
        shift += 7;
        left--;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
pass_32bit(ErlNifEnv *env, ep_dec_t *dec)
{
    if (dec->p + 4 <= dec->end) {
        dec->p += 4;
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
pass_64bit(ErlNifEnv *env, ep_dec_t *dec)
{
    if (dec->p + 8 <= dec->end) {
        dec->p += 8;
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
pass_length_prefixed(ErlNifEnv *env, ep_dec_t *dec)
{
    uint32_t        len;
    ERL_NIF_TERM    ret;

    check_ret(ret, do_unpack_uint32(env, dec, &len));

    if (dec->p + len <= dec->end) {
        dec->p += len;
        return RET_OK;
    }

    return_error(env, dec->term);
}

static inline ERL_NIF_TERM
pass_field(ErlNifEnv *env, ep_dec_t *dec, wire_type_e wire_type)
{
    ERL_NIF_TERM        ret;

    switch (wire_type) {
    case WIRE_TYPE_VARINT:
        check_ret(ret, pass_varint(env, dec));
        break;

    case WIRE_TYPE_64BIT:
        check_ret(ret, pass_64bit(env, dec));
        break;

    case WIRE_TYPE_LENGTH_PREFIXED:
        check_ret(ret, pass_length_prefixed(env, dec));
        break;

    case WIRE_TYPE_32BIT:
        check_ret(ret, pass_32bit(env, dec));
        break;

    default:
        return_error(env, dec->term);
    }

    return RET_OK;
}

static inline ep_field_t *
get_field(ErlNifEnv *env, uint32_t fnum, ep_node_t *node)
{
    ep_fnum_field_t    *ff;

    if (node->n_type == node_msg) {

        ff = bsearch(&fnum, node->v_fields, node->v_size, sizeof(ep_fnum_field_t), get_field_compare_fnum);
        if (ff != NULL) {
            return ff->field;
        }

    } else if (node->n_type == node_map) {
        return bsearch(&fnum, node->fields, node->size, sizeof(ep_field_t), get_map_field_compare_fnum);
    }

    return NULL;
}

static inline void
fill_default(ErlNifEnv *env, ep_spot_t *spot)
{
    ep_field_t     *field;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM   *t, *t_used_end;

    t = spot->t_arr;
    *t++ = spot->node->name;
    t_used_end = spot->t_arr + spot->t_used;
    field = spot->node->fields;
    while (t < t_used_end) {

        if (field->defaut_value > 0) {
            *t++ = field->defaut_value;

        } else if (field->o_type == occurrence_repeated) {
            *t++ = state->nil;

        } else if (field->o_type == occurrence_required) {

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
            case field_enum:
                *t++ = state->integer_zero;
                break;

            case field_bool:
                *t++ = state->atom_false;
                break;

            case field_float:
            case field_double:
                *t++ = state->double_zero;
                break;

            case field_string:
            case field_bytes:
                *t++ = state->binary_nil;
                break;

            default:
                *t++ = state->atom_undefined;
                break;
            }

        } else if (field->type == field_enum) {
            ep_enum_field_t    *efield;
            efield = field->sub_node->v_fields;
            if (efield == NULL) {
                *t++ = state->integer_zero;
            } else {
                *t++ = efield->name;
            }
            // TODO(murali@): ensure this runs only for proto3.

        } else {
            *t++ = state->atom_undefined;
        }
        field++;
    }
}

ERL_NIF_TERM
decode(ErlNifEnv *env, ep_tdata_t *tdata, ep_node_t *node)
{
    int32_t         arity;
    uint32_t        fnum, size, i, replaced = FALSE;
    ep_dec_t       *dec;
    ep_spot_t      *spot, *t_sp;
    ep_field_t     *field = NULL;
    ep_stack_t     *stack;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    wire_type_e     wire_type;
    ERL_NIF_TERM   *t, ret, head, tail, *t_used_end, term, *array;

    dec = &(tdata->dec);
    stack = &(tdata->stack);

    spot = stack->spots;

    spot->node = node;
    spot->t_used = spot->node->size + 1;
    fill_default(env, spot);

    spot->field = NULL;
    spot->type = spot_tuple;
    spot->pos = 0;
    spot->end_sentinel = dec->end;

    dec->result = 0;
    while (spot >= stack->spots) {

        if (dec->p == dec->end) {

            if (spot->node->n_type != node_map) {
                t = spot->t_arr + 1;
                t_used_end = spot->t_arr + spot->t_used;
                field = (ep_field_t *) (spot->node->fields);
                while (t < t_used_end) {

                    if (field->o_type == occurrence_required
                            && field->type != field_oneof
                            && *t == state->atom_undefined) {
                        return_error(env, dec->term);
                    }

                    if (field->o_type == occurrence_repeated && enif_is_list(env, *t)) {
                        enif_make_reverse_list(env, *t, t);
                    }
                    t++;
                    field++;
                }
            }

            spot->pos = 0;
            if (spot->field && spot->field->is_oneof) {
                term = enif_make_tuple_from_array(env, spot->t_arr, (unsigned) (spot->t_used));

            } else {
                term = enif_make_tuple_from_array(env, spot->t_arr, (unsigned) (spot->t_used));
            }
            spot->field = NULL;
            t_sp = spot;
            spot--;
            if (spot < stack->spots) {

                dec->result = term;
                break;
            }

            if (enif_is_list(env, spot->t_arr[spot->pos])) {

                if (spot->field->type == field_map) {

                    head = spot->t_arr[spot->pos];
                    while (enif_get_list_cell(env, head, &head, &tail)) {

                        if (enif_get_tuple(env, head, &arity, to_const(array))) {

                            if (!enif_compare(array[0], t_sp->t_arr[0])) {
                                replaced = TRUE;
                                array[1] = t_sp->t_arr[1];
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
                check_ret(ret, pass_field(env, dec, wire_type));

            } else {
                if (field->o_type == occurrence_repeated && is_packed(field, wire_type)) {
                    check_ret(ret, unpack_element_packed(env, dec, &(spot->t_arr[field->rnum]), field));

                } else if (field->o_type == occurrence_repeated) {

                    if (!enif_is_list(env, spot->t_arr[field->rnum])) {
                        spot->t_arr[field->rnum] = state->nil;
                    }

                    if (field->type == field_msg || field->type == field_map) {
                        spot->pos = field->rnum;
                        if (spot->field && spot->field->is_oneof) {
                            spot++;
                            spot->field = field;
                        } else {
                            spot->field = field;
                            spot++;
                        }

                        spot->node = field->sub_node;
                        if (spot->node == NULL) {
                            return_error(env, dec->term);
                        }

                        spot->type = spot_tuple;

                        check_ret(ret, do_unpack_uint32(env, dec, &size));
                        spot->end_sentinel = dec->p + size;
                        dec->end = spot->end_sentinel;

                        if (field->type == field_msg) {
                            spot->t_used = spot->node->size + 1;
                            fill_default(env, spot);

                        } else {
                            spot->t_used = spot->node->size;
                        }

                    } else {

                        check_ret(ret, unpack_field(env, dec, &head, wire_type, field));
                        if (head) {
                            spot->t_arr[field->rnum] = enif_make_list_cell(env, head, spot->t_arr[field->rnum]);
                        }
                    }

                } else {

                    if (field->type == field_msg || field->type == field_map) {

                        if (wire_type != WIRE_TYPE_LENGTH_PREFIXED) {
                            return_error(env, dec->term);
                        }

                        t_sp = spot;
                        t_sp->pos = field->rnum;
                        spot++;

                        spot->node = field->sub_node;
                        if (spot->node == NULL) {
                            return_error(env, dec->term);
                        }
                        spot->field = field;
                        spot->type = spot_tuple;

                        check_ret(ret, do_unpack_uint32(env, dec, &size));
                        spot->end_sentinel = dec->p + size;
                        dec->end = spot->end_sentinel;

                        if (field->type == field_msg) {

                            spot->t_used = spot->node->size + 1;
                            if (field->is_oneof && enif_is_tuple(env, t_sp->t_arr[t_sp->pos])) {
                                if (enif_get_tuple(env, t_sp->t_arr[t_sp->pos], &arity, to_const(array))
                                        && field->name == array[0]
                                                                && enif_get_tuple(env, array[1], &arity, to_const(array))) {

                                    for (i = 0; i < (uint32_t) arity; i++) {
                                        spot->t_arr[i] = array[i];
                                    }
                                }

                            } else {
                                fill_default(env, spot);
                            }

                        } else {
                            spot->t_used = spot->node->size;
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
