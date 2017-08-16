/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

int
ep_cache_create(size_t size, ep_cache_t **cache)
{
    ep_cache_t     *ca;

    ca = _calloc(sizeof(ep_cache_t), 1);
    if (ca == NULL) {
        return RET_ERROR;
    }

    ca->size = size;
    ca->used = 0;
    ca->ids = _calloc(sizeof(ep_node_id_t) * size, 1);
    if (ca->ids == NULL) {
        _free(ca);
        return RET_ERROR;
    }

    ca->names = _calloc(sizeof(ep_node_name_t) * size, 1);
    if (ca->names == NULL) {
        _free(ca->ids);
        _free(ca);
        return RET_ERROR;
    }

    *cache = ca;
    return RET_OK;
}

void
ep_cache_destroy(ep_cache_t **cache)
{
    uint32_t        i;
    ep_cache_t     *ca = *cache;

	if (*cache == NULL) {
		return;
	}

    for (i = 0; i < ca->size; i++) {

        /*
         * Ids.node and Types.node point to the same node. Just free once.
         */
        if (ca->ids[i].node != NULL) {
            free_node(ca->ids[i].node);
        }
    }

    _free(ca->ids);
    _free(ca->names);
    _free(ca);

    *cache = NULL;
}

int
ep_cache_insert(ep_node_t *node, ep_cache_t *cache)
{
    if (cache->used >= cache->size) {
        return RET_ERROR;
    }

    cache->ids[cache->used].id = node->id;
    cache->ids[cache->used].node = node;

    cache->names[cache->used].name = node->name;
    cache->names[cache->used].node = node;

    cache->used++;

    return RET_OK;
}

static int
sort_compare_id(const void *a, const void *b)
{
    return (int) (((ep_node_id_t *) a)->id - ((ep_node_id_t *) b)->id);
}

static int
sort_compare_type(const void *a, const void *b)
{
    return (int) (((ep_node_name_t *) a)->name - ((ep_node_name_t *) b)->name);
}

void
ep_cache_sort(ep_cache_t *cache)
{
    qsort(cache->ids, cache->size, sizeof(ep_node_id_t), sort_compare_id);
    qsort(cache->names, cache->size, sizeof(ep_node_name_t), sort_compare_type);
}

static int
search_compare_id(const void *a, const void *b)
{
    return (int) (*((uint32_t *) a) - ((ep_node_id_t *) b)->id);
}

ep_node_t *
get_node_by_id(uint32_t id, ep_cache_t *cache)
{
    ep_node_id_t   *i_node;

    i_node = bsearch(&id, cache->ids, cache->size, sizeof(ep_node_id_t), search_compare_id);
    if (i_node == NULL) {
        return NULL;
    }

    return i_node->node;
}

static int
search_compare_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((ep_node_name_t *) b)->name);
}

ep_node_t *
get_node_by_name(ERL_NIF_TERM name, ep_cache_t *cache)
{
    ep_node_name_t *n_node;

    n_node = bsearch(&name, cache->names, cache->size, sizeof(ep_node_name_t), search_compare_name);
    if (n_node == NULL) {
        return NULL;
    }

    return n_node->node;
}
