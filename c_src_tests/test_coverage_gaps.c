#include "ep_test_schema.h"
#include "test.h"
#include <string.h>

static int
expect_load_cache_fails(ErlNifEnv *env, ERL_NIF_TERM cache)
{
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ep_load_cache(env, cache);
        return RET_ERROR;
    }
    return RET_OK;
}

static int
test_load_cache_errors(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);

    TEST_ASSERT(expect_load_cache_fails(env, enif_make_list(env, 0)) == RET_OK);
    TEST_ASSERT(expect_load_cache_fails(env,
                                        enif_make_list(env, 1,
                                                       enif_make_tuple2(env, make_atom(env, "syntax"),
                                                                        enif_make_string(env, "proto99", ERL_NIF_LATIN1)))) == RET_OK);
    TEST_ASSERT(expect_load_cache_fails(env,
                                        enif_make_list(env, 1,
                                                       enif_make_int(env, 1))) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_load_cache_proto3_msgs(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    cache = enif_make_list(env, 3,
                           enif_make_tuple2(env, make_atom(env, "syntax"),
                                            enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                           enif_make_tuple2(env, make_atom(env, "proto3_msgs"),
                                            enif_make_list(env, 1, enif_make_atom(env, "p3m"))),
                           enif_make_tuple2(env,
                                            enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "p3m")),
                                            enif_make_list(env, 1,
                                                           enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "x"),
                                                                            enif_make_uint(env, 1), enif_make_uint(env, 1), st->atom_int32,
                                                                            st->atom_optional, enif_make_list(env, 0)))));

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    ep_test_env_destroy(env);
    return 0;
}

static int
test_enum_allow_alias(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, enif_make_list(env, 1,
                                                enif_make_tuple2(env,
                                                                 enif_make_tuple2(env, make_atom(env, "enum"), enif_make_atom(env, "e2")),
                                                                 enif_make_list(env, 4,
                                                                                enif_make_tuple3(env, st->atom_option, make_atom(env, "allow_alias"),
                                                                                                 st->atom_true),
                                                                                enif_make_tuple2(env, enif_make_atom(env, "a"), enif_make_int(env, 1)),
                                                                                enif_make_tuple2(env, enif_make_atom(env, "b"), enif_make_int(env, 2)),
                                                                                enif_make_tuple2(env, enif_make_atom(env, "c"), enif_make_int(env, 2))))));
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    ep_test_env_destroy(env);
    return 0;
}

static int
test_enum_dup_without_alias_fails(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);

    TEST_ASSERT(expect_load_cache_fails(env, enif_make_list(env, 1,
                                                            enif_make_tuple2(env,
                                                                             enif_make_tuple2(env, make_atom(env, "enum"), enif_make_atom(env, "eb")),
                                                                             enif_make_list(env, 2,
                                                                                            enif_make_tuple2(env, enif_make_atom(env, "a"), enif_make_int(env, 1)),
                                                                                            enif_make_tuple2(env, enif_make_atom(env, "b"), enif_make_int(env, 1)))))) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_special_floats_decode(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    static const unsigned char wire_f[] = {0x0d, 0x00, 0x00, 0x80, 0x7f};
    static const unsigned char wire_d[] = {
        0x11, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xf8, 0x7f};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_f", "f", 1, st->atom_float,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "t_f", wire_f, sizeof(wire_f)) == RET_OK);
    TEST_ASSERT(ep_test_load_single_field(env, "t_d", "d", 1, st->atom_double,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "t_d", wire_d, sizeof(wire_d)) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_encode_double_special_atoms(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_d", "d", 1, st->atom_double,
                                          st->atom_optional, NULL, 0) == RET_OK);

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_d"), st->atom_infinity), "t_d") == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_d"), st->atom_min_infinity), "t_d") == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_d"), st->atom_nan), "t_d") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_utf8_edge_codepoints(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    ep_test_set_opts(env, 1, 0);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_u", "s", 1, st->atom_string,
                                          st->atom_optional, NULL, 0) == RET_OK);

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_u"),
                                                   enif_make_list(env, 1, enif_make_int(env, 0xFFFF))),
                                  "t_u") == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_u"),
                                                   enif_make_list(env, 1, enif_make_int(env, 0xFFFE))),
                                  "t_u") == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_u"),
                                                   enif_make_list(env, 1, enif_make_int(env, 0x10000))),
                                  "t_u") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_high_field_number_tag(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_hi", "f", 536870912u, st->atom_int32,
                                          st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_hi"), enif_make_int(env, 1)),
                                  "t_hi") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_negative_enum_roundtrip(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM msg, *elems;
    int arity;
    const ERL_NIF_TERM *arr;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);

    msg = ep_test_make_m1_message(env, st);
    TEST_ASSERT(enif_get_tuple(env, msg, &arity, &arr));
    elems = malloc(sizeof(ERL_NIF_TERM) * (size_t)arity);
    TEST_ASSERT(elems != NULL);
    memcpy(elems, arr, sizeof(ERL_NIF_TERM) * (size_t)arity);
    elems[16] = enif_make_atom(env, "v2");
    msg = enif_make_tuple_from_array(env, elems, (unsigned)arity);
    free(elems);

    TEST_ASSERT(ep_test_roundtrip(env, msg, "m1") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_non_packed_repeated_wire(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    unsigned char buf[16];
    size_t n = 0;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_nr", "f", 1, st->atom_int32,
                                          st->atom_repeated, NULL, 0) == RET_OK);

    buf[n++] = 0x08;
    buf[n++] = 0x01;
    buf[n++] = 0x08;
    buf[n++] = 0x02;
    buf[n++] = 0x08;
    buf[n++] = 0x03;

    TEST_ASSERT(ep_test_decode_wire(env, "t_nr", buf, n) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_person_required_fields(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_person_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env, ep_test_make_person_message(env), "Person") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_free_node_map_oneof(void)
{
    ep_node_t *node;
    ep_field_t *field;

    node = make_node(1, node_msg);
    TEST_ASSERT(node != NULL);
    field = node->fields;
    field->type = field_map;
    field->sub_node = make_node(2, node_map);
    TEST_ASSERT(field->sub_node != NULL);
    free_node(node);

    node = make_node(1, node_msg);
    TEST_ASSERT(node != NULL);
    field = node->fields;
    field->type = field_oneof;
    field->sub_node = make_node(1, node_oneof);
    TEST_ASSERT(field->sub_node != NULL);
    free_node(node);
    return 0;
}

static int
test_string_iolist_encode(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    ep_test_set_opts(env, 1, 0);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_io", "s", 1, st->atom_string,
                                          st->atom_optional, NULL, 0) == RET_OK);

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "t_io"),
                                                   enif_make_list2(env,
                                                                   enif_make_string(env, "ab", ERL_NIF_LATIN1),
                                                                   enif_make_list(env, 1, enif_make_int(env, 'c')))),
                                  "t_io") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_empty_bytes_proto3(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    cache = enif_make_list2(env,
                            enif_make_tuple2(env, make_atom(env, "syntax"),
                                             enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                            enif_make_tuple2(env,
                                             enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "pb")),
                                             enif_make_list(env, 1,
                                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "b"),
                                                                             enif_make_uint(env, 1), enif_make_uint(env, 1), st->atom_bytes,
                                                                             st->atom_defaulty, enif_make_list(env, 0)))));

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(ep_test_roundtrip(env,
                                  enif_make_tuple2(env, enif_make_atom(env, "pb"), enif_make_string(env, "", ERL_NIF_LATIN1)),
                                  "pb") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
load_cache_term(ErlNifEnv *env, ERL_NIF_TERM cache)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ERL_NIF_TERM ret;

    if (setjmp(ep_test_exception_jmp) != 0) {
        return RET_ERROR;
    }
    ep_test_exception_active = 0;
    ret = ep_load_cache(env, cache);
    return ret == st->atom_ok ? RET_OK : RET_ERROR;
}

static int
test_group_optional_and_repeated_roundtrip(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM g_field, h_field, msg_def, g_def, h_def, cache, msg;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    g_field = enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "g"),
                               enif_make_uint(env, 1), enif_make_uint(env, 2),
                               enif_make_tuple2(env, st->atom_group, enif_make_atom(env, "g")),
                               st->atom_optional, enif_make_list(env, 0));
    h_field = enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "h"),
                               enif_make_uint(env, 2), enif_make_uint(env, 3),
                               enif_make_tuple2(env, st->atom_group, enif_make_atom(env, "h")),
                               st->atom_repeated, enif_make_list(env, 0));

    msg_def = enif_make_tuple2(env,
                               enif_make_tuple2(env, st->atom_msg, enif_make_atom(env, "gm")),
                               enif_make_list(env, 2, g_field, h_field));
    g_def = enif_make_tuple2(env,
                             enif_make_tuple2(env, st->atom_group, enif_make_atom(env, "g")),
                             enif_make_list(env, 1,
                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "gf"),
                                                             enif_make_uint(env, 10), enif_make_uint(env, 2), st->atom_int32,
                                                             st->atom_optional, enif_make_list(env, 0))));
    h_def = enif_make_tuple2(env,
                             enif_make_tuple2(env, st->atom_group, enif_make_atom(env, "h")),
                             enif_make_list(env, 1,
                                            enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "hf"),
                                                             enif_make_uint(env, 11), enif_make_uint(env, 2), st->atom_int32,
                                                             st->atom_optional, enif_make_list(env, 0))));
    cache = enif_make_list(env, 3, msg_def, g_def, h_def);
    TEST_ASSERT(load_cache_term(env, cache) == RET_OK);

    msg = enif_make_tuple3(env, enif_make_atom(env, "gm"),
                           enif_make_tuple2(env, enif_make_atom(env, "g"), enif_make_int(env, 7)),
                           enif_make_list(env, 2,
                                          enif_make_tuple2(env, enif_make_atom(env, "h"), enif_make_int(env, 8)),
                                          enif_make_tuple2(env, enif_make_atom(env, "h"), enif_make_int(env, 9))));
    TEST_ASSERT(ep_test_roundtrip(env, msg, "gm") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_coverage_gaps(void)
{
    TEST_RUN(test_load_cache_errors);
    TEST_RUN(test_load_cache_proto3_msgs);
    TEST_RUN(test_enum_allow_alias);
    TEST_RUN(test_enum_dup_without_alias_fails);
    TEST_RUN(test_special_floats_decode);
    TEST_RUN(test_encode_double_special_atoms);
    TEST_RUN(test_utf8_edge_codepoints);
    TEST_RUN(test_high_field_number_tag);
    TEST_RUN(test_negative_enum_roundtrip);
    TEST_RUN(test_non_packed_repeated_wire);
    TEST_RUN(test_person_required_fields);
    TEST_RUN(test_free_node_map_oneof);
    TEST_RUN(test_string_iolist_encode);
    TEST_RUN(test_empty_bytes_proto3);
    TEST_RUN(test_group_optional_and_repeated_roundtrip);
    return 0;
}
