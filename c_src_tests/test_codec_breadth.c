#include "ep_test_schema.h"
#include "test.h"

static int
test_utf8_string_roundtrip(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    ep_test_set_opts(env, 1, 0);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_u8", "s", 1, st->atom_string,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_u8"),
                                                   enif_make_list(env, 4,
                                                                  enif_make_int(env, 'h'),
                                                                  enif_make_int(env, 'i'),
                                                                  enif_make_int(env, 0x80),
                                                                  enif_make_int(env, '!'))),
                                  "t_u8") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_string_as_list_decode(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ep_tdata_t *tdata;
    ErlNifBinary bin;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_sl", "s", 1, st->atom_string,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_sl"),
                                                   enif_make_string(env, "abcd", ERL_NIF_LATIN1)),
                                  "t_sl") == RET_OK);

    tdata = ep_get_tdata(st);
    TEST_ASSERT(enif_inspect_binary(env, tdata->enc.result, &bin));
    ep_test_set_opts(env, 0, 1);
    TEST_ASSERT(ep_test_decode_wire(env, "t_sl", bin.data, bin.size) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_enum_as_integer(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_enum_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_enum"), enif_make_int(env, 2)),
                                  "t_enum") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_oneof_string_branch(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_oneof_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_oneof"),
                                                   enif_make_tuple2(env, enif_make_atom(env, "b"),
                                                                    enif_make_string(env, "txt", ERL_NIF_LATIN1))),
                                  "t_oneof") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_map_wire_decode(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    static const unsigned char wire[] = {
        0x0a, 0x08, 0x0a, 0x01, 'x', 0x15, 0x11, 0x00, 0x00, 0x00,
        0x0a, 0x08, 0x0a, 0x01, 'y', 0x15, 0x12, 0x00, 0x00, 0x00};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_map", "m", 1,
                                          enif_make_tuple3(env, st->atom_map, st->atom_string, st->atom_fixed32),
                                          st->atom_repeated, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "t_map", wire, sizeof(wire)) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_skip_unknown_field(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ep_tdata_t *tdata;
    ErlNifBinary bin;
    unsigned char buf[64];
    size_t n;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_skip", "a", 1, st->atom_int32,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_skip"), enif_make_int(env, 5)),
                                  "t_skip") == RET_OK);

    tdata = ep_get_tdata(st);
    TEST_ASSERT(enif_inspect_binary(env, tdata->enc.result, &bin));
    TEST_ASSERT(bin.size + 3 <= sizeof(buf));
    memcpy(buf, bin.data, bin.size);
    n = bin.size;
    buf[n++] = (uint8_t)((99u << 3) | 0u);
    buf[n++] = 0x96;
    buf[n++] = 0x01;
    TEST_ASSERT(ep_test_decode_wire(env, "t_skip", buf, n) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_proto3_defaulty_roundtrip(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    cache = enif_make_list2(env,
                            enif_make_tuple2(env, make_atom(env, "syntax"), enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                            enif_make_tuple2(env,
                                             enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "p3")),
                                             enif_make_list(env, 2,
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "a"),
                                                                             enif_make_uint(env, 1), enif_make_uint(env, 1), st->atom_int32,
                                                                             st->atom_defaulty, enif_make_list(env, 0)),
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "b"),
                                                                             enif_make_uint(env, 2), enif_make_uint(env, 2), st->atom_bool,
                                                                             st->atom_defaulty, enif_make_list(env, 0)))));

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple3(env, enif_make_atom(env, "p3"), st->atom_undefined, st->atom_undefined),
                                  "p3") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_repeated_nested_messages(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM inner, outer, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    inner = ep_test_mk_field(env, st, "x", 1, 1, st->atom_int32, st->atom_optional);
    outer = ep_test_mk_field(env, st, "items", 1, 1,
                             enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "inner")), st->atom_repeated);

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, enif_make_list2(env,
                                                 ep_test_mk_msg_def(env, "inner", enif_make_list(env, 1, inner)),
                                                 ep_test_mk_msg_def(env, "outer", enif_make_list(env, 1, outer))));
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "outer"),
                                                   enif_make_list2(env,
                                                                   enif_make_tuple2(env, enif_make_atom(env, "inner"), enif_make_int(env, 1)),
                                                                   enif_make_tuple2(env, enif_make_atom(env, "inner"), enif_make_int(env, 2)))),
                                  "outer") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_proto3_zero_defaults_encode(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    cache = enif_make_list2(env,
                            enif_make_tuple2(env, make_atom(env, "syntax"), enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                            enif_make_tuple2(env,
                                             enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "pz")),
                                             enif_make_list(env, 5,
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "i32"),
                                                                             enif_make_uint(env, 1), enif_make_uint(env, 1), st->atom_int32,
                                                                             st->atom_defaulty, enif_make_list(env, 0)),
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "u32"),
                                                                             enif_make_uint(env, 2), enif_make_uint(env, 2), st->atom_uint32,
                                                                             st->atom_defaulty, enif_make_list(env, 0)),
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "b"),
                                                                             enif_make_uint(env, 3), enif_make_uint(env, 3), st->atom_bool,
                                                                             st->atom_defaulty, enif_make_list(env, 0)),
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "s"),
                                                                             enif_make_uint(env, 4), enif_make_uint(env, 4), st->atom_string,
                                                                             st->atom_defaulty, enif_make_list(env, 0)),
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "d"),
                                                                             enif_make_uint(env, 5), enif_make_uint(env, 5), st->atom_double,
                                                                             st->atom_defaulty, enif_make_list(env, 0)))));

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    {
        ERL_NIF_TERM zelems[6];

        zelems[0] = enif_make_atom(env, "pz");
        zelems[1] = enif_make_int(env, 0);
        zelems[2] = enif_make_uint(env, 0);
        zelems[3] = st->atom_false;
        zelems[4] = enif_make_string(env, "", ERL_NIF_LATIN1);
        zelems[5] = enif_make_double(env, 0.0);
        TEST_ASSERT(ep_test_encode_only(env, enif_make_tuple_from_array(env, zelems, 6)) == RET_OK);
    }

    ep_test_env_destroy(env);
    return 0;
}

static int
test_encode_large_repeated(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM list, tail, msg;
    unsigned i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_big", "f", 1, st->atom_int32,
                                          st->atom_repeated, NULL, 0) == RET_OK);

    tail = enif_make_list(env, 0);
    for (i = 0; i < 2048; i++) {
        tail = enif_make_list_cell(env, enif_make_int(env, (int)i), tail);
    }
    list = tail;
    msg = enif_make_tuple2(env, enif_make_atom(env, "t_big"), list);
    TEST_ASSERT(ep_test_encode_only(env, msg) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_reload_cache(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_person_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_roundtrip(env, ep_test_make_m1_message(env, st), "m1") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_codec_breadth(void)
{
    TEST_RUN(test_utf8_string_roundtrip);
    TEST_RUN(test_string_as_list_decode);
    TEST_RUN(test_enum_as_integer);
    TEST_RUN(test_oneof_string_branch);
    TEST_RUN(test_map_wire_decode);
    TEST_RUN(test_skip_unknown_field);
    TEST_RUN(test_proto3_defaulty_roundtrip);
    TEST_RUN(test_proto3_zero_defaults_encode);
    TEST_RUN(test_encode_large_repeated);
    TEST_RUN(test_repeated_nested_messages);
    TEST_RUN(test_reload_cache);
    return 0;
}
