#include "ep_test_schema.h"
#include "test.h"

static int
rt_scalar(ErlNifEnv *env, const char *msg, ERL_NIF_TERM val)
{
    return ep_test_roundtrip(env,
                             enif_make_tuple2(env, enif_make_atom(env, msg), val), msg);
}

static int
test_scalar_types(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

#define LOAD1(name, type) TEST_ASSERT(ep_test_load_single_field(env, name, "f", 1, type, st->atom_optional, NULL, 0) == RET_OK)

    LOAD1("t_int32", st->atom_int32);
    TEST_ASSERT(rt_scalar(env, "t_int32", enif_make_int(env, -7)) == RET_OK);

    LOAD1("t_int64", st->atom_int64);
    TEST_ASSERT(rt_scalar(env, "t_int64", enif_make_int64(env, -9000)) == RET_OK);

    LOAD1("t_uint32", st->atom_uint32);
    TEST_ASSERT(rt_scalar(env, "t_uint32", enif_make_uint(env, 42)) == RET_OK);

    LOAD1("t_uint64", st->atom_uint64);
    TEST_ASSERT(rt_scalar(env, "t_uint64", enif_make_uint64(env, 99)) == RET_OK);

    LOAD1("t_sint32", st->atom_sint32);
    TEST_ASSERT(rt_scalar(env, "t_sint32", enif_make_int(env, -15)) == RET_OK);

    LOAD1("t_sint64", st->atom_sint64);
    TEST_ASSERT(rt_scalar(env, "t_sint64", enif_make_int64(env, -33)) == RET_OK);

    LOAD1("t_fixed32", st->atom_fixed32);
    TEST_ASSERT(rt_scalar(env, "t_fixed32", enif_make_int(env, 0x11223344)) == RET_OK);

    LOAD1("t_fixed64", st->atom_fixed64);
    TEST_ASSERT(rt_scalar(env, "t_fixed64", enif_make_int64(env, 123456)) == RET_OK);

    LOAD1("t_sfixed32", st->atom_sfixed32);
    TEST_ASSERT(rt_scalar(env, "t_sfixed32", enif_make_int(env, -88)) == RET_OK);

    LOAD1("t_sfixed64", st->atom_sfixed64);
    TEST_ASSERT(rt_scalar(env, "t_sfixed64", enif_make_int64(env, -99)) == RET_OK);

    LOAD1("t_bool", st->atom_bool);
    TEST_ASSERT(rt_scalar(env, "t_bool", st->atom_true) == RET_OK);

    LOAD1("t_float", st->atom_float);
    TEST_ASSERT(rt_scalar(env, "t_float", enif_make_double(env, 1.25)) == RET_OK);

    LOAD1("t_sfixed64", st->atom_sfixed64);
    TEST_ASSERT(rt_scalar(env, "t_sfixed64", enif_make_int64(env, -123456789)) == RET_OK);

    LOAD1("t_double", st->atom_double);
    TEST_ASSERT(rt_scalar(env, "t_double", enif_make_double(env, 9.875)) == RET_OK);

    LOAD1("t_string", st->atom_string);
    TEST_ASSERT(rt_scalar(env, "t_string", enif_make_string(env, "text", ERL_NIF_LATIN1)) == RET_OK);

    LOAD1("t_bytes", st->atom_bytes);
    TEST_ASSERT(rt_scalar(env, "t_bytes", enif_make_string(env, "bin", ERL_NIF_LATIN1)) == RET_OK);

#undef LOAD1

    ep_test_env_destroy(env);
    return 0;
}

static int
test_enum_type(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_nested_msg(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_repeated_packed(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    TEST_ASSERT(ep_test_load_packed_field(env, "t_rep", "f", 1, st->atom_int32) == RET_OK);
    TEST_ASSERT(rt_scalar(env, "t_rep",
                          enif_make_list(env, 3, enif_make_int(env, 1), enif_make_int(env, 2), enif_make_int(env, 3))) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_codec_types(void)
{
    TEST_RUN(test_scalar_types);
    TEST_RUN(test_enum_type);
    TEST_RUN(test_nested_msg);
    TEST_RUN(test_repeated_packed);
    return 0;
}
