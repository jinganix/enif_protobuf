#include "../c_src/enif_protobuf.h"
#include "ep_test_schema.h"
#include "test.h"

static int
test_make_free_msg_node(void)
{
    ep_node_t *node = make_node(2, node_msg);

    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->fields != NULL);
    TEST_ASSERT(node->n_type == 0 || node->n_type == node_msg);

    free_node(node);
    return 0;
}

static int
test_make_free_enum_node(void)
{
    ep_node_t *node = make_node(3, node_enum);

    TEST_ASSERT(node != NULL);
    TEST_ASSERT(node->fields != NULL);
    TEST_ASSERT(node->v_fields != NULL);

    free_node(node);
    return 0;
}

static int
test_field_compare_fnum(void)
{
    ep_fnum_field_t ff;
    int32_t key = 7;

    ff.fnum = 10;
    TEST_ASSERT(get_field_compare_fnum(&key, &ff) < 0);

    ff.fnum = 7;
    TEST_ASSERT_EQ(get_field_compare_fnum(&key, &ff), 0);

    ff.fnum = 3;
    TEST_ASSERT(get_field_compare_fnum(&key, &ff) > 0);
    return 0;
}

static int
test_enum_compare_value(void)
{
    ep_enum_field_t ef;
    int32_t key = 5;

    ef.value = 8;
    TEST_ASSERT(get_enum_compare_value(&key, &ef) < 0);

    ef.value = 5;
    TEST_ASSERT_EQ(get_enum_compare_value(&key, &ef), 0);
    return 0;
}

static int
test_parse_via_load_cache(void)
{
    ErlNifEnv *env = ep_test_env_create();

    TEST_ASSERT(env != NULL);
    TEST_ASSERT(ep_test_init_state(env, 1) == RET_OK);
    TEST_ASSERT(ep_test_load_m1_cache(env) == RET_OK);
    TEST_ASSERT(ep_test_load_person_cache(env) == RET_OK);

    ep_test_env_destroy(env);
    return 0;
}

static int
test_map_field_compare(void)
{
    ep_field_t f;
    int32_t key = 2;

    f.fnum = 3;
    TEST_ASSERT(get_map_field_compare_fnum(&key, &f) < 0);
    f.fnum = 2;
    TEST_ASSERT_EQ(get_map_field_compare_fnum(&key, &f), 0);
    return 0;
}

static int
test_enum_compare_name(void)
{
    ep_enum_field_t ef;
    ERL_NIF_TERM key = (ERL_NIF_TERM)50;

    ef.name = (ERL_NIF_TERM)100;
    TEST_ASSERT(get_enum_compare_name(&key, &ef) < 0);
    ef.name = key;
    TEST_ASSERT_EQ(get_enum_compare_name(&key, &ef), 0);
    return 0;
}

int
run_test_ep_node(void)
{
    TEST_RUN(test_make_free_msg_node);
    TEST_RUN(test_make_free_enum_node);
    TEST_RUN(test_field_compare_fnum);
    TEST_RUN(test_enum_compare_value);
    TEST_RUN(test_map_field_compare);
    TEST_RUN(test_enum_compare_name);
    TEST_RUN(test_parse_via_load_cache);
    return 0;
}
