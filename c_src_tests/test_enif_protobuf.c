#include "../c_src/enif_protobuf.h"
#include "stub/erl_nif_stub.h"
#include "test.h"

static int
test_make_atom_existing(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ERL_NIF_TERM a1, a2;

    TEST_ASSERT(env != NULL);
    a1 = make_atom(env, "ok");
    a2 = make_atom(env, "ok");
    TEST_ASSERT(a1 != 0);
    TEST_ASSERT(a1 == a2);
    TEST_ASSERT(enif_is_atom(env, a1));

    ep_test_env_destroy(env);
    return 0;
}

static int
test_make_atom_new(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ERL_NIF_TERM a, b;

    TEST_ASSERT(env != NULL);
    a = make_atom(env, "unique_atom_xyz_123");
    b = make_atom(env, "unique_atom_xyz_123");
    TEST_ASSERT(enif_is_atom(env, a));
    TEST_ASSERT(a == b);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_enif_protobuf(void)
{
    TEST_RUN(test_make_atom_existing);
    TEST_RUN(test_make_atom_new);
    return 0;
}
