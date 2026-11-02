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

static int
test_encode_utf8_string_list_grows_buffer(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ep_tdata_t *tdata;
    ERL_NIF_TERM msg, list;
    ErlNifBinary bin;
    unsigned char expected[2 + 40 * 3];
    unsigned i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "u8", "s", 1, st->atom_string, st->atom_optional, NULL, 0) == RET_OK);
    ep_test_set_opts(env, 1, 0);

    list = enif_make_list(env, 0);
    for (i = 0; i < 40; i++) {
        list = enif_make_list_cell(env, enif_make_uint(env, 0x800), list);
    }
    msg = enif_make_tuple2(env, enif_make_atom(env, "u8"), list);

    expected[0] = 0x0a; /* field 1, length delimited */
    expected[1] = 40 * 3;
    for (i = 0; i < 40; i++) {
        expected[2 + i * 3] = 0xE0;
        expected[3 + i * 3] = 0xA0;
        expected[4 + i * 3] = 0x80;
    }

    /* 122 payload bytes do not fit the 32-byte buffer, so the encoder grows it
     * while it is still packing this string list. */
    TEST_ASSERT(ep_test_encode_only_small_buf(env, msg, 32) == RET_OK);
    tdata = ep_get_tdata(st);
    TEST_ASSERT(enif_inspect_binary(env, tdata->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, sizeof(expected));
    TEST_ASSERT(memcmp(bin.data, expected, sizeof(expected)) == 0);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_proto3_enum_skips_zero_value(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret, msg;
    ErlNifBinary bin;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    /* 'a' = 1 sorts before 'z' = 0, so fields[0].value is 1, not the default. */

    cache = enif_make_list(env, 3,
                           enif_make_tuple2(env, make_atom(env, "syntax"), enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                           enif_make_tuple2(env,
                                            enif_make_tuple2(env, make_atom(env, "enum"), enif_make_atom(env, "e")),
                                            /* Later created first so that, once sorted by
                                             * name, the non-zero value is fields[0]. */
                                            enif_make_list2(env, enif_make_tuple2(env, enif_make_atom(env, "a"), enif_make_int(env, 1)),
                                                            enif_make_tuple2(env, enif_make_atom(env, "z"), enif_make_int(env, 0)))),
                           ep_test_mk_msg_def(env, "pe",
                                              enif_make_list(env, 1,
                                                             enif_make_tuple7(env, st->atom_field, enif_make_atom(env, "f"),
                                                                              enif_make_uint(env, 1), enif_make_uint(env, 2),
                                                                              enif_make_tuple2(env, st->atom_enum, enif_make_atom(env, "e")),
                                                                              st->atom_defaulty, enif_make_list(env, 0)))));
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }

    /* value 1 must be encoded */
    msg = enif_make_tuple2(env, enif_make_atom(env, "pe"), enif_make_int(env, 1));
    TEST_ASSERT(ep_test_encode_only(env, msg) == RET_OK);
    TEST_ASSERT(enif_inspect_binary(env, ep_get_tdata(st)->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, 2);
    TEST_ASSERT_EQ(bin.data[0], 0x08);
    TEST_ASSERT_EQ(bin.data[1], 0x01);

    /* default value 0 must be omitted */
    msg = enif_make_tuple2(env, enif_make_atom(env, "pe"), enif_make_int(env, 0));
    TEST_ASSERT(ep_test_encode_only(env, msg) == RET_OK);
    TEST_ASSERT(enif_inspect_binary(env, ep_get_tdata(st)->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, 0);

    ep_test_env_destroy(env);
    return 0;
}

/* INT32_MIN zigzag is 0xFFFFFFFF; must not go through signed overflow. */ static int
test_sint32_min_roundtrip(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM msg;
    ErlNifBinary bin;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "s32", "f", 1, st->atom_sint32, st->atom_optional, NULL, 0) == RET_OK);

    msg = enif_make_tuple2(env, enif_make_atom(env, "s32"), enif_make_int(env, -2147483647 - 1));
    TEST_ASSERT(ep_test_encode_only(env, msg) == RET_OK);
    TEST_ASSERT(enif_inspect_binary(env, ep_get_tdata(st)->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, 6);
    TEST_ASSERT_EQ(bin.data[0], 0x08);
    TEST_ASSERT_EQ(bin.data[1], 0xff);
    TEST_ASSERT_EQ(bin.data[2], 0xff);
    TEST_ASSERT_EQ(bin.data[3], 0xff);
    TEST_ASSERT_EQ(bin.data[4], 0xff);
    TEST_ASSERT_EQ(bin.data[5], 0x0f);

    TEST_ASSERT(ep_test_roundtrip(env, msg, "s32") == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

/* Run encode() with an exactly-sized scratch buffer, so that reservations and
 * rewinds at the buffer limit are exercised deterministically. */
static int
encode_with_buf(ErlNifEnv *env, ERL_NIF_TERM msg, size_t size)
{
    ep_state_t *st = (ep_state_t *)enif_priv_data(env);
    ep_tdata_t *tdata = ep_get_tdata(st);
    char *mem = _calloc(size, 1);

    if (mem == NULL) {
        return RET_ERROR;
    }
    _free(tdata->enc.mem);
    tdata->enc.mem = mem;
    tdata->enc.size = size;
    tdata->enc.p = mem;
    tdata->enc.end = mem + size;
    tdata->enc.result = 0;

    return encode(env, msg, tdata);
}

/* A 10-byte unknown varint must grow the buffer: pack_tag only reserves room for
 * the tag itself. */
static int
test_unknown_varint_grows_buffer(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM msg, item;
    ErlNifBinary bin;
    unsigned i;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    TEST_ASSERT(ep_test_load_single_field(env, "uk", "$unknown", 1, st->atom_unknown, st->atom_repeated, NULL, 0) == RET_OK);

    item = enif_make_tuple3(env, st->atom_varint, enif_make_uint(env, 1), enif_make_int64(env, -1));
    msg = enif_make_tuple2(env, enif_make_atom(env, "uk"), enif_make_list(env, 1, item));

    /* exactly 10 bytes, so the 1-byte tag leaves 9 and the 10-byte varint must grow */
    TEST_ASSERT(encode_with_buf(env, msg, 10) == RET_OK);
    TEST_ASSERT(enif_inspect_binary(env, ep_get_tdata(st)->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, 11);
    TEST_ASSERT_EQ(bin.data[0], 0x08);
    for (i = 1; i < 10; i++) {
        TEST_ASSERT_EQ(bin.data[i], 0xff);
    }
    TEST_ASSERT_EQ(bin.data[10], 0x01);

    ep_test_env_destroy(env);
    return 0;
}

/* A proto3 default string that needs a buffer growth while packing must still be
 * omitted completely, not left behind as a bare tag. */
static int
test_proto3_empty_string_list_after_growth(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ERL_NIF_TERM cache, ret, msg, inner;
    ErlNifBinary bin;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    cache = enif_make_list(env, 3,
                           enif_make_tuple2(env, make_atom(env, "syntax"), enif_make_string(env, "proto3", ERL_NIF_LATIN1)),
                           enif_make_tuple2(env, make_atom(env, "proto3_msgs"), enif_make_list(env, 1, enif_make_atom(env, "p3s"))),
                           ep_test_mk_msg_def(env, "p3s",
                                              enif_make_list(env, 1,
                                                             ep_test_mk_field(env, st, "s", 1, 2, st->atom_string, st->atom_defaulty))));
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        ret = ep_load_cache(env, cache);
        TEST_ASSERT(ret == st->atom_ok);
    } else {
        TEST_ASSERT(0);
    }
    ep_test_set_opts(env, 1, 0);

    /* [[]] is a non-empty list whose iolist payload is empty: packing it reserves
     * a length slot (which grows the 10-byte buffer) and then writes nothing. */
    inner = enif_make_list(env, 1, enif_make_list(env, 0));
    msg = enif_make_tuple2(env, enif_make_atom(env, "p3s"), inner);

    TEST_ASSERT(encode_with_buf(env, msg, 10) == RET_OK);
    TEST_ASSERT(enif_inspect_binary(env, ep_get_tdata(st)->enc.result, &bin));
    TEST_ASSERT_EQ(bin.size, 0);

    ep_test_env_destroy(env);
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
    TEST_RUN(test_encode_utf8_string_list_grows_buffer);
    TEST_RUN(test_proto3_enum_skips_zero_value);
    TEST_RUN(test_sint32_min_roundtrip);
    TEST_RUN(test_unknown_varint_grows_buffer);
    TEST_RUN(test_proto3_empty_string_list_after_growth);
    return 0;
}
