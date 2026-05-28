#include "../c_src/enif_protobuf.h"
#include "test.h"

static int
rt_u32(uint32_t val)
{
    unsigned char   buf[32];
    size_t          n;
    uint32_t        out;

    n = ep_unit_pack_uint32(val, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_uint32(buf, n, &out) == 0);
    TEST_ASSERT_EQ(out, val);
    return 0;
}

static int
rt_s32(int32_t val)
{
    unsigned char   buf[32];
    size_t          n;
    int32_t         out;

    n = ep_unit_pack_sint32(val, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_sint32(buf, n, &out) == 0);
    TEST_ASSERT_EQ(out, val);
    return 0;
}

static int
rt_i32(int32_t val)
{
    unsigned char   buf[32];
    size_t          n;
    int32_t         out;

    n = ep_unit_pack_int32(val, buf, sizeof(buf));
    TEST_ASSERT(n > 0);
    TEST_ASSERT(ep_unit_unpack_int32(buf, n, &out) == 0);
    TEST_ASSERT_EQ(out, val);
    return 0;
}

static int
test_wire_roundtrips(void)
{
    TEST_ASSERT(rt_u32(300) == 0);
    TEST_ASSERT(rt_s32(-42) == 0);
    TEST_ASSERT(rt_i32(150) == 0);
    TEST_ASSERT(rt_i32(-1) == 0);

    {
        unsigned char   buf[32];
        size_t          n;
        int64_t         out;

        n = ep_unit_pack_int64(123456789, buf, sizeof(buf));
        TEST_ASSERT(n > 0);
        TEST_ASSERT(ep_unit_unpack_int64(buf, n, &out) == 0);
        TEST_ASSERT_EQ(out, 123456789);
    }
    {
        unsigned char   buf[32];
        size_t          n;
        uint64_t        out;

        n = ep_unit_pack_uint64(9876543210ULL, buf, sizeof(buf));
        TEST_ASSERT(n > 0);
        TEST_ASSERT(ep_unit_unpack_uint64(buf, n, &out) == 0);
        TEST_ASSERT_EQ(out, 9876543210ULL);
    }
    {
        unsigned char   buf[32];
        size_t          n;
        uint32_t        out;

        n = ep_unit_pack_fixed32(0x0A0B0C0D, buf, sizeof(buf));
        TEST_ASSERT_EQ(n, sizeof(uint32_t));
        TEST_ASSERT(ep_unit_unpack_fixed32(buf, n, &out) == 0);
        TEST_ASSERT_EQ(out, 0x0A0B0C0DU);
    }
    {
        unsigned char   buf[32];
        size_t          n;
        int             out;

        n = ep_unit_pack_bool(1, buf, sizeof(buf));
        TEST_ASSERT(n > 0);
        TEST_ASSERT(ep_unit_unpack_bool(buf, n, &out) == 0);
        TEST_ASSERT_EQ(out, 1);
    }
    {
        unsigned char   buf[32];
        size_t          n;
        double          out;

        n = ep_unit_pack_double(3.14159, buf, sizeof(buf));
        TEST_ASSERT_EQ(n, sizeof(double));
        TEST_ASSERT(ep_unit_unpack_double(buf, n, &out) == 0);
        TEST_ASSERT(out > 3.14158 && out < 3.14160);
    }
    {
        unsigned char   buf[32];
        size_t          n;
        uint64_t        out;

        n = ep_unit_pack_fixed64(0x0102030405060708LL, buf, sizeof(buf));
        TEST_ASSERT_EQ(n, sizeof(uint64_t));
        TEST_ASSERT(ep_unit_unpack_fixed64(buf, n, &out) == 0);
        TEST_ASSERT_EQ(out, 0x0102030405060708ULL);
    }
    return 0;
}

static int
test_swap_all(void)
{
    TEST_ASSERT_EQ(swap_int16(0x0102), 0x0201);
    TEST_ASSERT_EQ(swap_uint16(0x0102), 0x0201);
    TEST_ASSERT_EQ(swap_int32(0x01020304), 0x04030201);
    TEST_ASSERT_EQ(swap_int64(0x0102030405060708LL), 0x0807060504030201LL);
    return 0;
}

int
run_test_wire_codec(void)
{
    TEST_RUN(test_swap_all);
    TEST_RUN(test_wire_roundtrips);
    return 0;
}
