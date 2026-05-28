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
    tdata = ep_get_tdata(state);
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

static int
test_unpack_additional_scalars(void)
{
    unsigned char buf[32];
    uint32_t u32;
    int32_t s32;
    int64_t i64;
    uint64_t u64;
    int b;
    double d;

    buf[0] = 0x09;
    TEST_ASSERT(ep_unit_unpack_sint32(buf, 1, &s32) == 0);
    TEST_ASSERT_EQ(s32, -5);

    buf[0] = 0xD6;
    buf[1] = 0x01;
    TEST_ASSERT(ep_unit_unpack_int32(buf, 2, &s32) == 0);
    TEST_ASSERT_EQ(s32, 214);

    buf[0] = 0x96;
    buf[1] = 0x01;
    TEST_ASSERT(ep_unit_unpack_uint64(buf, 2, &u64) == 0);
    TEST_ASSERT_EQ((long)u64, 150);
    TEST_ASSERT(ep_unit_unpack_int64(buf, 2, &i64) == 0);
    TEST_ASSERT_EQ((long)i64, 150);

    buf[0] = 0x01;
    TEST_ASSERT(ep_unit_unpack_bool(buf, 1, &b) == 0);
    TEST_ASSERT_EQ(b, 1);

    {
        uint32_t v32 = 0x01020304u;
        memcpy(buf, &v32, sizeof(v32));
    }
    TEST_ASSERT(ep_unit_unpack_fixed32(buf, sizeof(uint32_t), &u32) == 0);
    TEST_ASSERT_EQ((long)u32, 0x01020304L);

    {
        uint64_t v64 = 0x0102030405060708ULL;
        memcpy(buf, &v64, sizeof(v64));
    }
    TEST_ASSERT(ep_unit_unpack_fixed64(buf, sizeof(uint64_t), &u64) == 0);
    TEST_ASSERT_EQ((long)(u64 & 0xffffffffULL), 0x05060708L);

    {
        double dv = 3.5;
        memcpy(buf, &dv, sizeof(dv));
    }
    TEST_ASSERT(ep_unit_unpack_double(buf, sizeof(double), &d) == 0);
    TEST_ASSERT(d > 3.49 && d < 3.51);

    return 0;
}

static int
test_unpack_failure_paths(void)
{
    unsigned char one[] = {0x80};
    unsigned char two[] = {0x80, 0x80};
    uint32_t u32;
    int32_t s32;
    int64_t i64;
    uint64_t u64;
    int b;
    double d;

    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        TEST_ASSERT(ep_unit_unpack_uint32(one, sizeof(one), &u32) != 0);
    }
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        TEST_ASSERT(ep_unit_unpack_sint32(one, sizeof(one), &s32) != 0);
    }
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        TEST_ASSERT(ep_unit_unpack_int32(one, sizeof(one), &s32) != 0);
    }
    if (setjmp(ep_test_exception_jmp) == 0) {
        ep_test_exception_active = 0;
        TEST_ASSERT(ep_unit_unpack_bool(one, sizeof(one), &b) != 0);
    }
    TEST_ASSERT(ep_unit_unpack_int64(two, sizeof(two), &i64) != 0);
    TEST_ASSERT(ep_unit_unpack_uint64(two, sizeof(two), &u64) != 0);
    TEST_ASSERT(ep_unit_unpack_fixed32(one, sizeof(one), &u32) != 0);
    TEST_ASSERT(ep_unit_unpack_fixed64(one, sizeof(one), &u64) != 0);
    TEST_ASSERT(ep_unit_unpack_double(one, sizeof(one), &d) != 0);
    return 0;
}

static int
test_decode_packed_more_types(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    unsigned char payload[16];
    unsigned char wire[32];
    size_t plen;
    float fv = 1.5f;
    double dv = 2.5;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    plen = ep_unit_pack_sint32(-3, payload, sizeof(payload));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_s32", "f", 1, st->atom_sint32) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)plen;
    memcpy(wire + 2, payload, plen);
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_s32", wire, plen + 2) == RET_OK);

    payload[0] = 0x05;
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_s64", "f", 1, st->atom_sint64) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = 1;
    wire[2] = payload[0];
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_s64", wire, 3) == RET_OK);

    plen = ep_unit_pack_fixed32(123, payload, sizeof(payload));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_f32", "f", 1, st->atom_fixed32) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)plen;
    memcpy(wire + 2, payload, plen);
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_f32", wire, plen + 2) == RET_OK);

    plen = ep_unit_pack_fixed64(456, payload, sizeof(payload));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_f64", "f", 1, st->atom_fixed64) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)plen;
    memcpy(wire + 2, payload, plen);
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_f64", wire, plen + 2) == RET_OK);

    plen = ep_unit_pack_fixed32(-7, payload, sizeof(payload));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_sf32", "f", 1, st->atom_sfixed32) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)plen;
    memcpy(wire + 2, payload, plen);
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_sf32", wire, plen + 2) == RET_OK);

    plen = ep_unit_pack_fixed64(-8, payload, sizeof(payload));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_sf64", "f", 1, st->atom_sfixed64) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)plen;
    memcpy(wire + 2, payload, plen);
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_sf64", wire, plen + 2) == RET_OK);

    memcpy(payload, &fv, sizeof(fv));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_float", "f", 1, st->atom_float) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)sizeof(fv);
    memcpy(wire + 2, payload, sizeof(fv));
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_float", wire, sizeof(fv) + 2) == RET_OK);

    memcpy(payload, &dv, sizeof(dv));
    TEST_ASSERT(ep_test_load_packed_field(env, "tpd_double", "f", 1, st->atom_double) == RET_OK);
    wire[0] = 0x0a;
    wire[1] = (unsigned char)sizeof(dv);
    memcpy(wire + 2, payload, sizeof(dv));
    TEST_ASSERT(ep_test_decode_wire(env, "tpd_double", wire, sizeof(dv) + 2) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_decode_wrong_wire_type_skips(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    unsigned char varint_wire[] = {0x08, 0x01};
    unsigned char len_wire[] = {0x0a, 0x00};

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_s32", "f", 1, st->atom_sint32, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_s32", len_wire, sizeof(len_wire)) == RET_OK);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_u32", "f", 1, st->atom_uint32, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_u32", len_wire, sizeof(len_wire)) == RET_OK);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_bool", "f", 1, st->atom_bool, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_bool", len_wire, sizeof(len_wire)) == RET_OK);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_f32", "f", 1, st->atom_fixed32, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_f32", varint_wire, sizeof(varint_wire)) == RET_OK);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_f64", "f", 1, st->atom_fixed64, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_f64", varint_wire, sizeof(varint_wire)) == RET_OK);

    TEST_ASSERT(ep_test_load_single_field(env, "tw_str", "f", 1, st->atom_string, st->atom_optional, NULL, 0) == RET_OK);
    TEST_ASSERT(ep_test_decode_wire(env, "tw_str", varint_wire, sizeof(varint_wire)) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_decode_collect_unknown_fields_manual_node(void)
{
    ErlNifEnv *env = ep_test_env_create();
    ep_state_t *st;
    ep_tdata_t *tdata;
    ep_node_t *node;
    ep_field_t *fields;
    ep_fnum_field_t *vf;
    unsigned char wire[] = {
        0x0a, 0x03, 'a', 'b', 'c',
        0x10, 0x02,
        0x19, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
        0x22, 0x02, 0x04, 0x04,
        0x2b, 0x50, 0x05, 0x2c,
        0x35, 0x06, 0x00, 0x00, 0x00};
    int arity;
    unsigned unknown_len = 0;
    const ERL_NIF_TERM *arr;

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    st = (ep_state_t *)enif_priv_data(env);
    tdata = ep_get_tdata(st);
    TEST_ASSERT(tdata->stack.spots != NULL);
    if (tdata->stack.spots[0].t_arr == NULL || tdata->stack.spots[0].t_size < 3) {
        if (tdata->stack.spots[0].t_arr != NULL) {
            _free(tdata->stack.spots[0].t_arr);
        }
        tdata->stack.spots[0].t_arr = _calloc(sizeof(ERL_NIF_TERM), 3);
        TEST_ASSERT(tdata->stack.spots[0].t_arr != NULL);
        tdata->stack.spots[0].t_size = 3;
    }

    node = make_node(2, node_msg);
    TEST_ASSERT(node != NULL);
    node->name = enif_make_atom(env, "msg");
    node->size = 2;
    node->n_type = node_msg;
    node->proto_v = 2;
    node->v_size = 1;
    node->v_fields = _calloc(sizeof(ep_fnum_field_t), 1);
    TEST_ASSERT(node->v_fields != NULL);

    fields = (ep_field_t *)node->fields;
    fields[0].name = enif_make_atom(env, "a");
    fields[0].fnum = 1;
    fields[0].rnum = 1;
    fields[0].type = field_string;
    fields[0].o_type = occurrence_optional;

    fields[1].name = st->atom_d_unknown;
    fields[1].fnum = 0;
    fields[1].rnum = 2;
    fields[1].type = field_unknown;
    fields[1].o_type = occurrence_repeated;

    vf = (ep_fnum_field_t *)node->v_fields;
    vf[0].fnum = 1;
    vf[0].field = &fields[0];

    TEST_ASSERT(enif_alloc_binary(sizeof(wire), &tdata->dec.bin));
    memcpy(tdata->dec.bin.data, wire, sizeof(wire));
    tdata->dec.p = (char *)tdata->dec.bin.data;
    tdata->dec.end = tdata->dec.p + sizeof(wire);
    tdata->dec.term = enif_make_binary(env, &tdata->dec.bin);
    TEST_ASSERT(decode(env, tdata, node) == RET_OK);

    TEST_ASSERT(enif_get_tuple(env, tdata->dec.result, &arity, &arr));
    TEST_ASSERT_EQ(arity, 3);
    TEST_ASSERT(enif_get_list_length(env, arr[2], &unknown_len));
    TEST_ASSERT_EQ((int)unknown_len, 5);

    free_node(node);
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
    TEST_RUN(test_unpack_additional_scalars);
    TEST_RUN(test_unpack_failure_paths);
    TEST_RUN(test_decode_packed_more_types);
    TEST_RUN(test_decode_wrong_wire_type_skips);
    TEST_RUN(test_decode_collect_unknown_fields_manual_node);
    return 0;
}
