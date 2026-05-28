#include "ep_test_schema.h"
#include "test.h"

static int
test_person_only(void)
{
    ErlNifEnv      *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_person_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_roundtrip(env, ep_test_make_person_message(env), "Person") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_m1_load_and_encode(void)
{
    ErlNifEnv      *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_proto3_syntax(void)
{
    ErlNifEnv      *env = ep_test_env_create();
    ep_state_t     *st;
    ERL_NIF_TERM    cache, ret;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *) enif_priv_data(env);

    cache = enif_make_list2(env,
        enif_make_tuple2(env, make_atom(env, "syntax"), enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
        enif_make_tuple2(env,
            enif_make_tuple2(env, make_atom(env, "msg"), enif_make_atom(env, "p3m")),
            enif_make_list(env, 1,
                enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "x"),
                    enif_make_uint(env, 1), enif_make_uint(env, 1), st->atom_int32,
                    st->atom_defaulty, enif_make_list(env, 0)))));

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

int
run_test_integration(void)
{
    TEST_RUN(test_person_only);
    TEST_RUN(test_m1_load_and_encode);
    TEST_RUN(test_proto3_syntax);
    return 0;
}
