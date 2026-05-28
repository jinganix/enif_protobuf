#include "../c_src/enif_protobuf.h"
#include "ep_test_schema.h"
#include "stub/erl_nif_stub.h"
#include "test.h"

static int
test_unpack_uint32_varint(void)
{
    unsigned char buf[] = {0x96, 0x01};
    uint32_t val;

    TEST_ASSERT(ep_unit_unpack_uint32(buf, sizeof(buf), &val) == 0);
    TEST_ASSERT_EQ(val, 150);
    return 0;
}

static int
test_unpack_truncated_varint(void)
{
    unsigned char buf[] = {0x80};
    uint32_t val;

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        TEST_ASSERT(ep_unit_unpack_uint32(buf, sizeof(buf), &val) != 0);
    } else {
        TEST_ASSERT(ep_test_exception_active);
    }
    return 0;
}

static int
test_decode_int32_field(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *state;
    ep_tdata_t *tdata;
    ep_node_t *node;
    ERL_NIF_TERM ret;
    int arity, decoded;
    const ERL_NIF_TERM *elems;
    unsigned char wire[] = {0x08, 0x96, 0x01};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_build_int32_msg(env, "m1", 1) == RET_OK);
    ep_test_prepare_decode_stack(env);

    state = (ep_state_t *)enif_priv_data(env);
    tdata = &state->tdata[0];
    node = get_node_by_name(enif_make_atom(env, "m1"), state->cache);

    TEST_ASSERT(enif_alloc_binary(sizeof(wire), &tdata->dec.bin));
    memcpy(tdata->dec.bin.data, wire, sizeof(wire));
    tdata->dec.p = (char *)tdata->dec.bin.data;
    tdata->dec.end = tdata->dec.p + sizeof(wire);
    tdata->dec.term = enif_make_binary(env, &tdata->dec.bin);

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = decode(env, tdata, node);
        TEST_ASSERT(ret == RET_OK);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(enif_get_tuple(env, tdata->dec.result, &arity, &elems));
    TEST_ASSERT_EQ(arity, 2);
    TEST_ASSERT(enif_is_atom(env, elems[0]));
    TEST_ASSERT(enif_get_int(env, elems[1], &decoded));
    TEST_ASSERT_EQ(decoded, 150);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_decode_nested_truncated_payload(void)
{
    ErlNifEnv *env = ep_test_env_create();
    unsigned char wire[] = {0x0a, 0x05, 0x08};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_nested_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "outer", wire, sizeof(wire)) == RET_ERROR);

    ep_test_env_destroy(env);
    return 0;
}

int
run_test_ep_decoder(void)
{
    TEST_RUN(test_unpack_uint32_varint);
    TEST_RUN(test_unpack_truncated_varint);
    TEST_RUN(test_decode_int32_field);
    TEST_RUN(test_decode_nested_truncated_payload);
    return 0;
}
