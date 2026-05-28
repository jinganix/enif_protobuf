#include "../c_src/enif_protobuf.h"
#include "ep_test_schema.h"
#include "stub/erl_nif_stub.h"
#include "test.h"

static int
test_swap_uint32(void)
{
    TEST_ASSERT_EQ(swap_uint32(0x01020304), 0x04030201U);
    TEST_ASSERT_EQ(swap_uint32(0), 0);
    return 0;
}

static int
test_swap_uint64(void)
{
    TEST_ASSERT_EQ(swap_uint64(0x0102030405060708ULL), 0x0807060504030201ULL);
    return 0;
}

static int
test_pack_uint32_varint(void)
{
    unsigned char buf[16];
    size_t n;
    uint32_t out;

    n = ep_unit_pack_uint32(0, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 1);
    TEST_ASSERT_EQ(buf[0], 0);

    n = ep_unit_pack_uint32(127, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 1);
    TEST_ASSERT_EQ(buf[0], 127);

    n = ep_unit_pack_uint32(128, buf, sizeof(buf));
    TEST_ASSERT_EQ(n, 2);
    TEST_ASSERT_EQ(buf[0], 0x80);
    TEST_ASSERT_EQ(buf[1], 0x01);

    TEST_ASSERT(ep_unit_unpack_uint32(buf, n, &out) == 0);
    TEST_ASSERT_EQ(out, 128);
    return 0;
}

static int
test_encode_int32_field(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *state;
    ep_tdata_t *tdata;
    ERL_NIF_TERM msg, ret;
    ErlNifBinary bin;
    unsigned char expected[] = {0x08, 0x96, 0x01};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_build_int32_msg(env, "m1", 1) == RET_OK);

    state = (ep_state_t *)enif_priv_data(env);
    tdata = ep_get_tdata(state);
    tdata->enc.p = tdata->enc.mem;

    msg = enif_make_tuple2(env, enif_make_atom(env, "m1"), enif_make_int(env, 150));

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = encode(env, msg, tdata);
        TEST_ASSERT(ret == RET_OK);
    } else {
        TEST_ASSERT(0);
    }

    TEST_ASSERT(enif_inspect_binary(env, tdata->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, sizeof(expected));
    TEST_ASSERT(memcmp(bin.data, expected, bin.size) == 0);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_pack_more_scalar_helpers(void)
{
    unsigned char buf[32];
    size_t n;
    uint32_t u32;
    int32_t s32;
    int64_t i64;
    uint64_t u64;
    int b;
    double d;

    n = ep_unit_pack_sint32(-5, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_sint32(buf, n, &s32) == 0);
    TEST_ASSERT_EQ(s32, -5);

    n = ep_unit_pack_int32(-7, buf, sizeof(buf));
    TEST_ASSERT(n >= 10);
    TEST_ASSERT(ep_unit_unpack_int32(buf, n, &s32) == 0);
    TEST_ASSERT_EQ(s32, -7);

    n = ep_unit_pack_int64(-9, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_int64(buf, n, &i64) == 0);
    TEST_ASSERT_EQ((long)i64, -9);

    n = ep_unit_pack_uint64(1234, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_uint64(buf, n, &u64) == 0);
    TEST_ASSERT_EQ((long)u64, 1234);

    n = ep_unit_pack_fixed32(0x11223344, buf, sizeof(buf));
    TEST_ASSERT_EQ((long)n, (long)sizeof(uint32_t));
    TEST_ASSERT(ep_unit_unpack_fixed32(buf, n, &u32) == 0);
    TEST_ASSERT_EQ((long)u32, 0x11223344L);

    n = ep_unit_pack_fixed64(0x0102030405060708LL, buf, sizeof(buf));
    TEST_ASSERT_EQ((long)n, (long)sizeof(uint64_t));
    TEST_ASSERT(ep_unit_unpack_fixed64(buf, n, &u64) == 0);
    TEST_ASSERT_EQ((long)(u64 & 0xffffffffULL), 0x05060708L);

    n = ep_unit_pack_bool(1, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_bool(buf, n, &b) == 0);
    TEST_ASSERT_EQ(b, 1);

    n = ep_unit_pack_double(8.5, buf, sizeof(buf));
    TEST_ASSERT_EQ((long)n, (long)sizeof(double));
    TEST_ASSERT(ep_unit_unpack_double(buf, n, &d) == 0);
    TEST_ASSERT(d > 8.49 && d < 8.51);

    return 0;
}

int
run_test_ep_encoder(void)
{
    TEST_RUN(test_swap_uint32);
    TEST_RUN(test_swap_uint64);
    TEST_RUN(test_pack_uint32_varint);
    TEST_RUN(test_encode_int32_field);
    TEST_RUN(test_pack_more_scalar_helpers);
    return 0;
}
