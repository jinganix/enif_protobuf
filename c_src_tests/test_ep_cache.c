#include "../c_src/enif_protobuf.h"
#include "test.h"

static ep_node_t *
make_test_node(uint32_t id, ERL_NIF_TERM name)
{
    ep_node_t *node = make_node(0, node_msg);

    if (node == NULL) {
        return NULL;
    }
    node->id = id;
    node->name = name;
    node->n_type = node_msg;
    return node;
}

static int
test_cache_create_destroy(void)
{
    ep_cache_t *cache = NULL;

    TEST_ASSERT(ep_cache_create(4, &cache) == RET_OK);
    TEST_ASSERT(cache != NULL);
    TEST_ASSERT_EQ(cache->size, 4);
    TEST_ASSERT_EQ(cache->used, 0);

    ep_cache_destroy(&cache);
    TEST_ASSERT(cache == NULL);
    return 0;
}

static int
test_cache_insert_sort_lookup(void)
{
    ep_cache_t *cache = NULL;
    ep_node_t *n1, *n2, *found;
    ERL_NIF_TERM name1 = (ERL_NIF_TERM)100;
    ERL_NIF_TERM name2 = (ERL_NIF_TERM)200;

    TEST_ASSERT(ep_cache_create(4, &cache) == RET_OK);

    n1 = make_test_node(2, name2);
    n2 = make_test_node(1, name1);
    TEST_ASSERT(n1 != NULL && n2 != NULL);

    TEST_ASSERT(ep_cache_insert(n2, cache) == RET_OK);
    TEST_ASSERT(ep_cache_insert(n1, cache) == RET_OK);
    TEST_ASSERT_EQ(cache->used, 2);

    ep_cache_sort(cache);

    found = get_node_by_id(1, cache);
    TEST_ASSERT(found == n2);
    found = get_node_by_id(2, cache);
    TEST_ASSERT(found == n1);

    found = get_node_by_name(name1, cache);
    TEST_ASSERT(found == n2);
    found = get_node_by_name(name2, cache);
    TEST_ASSERT(found == n1);

    found = get_node_by_id(99, cache);
    TEST_ASSERT(found == NULL);

    ep_cache_destroy(&cache);
    return 0;
}

static int
test_cache_insert_full(void)
{
    ep_cache_t *cache = NULL;
    ep_node_t *node;

    TEST_ASSERT(ep_cache_create(1, &cache) == RET_OK);
    node = make_test_node(1, (ERL_NIF_TERM)1);
    TEST_ASSERT(ep_cache_insert(node, cache) == RET_OK);
    TEST_ASSERT(ep_cache_insert(node, cache) == RET_ERROR);
    ep_cache_destroy(&cache);
    return 0;
}

static int
test_cache_sort_extreme_keys(void)
{
    ep_cache_t *cache = NULL;
    ep_node_t *n_low, *n_high, *found;
    ERL_NIF_TERM name_low = (ERL_NIF_TERM)1;
    ERL_NIF_TERM name_high = (ERL_NIF_TERM)UINTPTR_MAX;

    TEST_ASSERT(ep_cache_create(4, &cache) == RET_OK);

    n_low = make_test_node(0, name_low);
    n_high = make_test_node(UINT32_MAX, name_high);
    TEST_ASSERT(n_low != NULL && n_high != NULL);

    TEST_ASSERT(ep_cache_insert(n_high, cache) == RET_OK);
    TEST_ASSERT(ep_cache_insert(n_low, cache) == RET_OK);

    ep_cache_sort(cache);

    found = get_node_by_id(0, cache);
    TEST_ASSERT(found == n_low);
    found = get_node_by_id(UINT32_MAX, cache);
    TEST_ASSERT(found == n_high);

    found = get_node_by_name(name_low, cache);
    TEST_ASSERT(found == n_low);
    found = get_node_by_name(name_high, cache);
    TEST_ASSERT(found == n_high);

    ep_cache_destroy(&cache);
    return 0;
}

int
run_test_ep_cache(void)
{
    TEST_RUN(test_cache_create_destroy);
    TEST_RUN(test_cache_insert_sort_lookup);
    TEST_RUN(test_cache_insert_full);
    TEST_RUN(test_cache_sort_extreme_keys);
    return 0;
}
