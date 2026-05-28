#include "ep_test_schema.h"
#include "test.h"
#include <string.h>

static ERL_NIF_TERM
list_from_terms(ErlNifEnv *env, ERL_NIF_TERM *items, size_t n)
{
    ERL_NIF_TERM list = enif_make_list(env, 0);
    size_t i;

    for (i = n; i > 0; i--) {
        list = enif_make_list_cell(env, items[i - 1], list);
    }
    return list;
}

static ERL_NIF_TERM
field_opts(ErlNifEnv *env)
{
    return enif_make_list2(env,
                           make_atom(env, "packed"),
                           enif_make_tuple2(env, make_atom(env, "default"), make_atom(env, "ok")));
}

static ERL_NIF_TERM
mk_field_opts(ErlNifEnv *env, ep_state_t *st, const char *name, unsigned fnum,
              unsigned rnum, ERL_NIF_TERM type, ERL_NIF_TERM occ, ERL_NIF_TERM opts)
{
    return enif_make_tuple7(env, st->atom_field, enif_make_atom(env, name),
                            enif_make_uint(env, fnum), enif_make_uint(env, rnum), type, occ, opts);
}

static ERL_NIF_TERM
mk_field(ErlNifEnv *env, ep_state_t *st, const char *name, unsigned fnum,
         unsigned rnum, ERL_NIF_TERM type, ERL_NIF_TERM occ)
{
    return mk_field_opts(env, st, name, fnum, rnum, type, occ, field_opts(env));
}

static ERL_NIF_TERM
mk_msg_def(ErlNifEnv *env, ep_state_t *st, const char *name, ERL_NIF_TERM fields)
{
    return enif_make_tuple2(env,
                            enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, name)),
                            fields);
}

ERL_NIF_TERM
ep_test_mk_field(ErlNifEnv *env, ep_state_t *st, const char *name, unsigned fnum,
                 unsigned rnum, ERL_NIF_TERM type, ERL_NIF_TERM occ)
{
    return mk_field_opts(env, st, name, fnum, rnum, type, occ, enif_make_list(env, 0));
}

ERL_NIF_TERM
ep_test_mk_msg_def(ErlNifEnv *env, const char *name, ERL_NIF_TERM fields)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);

    return mk_msg_def(env, st, name, fields);
}

static ERL_NIF_TERM
mk_enum_def(ErlNifEnv *env, ep_state_t *st, const char *name, ERL_NIF_TERM values)
{
    return enif_make_tuple2(env,
                            enif_make_tuple2(env, st->atom_enum, enif_make_atom(env, name)),
                            values);
}

int
ep_test_load_m1_cache(ErlNifEnv *env)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM fields[22];
    ERL_NIF_TERM oneof_fields[2];
    ERL_NIF_TERM cache_list[4];
    ERL_NIF_TERM ret;

    oneof_fields[0] = mk_field(env, st, "int32", 22, 22, st->atom_int32, st->atom_optional);
    oneof_fields[1] = mk_field(env, st, "int64", 23, 22, st->atom_int64, st->atom_optional);

    fields[0] = mk_field(env, st, "int32", 1, 1, st->atom_int32, st->atom_optional);
    fields[1] = mk_field(env, st, "int64", 2, 2, st->atom_int64, st->atom_optional);
    fields[2] = mk_field(env, st, "uint32", 3, 3, st->atom_uint32, st->atom_optional);
    fields[3] = mk_field(env, st, "uint64", 4, 4, st->atom_uint64, st->atom_optional);
    fields[4] = mk_field(env, st, "sint32", 5, 5, st->atom_sint32, st->atom_optional);
    fields[5] = mk_field(env, st, "sint64", 6, 6, st->atom_sint64, st->atom_optional);
    fields[6] = mk_field(env, st, "fixed32", 7, 7, st->atom_fixed32, st->atom_optional);
    fields[7] = mk_field(env, st, "fixed64", 8, 8, st->atom_fixed64, st->atom_optional);
    fields[8] = mk_field(env, st, "sfixed32", 9, 9, st->atom_sfixed32, st->atom_optional);
    fields[9] = mk_field(env, st, "sfixed64", 10, 10, st->atom_sfixed64, st->atom_optional);
    fields[10] = mk_field(env, st, "bool", 11, 11, st->atom_bool, st->atom_optional);
    fields[11] = mk_field(env, st, "float", 12, 12, st->atom_float, st->atom_optional);
    fields[12] = mk_field(env, st, "double", 13, 13, st->atom_double, st->atom_optional);
    fields[13] = mk_field(env, st, "string", 14, 14, st->atom_string, st->atom_optional);
    fields[14] = mk_field(env, st, "bytes", 15, 15, st->atom_bytes, st->atom_optional);
    fields[15] = mk_field(env, st, "enum", 16, 16,
                          enif_make_tuple2(env, st->atom_enum, enif_make_atom(env, "e")), st->atom_optional);
    fields[16] = mk_field(env, st, "msg", 17, 17,
                          enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "m2")), st->atom_optional);
    fields[17] = mk_field_opts(env, st, "map", 18, 18,
                               enif_make_tuple3(env, st->atom_map, st->atom_string, st->atom_int32),
                               st->atom_repeated, enif_make_list(env, 0));
    fields[18] = mk_field(env, st, "required", 19, 19, st->atom_fixed32, st->atom_required);
    fields[19] = mk_field(env, st, "optional", 20, 20, st->atom_fixed32, st->atom_optional);
    fields[20] = mk_field(env, st, "repeated", 21, 21, st->atom_fixed32, st->atom_repeated);
    fields[21] = enif_make_tuple4(env, make_atom(env, "gpb_oneof"), enif_make_atom(env, "oneof"),
                                  enif_make_uint(env, 22), list_from_terms(env, oneof_fields, 2));

    cache_list[0] = mk_msg_def(env, st, "m1", list_from_terms(env, fields, 22));
    cache_list[1] = mk_msg_def(env, st, "m2", list_from_terms(env, (ERL_NIF_TERM[]){mk_field(env, st, "int32", 1, 1, st->atom_int32, st->atom_optional)}, 1));
    cache_list[2] = mk_msg_def(env, st, "m3", list_from_terms(env, (ERL_NIF_TERM[]){mk_field(env, st, "int32", 0, 1, st->atom_int32, st->atom_optional), mk_field(env, st, "int64", 1, 2, st->atom_int64, st->atom_optional)}, 2));
    cache_list[3] = mk_enum_def(env, st, "e", list_from_terms(env, (ERL_NIF_TERM[]){enif_make_tuple2(env, enif_make_atom(env, "v1"), enif_make_int(env, 100)), enif_make_tuple2(env, enif_make_atom(env, "v2"), enif_make_int(env, -2)), enif_make_tuple2(env, enif_make_atom(env, "v3"), enif_make_int(env, -2)), enif_make_tuple3(env, st->atom_option, make_atom(env, "allow_alias"), st->atom_true)}, 4));

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, list_from_terms(env, cache_list, 4));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_load_single_field(ErlNifEnv *env, const char *msg_name, const char *field_name,
                          unsigned fnum, ERL_NIF_TERM type, ERL_NIF_TERM occ, ERL_NIF_TERM *extra_defs, size_t extra_n)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM field, cache_list[8], ret;
    size_t n = 1;

    field = mk_field_opts(env, st, field_name, fnum, 1, type, occ, enif_make_list(env, 0));
    cache_list[0] = mk_msg_def(env, st, msg_name, list_from_terms(env, &field, 1));
    if (extra_defs != NULL) {
        for (size_t i = 0; i < extra_n && n < 8; i++) {
            cache_list[n++] = extra_defs[i];
        }
    }

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, list_from_terms(env, cache_list, n));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_load_oneof_cache(ErlNifEnv *env)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM oneof_fields[2];
    ERL_NIF_TERM fields[1];
    ERL_NIF_TERM ret;

    oneof_fields[0] = ep_test_mk_field(env, st, "a", 1, 1, st->atom_int32, st->atom_optional);
    oneof_fields[1] = ep_test_mk_field(env, st, "b", 2, 1, st->atom_string, st->atom_optional);
    fields[0] = enif_make_tuple4(env, make_atom(env, "gpb_oneof"), enif_make_atom(env, "u"),
                                 enif_make_uint(env, 1), enif_make_list2(env, oneof_fields[0], oneof_fields[1]));

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, enif_make_list(env, 1,
                                            ep_test_mk_msg_def(env, "t_oneof", enif_make_list(env, 1, fields[0]))));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_load_nested_cache(ErlNifEnv *env)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM inner, outer, ret;

    inner = ep_test_mk_field(env, st, "x", 1, 1, st->atom_int32, st->atom_optional);
    outer = ep_test_mk_field(env, st, "n", 1, 1,
                             enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "inner")), st->atom_optional);

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, enif_make_list2(env,
                                             ep_test_mk_msg_def(env, "inner", enif_make_list(env, 1, inner)),
                                             ep_test_mk_msg_def(env, "outer", enif_make_list(env, 1, outer))));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_load_enum_cache(ErlNifEnv *env)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM enum_def, field, ret;

    enum_def = enif_make_tuple2(env,
                                enif_make_tuple2(env, st->atom_enum, enif_make_atom(env, "colors")),
                                enif_make_list2(env,
                                                enif_make_tuple2(env, enif_make_atom(env, "red"), enif_make_int(env, 1)),
                                                enif_make_tuple2(env, enif_make_atom(env, "blue"), enif_make_int(env, 2))));
    field = ep_test_mk_field(env, st, "c", 1, 1,
                             enif_make_tuple2(env, st->atom_enum, enif_make_atom(env, "colors")), st->atom_optional);

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, enif_make_list2(env, enum_def,
                                             ep_test_mk_msg_def(env, "t_enum", enif_make_list(env, 1, field))));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_load_packed_field(ErlNifEnv *env, const char *msg_name, const char *field_name,
                          unsigned fnum, ERL_NIF_TERM type)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM field, cache_list[1], ret;

    field = mk_field(env, st, field_name, fnum, 1, type, st->atom_repeated);
    cache_list[0] = mk_msg_def(env, st, msg_name, list_from_terms(env, &field, 1));

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, list_from_terms(env, cache_list, 1));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

int
ep_test_encode_only(ErlNifEnv *env, ERL_NIF_TERM msg)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ep_tdata_t *tdata = ep_get_tdata(state);

    ep_test_prepare_decode_stack(env);
    tdata->enc.p = tdata->enc.mem;

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    return encode(env, msg, tdata);
}

int
ep_test_encode_only_small_buf(ErlNifEnv *env, ERL_NIF_TERM msg, size_t buf_size)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ep_tdata_t *tdata = ep_get_tdata(state);
    char *mem;

    if (buf_size < 32) {
        buf_size = 32;
    }
    mem = _realloc(tdata->enc.mem, buf_size);
    if (mem == NULL) {
        return RET_ERROR;
    }
    tdata->enc.mem = mem;
    tdata->enc.size = buf_size;
    tdata->enc.p = tdata->enc.mem;
    tdata->enc.end = tdata->enc.mem + buf_size;

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    return encode(env, msg, tdata);
}

int
ep_test_load_person_cache(ErlNifEnv *env)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM fields[3];
    ERL_NIF_TERM ret;

    fields[0] = mk_field(env, st, "name", 1, 2, st->atom_string, st->atom_required);
    fields[1] = mk_field(env, st, "id", 2, 3, st->atom_int32, st->atom_required);
    fields[2] = mk_field(env, st, "email", 3, 4, st->atom_string, st->atom_optional);

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, list_from_terms(env,
                                             (ERL_NIF_TERM[]){mk_msg_def(env, st, "Person", list_from_terms(env, fields, 3))}, 1));
    return (ret == st->atom_ok) ? RET_OK : RET_ERROR;
}

static ERL_NIF_TERM
m1_field_value(ErlNifEnv *env, ep_state_t *state, unsigned idx)
{
    switch (idx) {
    case 0:
        return enif_make_int(env, 1);
    case 1:
        return enif_make_int64(env, -2);
    case 2:
        return enif_make_uint(env, 3);
    case 3:
        return enif_make_uint64(env, 4);
    case 4:
        return enif_make_int(env, -5);
    case 5:
        return enif_make_int64(env, 6);
    case 6:
        return enif_make_int(env, 7);
    case 7:
        return enif_make_int64(env, 8);
    case 8:
        return enif_make_int(env, 9);
    case 9:
        return enif_make_int64(env, 10);
    case 10:
        return state->atom_true;
    case 11:
        return enif_make_double(env, 1.5);
    case 12:
        return enif_make_double(env, 2.5);
    case 13:
        return enif_make_string(env, "hello", ERL_NIF_LATIN1);
    case 14:
        return enif_make_string(env, "bytes", ERL_NIF_LATIN1);
    case 15:
        return enif_make_atom(env, "v1");
    case 16:
        return enif_make_tuple2(env, enif_make_atom(env, "m2"), enif_make_int(env, 7));
    case 17:
        return enif_make_list_cell(env,
                                   enif_make_tuple2(env, enif_make_string(env, "k1", ERL_NIF_LATIN1), enif_make_int(env, 42)),
                                   enif_make_list(env, 0));
    case 18:
        return enif_make_int(env, 99);
    case 19:
        return state->atom_undefined;
    case 20:
        return enif_make_list(env, 3, enif_make_int(env, 1), enif_make_int(env, 2), enif_make_int(env, 3));
    case 21:
        return enif_make_tuple2(env, enif_make_atom(env, "int32"), enif_make_int(env, 55));
    default:
        return state->atom_undefined;
    }
}

ERL_NIF_TERM
ep_test_make_m1_message(ErlNifEnv *env, ep_state_t *state)
{
    ERL_NIF_TERM elems[23];
    unsigned i;

    elems[0] = enif_make_atom(env, "m1");
    for (i = 0; i < 22; i++) {
        elems[i + 1] = m1_field_value(env, state, i);
    }
    return enif_make_tuple_from_array(env, elems, 23);
}

ERL_NIF_TERM
ep_test_make_m1_message_prefix(ErlNifEnv *env, ep_state_t *state, unsigned n_set)
{
    ERL_NIF_TERM elems[23];
    unsigned i;

    elems[0] = enif_make_atom(env, "m1");
    for (i = 0; i < 22; i++) {
        if (i < n_set || i == 18) {
            elems[i + 1] = m1_field_value(env, state, i);
        } else {
            elems[i + 1] = state->atom_undefined;
        }
    }
    return enif_make_tuple_from_array(env, elems, 23);
}

ERL_NIF_TERM
ep_test_make_m1_message_field(ErlNifEnv *env, ep_state_t *state, unsigned field_idx)
{
    ERL_NIF_TERM elems[23];
    unsigned i;

    elems[0] = enif_make_atom(env, "m1");
    for (i = 0; i < 22; i++) {
        if (i == field_idx || i == 18) {
            elems[i + 1] = m1_field_value(env, state, i);
        } else if (i == 17 || i == 20) {
            elems[i + 1] = enif_make_list(env, 0);
        } else {
            elems[i + 1] = state->atom_undefined;
        }
    }
    return enif_make_tuple_from_array(env, elems, 23);
}

ERL_NIF_TERM
ep_test_make_person_message(ErlNifEnv *env)
{
    return enif_make_tuple4(env, enif_make_atom(env, "Person"),
                            enif_make_string(env, "abc def", ERL_NIF_LATIN1),
                            enif_make_int(env, 345),
                            enif_make_string(env, "a@example.com", ERL_NIF_LATIN1));
}

int
ep_test_decode_wire(ErlNifEnv *env, const char *msg_name, const unsigned char *buf, size_t len)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ep_tdata_t *tdata = ep_get_tdata(state);
    ep_node_t *node;

    ep_test_prepare_decode_stack(env);
    node = get_node_by_name(enif_make_atom(env, msg_name), state->cache);
    if (node == NULL) {
        return RET_ERROR;
    }

    if (!enif_alloc_binary(len, &tdata->dec.bin)) {
        return RET_ERROR;
    }
    memcpy(tdata->dec.bin.data, buf, len);
    tdata->dec.p = (char *)tdata->dec.bin.data;
    tdata->dec.end = tdata->dec.p + len;
    tdata->dec.term = enif_make_binary(env, &tdata->dec.bin);

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    return decode(env, tdata, node) == RET_OK ? RET_OK : RET_ERROR;
}

int
ep_test_roundtrip(ErlNifEnv *env, ERL_NIF_TERM msg, const char *msg_name)
{
    ep_state_t *state = (ep_state_t *)enif_priv_data(env);
    ep_tdata_t *tdata = ep_get_tdata(state);
    ep_node_t *node;
    ErlNifBinary bin;

    ep_test_prepare_decode_stack(env);
    tdata->enc.p = tdata->enc.mem;

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;

    if (encode(env, msg, tdata) != RET_OK) {
        return RET_ERROR;
    }
    if (!enif_inspect_binary(env, tdata->enc.result, &bin)) {
        return RET_ERROR;
    }

    node = get_node_by_name(enif_make_atom(env, msg_name), state->cache);
    if (node == NULL) {
        return RET_ERROR;
    }

    if (!enif_alloc_binary(bin.size, &tdata->dec.bin)) {
        return RET_ERROR;
    }
    memcpy(tdata->dec.bin.data, bin.data, bin.size);
    tdata->dec.p = (char *)tdata->dec.bin.data;
    tdata->dec.end = tdata->dec.p + bin.size;
    tdata->dec.term = enif_make_binary(env, &tdata->dec.bin);

    if (decode(env, tdata, node) != RET_OK) {
        return RET_ERROR;
    }

    return RET_OK;
}
