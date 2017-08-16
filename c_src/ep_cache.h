/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef __EP_CACHE_H__
#define __EP_CACHE_H__

#include "enif_protobuf.h"

struct ep_cache_s {
    size_t          size;
    size_t          used;
    ep_node_id_t   *ids;
    ep_node_name_t *names;

    uint32_t        proto_v;
};

int
ep_cache_create(size_t size, ep_cache_t **cache);

void
ep_cache_destroy(ep_cache_t **cache);

int
ep_cache_insert(ep_node_t *node, ep_cache_t *cache);

void
ep_cache_sort(ep_cache_t *cache);

ep_node_t *
get_node_by_id(uint32_t id, ep_cache_t *cache);

ep_node_t *
get_node_by_name(ERL_NIF_TERM name, ep_cache_t *cache);

#endif
