/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef ENIF_PROTO_CACHE_H_INCLUDED
#define ENIF_PROTO_CACHE_H_INCLUDED

#include "enif_protobuf.h"

struct cache_s {
    size_t          size;
    size_t          used;
    node_id_t      *ids;
    node_name_t    *names;

    uint32_t        proto_v;
};

int
cache_create(size_t size, cache_t **cache);

void
cache_destroy(cache_t **cache);

int
cache_insert(node_t *node, cache_t *cache);

void
cache_sort(cache_t *cache);

node_t *
get_node_by_id(uint32_t id, cache_t *cache);

node_t *
get_node_by_name(ERL_NIF_TERM name, cache_t *cache);

#endif
