#include "ep_test_schema.h"
#include "test.h"
#include <string.h>

static int
test_enc_buffer_grow(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM tail, msg;
    unsigned i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_grow", "f", 1, st->atom_int32,
                                          st->atom_repeated, NULL, 0) == RET_OK);

    tail = enif_make_list(env, 0);
    for (i = 0; i < 4096; i++) {
        tail = enif_make_list_cell(env, enif_make_int(env, (int)i), tail);
    }
    msg = enif_make_tuple2(env, enif_make_atom(env, "t_grow"), tail);
    TEST_ASSERT(ep_test_encode_only_small_buf(env, msg, 64) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_all_packed_scalar_types(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    struct {
        const char *name;
        ERL_NIF_TERM type;
        ERL_NIF_TERM val;
    } cases[] = {
        {"int32", 0, 0},
        {"uint32", 0, 0},
        {"int64", 0, 0},
        {"uint64", 0, 0},
        {"bool", 0, 0},
    };
    unsigned i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    cases[0].type = st->atom_int32;
    cases[0].val = enif_make_int(env, 3);
    cases[1].type = st->atom_uint32;
    cases[1].val = enif_make_uint(env, 4);
    cases[2].type = st->atom_int64;
    cases[2].val = enif_make_int64(env, 6);
    cases[3].type = st->atom_uint64;
    cases[3].val = enif_make_uint64(env, 7);
    cases[4].type = st->atom_bool;
    cases[4].val = st->atom_true;

    for (i = 0; i < sizeof(cases) / sizeof(cases[0]); i++) {
        char msg_name[16];

        sprintf(msg_name, "tp_%u", i);
        TEST_ASSERT(ep_test_load_packed_field(env, msg_name, "f", 1, cases[i].type) == RET_OK);
        TEST_ASSERT(ep_test_roundtrip(env,
                                      enif_make_tuple2(env, enif_make_atom(env, msg_name),
                                                       enif_make_list(env, 3, cases[i].val, cases[i].val, cases[i].val)),
                                      msg_name) == RET_OK);
    }

    ep_test_env_destroy(env);
    return 0;
}

static int
test_float_encode_from_int(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_fi", "f", 1, st->atom_float,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_fi"), enif_make_int(env, 2)),
                                  "t_fi") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_string_binary_decode_path(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    ep_test_set_opts(env, 0, 0);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_bin", "s", 1, st->atom_string,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_bin"),
                                                   enif_make_string(env, "raw-bytes", ERL_NIF_LATIN1)),
                                  "t_bin") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_parse_invalid_field_type(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ep_load_cache(env, enif_make_list(env, 1,
                                          enif_make_tuple2(env,
                                                           enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "bad")),
                                                           enif_make_list(env, 1,
                                                                          enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "x"),
                                                                                           enif_make_uint(env, 1), enif_make_uint(env, 1),
                                                                                           enif_make_int(env, 999), st->atom_optional,
                                                                                           enif_make_list(env, 0))))));
        TEST_ASSERT(0);
    }

    ep_test_env_destroy(env);
    return 0;
}

static int
test_nested_oneof_deep(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, enif_make_list(env, 4,
                                                ep_test_mk_msg_def(env, "ChildLink", enif_make_list(env, 1, ep_test_mk_field(env, st, "name", 2, 2, st->atom_bytes, st->atom_required))),
                                                ep_test_mk_msg_def(env, "FileChildren", enif_make_list(env, 1, ep_test_mk_field(env, st, "child_links", 1, 2, enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "ChildLink")), st->atom_repeated))),
                                                ep_test_mk_msg_def(env, "FuseResponse", enif_make_list(env, 1, enif_make_tuple4(env, make_atom(env, "gpb_oneof"), enif_make_atom(env, "fuse_response"), enif_make_uint(env, 2), enif_make_list2(env, ep_test_mk_field(env, st, "file_children", 3, 2, enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "FileChildren")), st->atom_optional), ep_test_mk_field(env, st, "xattr", 13, 2, st->atom_bytes, st->atom_optional))))),
                                                ep_test_mk_msg_def(env, "ServerMessage", enif_make_list(env, 1, enif_make_tuple4(env, make_atom(env, "gpb_oneof"), enif_make_atom(env, "message_body"), enif_make_uint(env, 2), enif_make_list(env, 1, ep_test_mk_field(env, st, "fuse_response", 15, 2, enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "FuseResponse")), st->atom_optional)))))));
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "ServerMessage"),
                                                   enif_make_tuple2(env, enif_make_atom(env, "fuse_response"),
                                                                    enif_make_tuple2(env, enif_make_atom(env, "FuseResponse"),
                                                                                     enif_make_tuple2(env, enif_make_atom(env, "file_children"),
                                                                                                      enif_make_tuple2(env, enif_make_atom(env, "FileChildren"),
                                                                                                                       enif_make_list(env, 1,
                                                                                                                                      enif_make_tuple2(env, enif_make_atom(env, "ChildLink"),
                                                                                                                                                       enif_make_string(env, "1", ERL_NIF_LATIN1)))))))),
                                  "ServerMessage") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_coverage_exhaustive(void)
{
    TEST_RUN(test_enc_buffer_grow);
    TEST_RUN(test_all_packed_scalar_types);
    TEST_RUN(test_float_encode_from_int);
    TEST_RUN(test_string_binary_decode_path);
    TEST_RUN(test_parse_invalid_field_type);
    TEST_RUN(test_nested_oneof_deep);
    return 0;
}
