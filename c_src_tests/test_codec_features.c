#include "ep_test_schema.h"
#include "test.h"

static int
test_m1_roundtrip(void)
{
    ErlNifEnv      *env = ep_test_env_create();
    ep_state_t     *st;
    unsigned        i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);
    st = (ep_state_t *) enif_priv_data(env);
    for (i = 0; i < 22; i++) {
        TEST_ASSERT(ep_test_roundtrip(env, ep_test_make_m1_message_field(env, st, i), "m1") == RET_OK);
    }
    TEST_ASSERT(ep_test_roundtrip(env, ep_test_make_m1_message(env, st), "m1") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_map_roundtrip(void)
{
    ErlNifEnv      *env = ep_test_env_create();
    ep_state_t     *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *) enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_map", "m", 1,
        enif_make_tuple3(env, st->atom_map, st->atom_string, st->atom_int32),
        st->atom_repeated, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "t_map"),
            enif_make_list(env, 1,
                enif_make_tuple2(env,
                    enif_make_string(env, "k", ERL_NIF_LATIN1),
                    enif_make_int(env, 42)))),
        "t_map") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_oneof_roundtrip(void)
{
    ErlNifEnv      *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_oneof_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "t_oneof"),
            enif_make_tuple2(env, enif_make_atom(env, "a"), enif_make_int(env, 7))),
        "t_oneof") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_nested_roundtrip(void)
{
    ErlNifEnv      *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_nested_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "outer"),
            enif_make_tuple2(env, enif_make_atom(env, "inner"), enif_make_int(env, 99))),
        "outer") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_enum_roundtrip(void)
{
    ErlNifEnv      *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_enum_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "t_enum"), enif_make_atom(env, "red")),
        "t_enum") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_repeated_strings(void)
{
    ErlNifEnv      *env = ep_test_env_create();
    ep_state_t     *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *) enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_rs", "s", 1, st->atom_string,
        st->atom_repeated, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "t_rs"),
            enif_make_list2(env,
                enif_make_string(env, "a", ERL_NIF_LATIN1),
                enif_make_string(env, "b", ERL_NIF_LATIN1))),
        "t_rs") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_nonpacked_repeated(void)
{
    ErlNifEnv      *env = ep_test_env_create();
    ep_state_t     *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *) enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "t_npr", "f", 1, st->atom_int32,
        st->atom_repeated, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env,
        enif_make_tuple2(env, enif_make_atom(env, "t_npr"),
            enif_make_list(env, 3, enif_make_int(env, 10), enif_make_int(env, 20), enif_make_int(env, 30))),
        "t_npr") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_codec_features(void)
{
    TEST_RUN(test_m1_roundtrip);
    TEST_RUN(test_map_roundtrip);
    TEST_RUN(test_oneof_roundtrip);
    TEST_RUN(test_nested_roundtrip);
    TEST_RUN(test_enum_roundtrip);
    TEST_RUN(test_repeated_strings);
    TEST_RUN(test_nonpacked_repeated);
    return 0;
}
