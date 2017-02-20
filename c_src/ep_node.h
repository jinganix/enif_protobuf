/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#ifndef EP_NODE_H_INCLUDED
#define EP_NODE_H_INCLUDED

#include "enif_protobuf.h"

struct enum_field_s {
    ERL_NIF_TERM    name;
    int32_t         value;
    uint32_t        proto_v;
};

struct field_s {
    ERL_NIF_TERM        name;
    occurrence_type_e   o_type;
    field_type_e        type;
    node_t             *sub_node;
    ERL_NIF_TERM        sub_name;
    ERL_NIF_TERM        defaut_value;
    uint32_t            id;
    uint32_t            fnum;
    uint32_t            rnum;
    uint32_t            proto_v;
    uint32_t            is_oneof;
    uint32_t            packed;
};

struct fnum_field_s {
    uint32_t        fnum;
    field_t        *field;
};

struct node_s {
    node_type_e     n_type;
    ERL_NIF_TERM    name;
    uint32_t        id;
    uint32_t        proto_v;
    uint32_t        size;
    uint32_t        v_size;
    void           *fields;
    void           *v_fields;
};

struct node_id_s {
    uint32_t        id;
    node_t         *node;
};

struct node_name_s {
    ERL_NIF_TERM    name;
    node_t         *node;
};

void
free_node(node_t *node);

ERL_NIF_TERM
parse_node(ErlNifEnv *env, ERL_NIF_TERM term, node_t **node, uint32_t proto_v, ERL_NIF_TERM syn_list);

ERL_NIF_TERM
prelink_nodes(ErlNifEnv *env, cache_t *cache);

void
stack_ensure_all(ErlNifEnv *env, cache_t *cache);

ERL_NIF_TERM
stack_ensure(ErlNifEnv *env, stack_t *stack, spot_t **spot);

int
get_field_compare_name(const void *a, const void *b);

int
get_map_field_compare_fnum(const void *a, const void *b);

int
get_field_compare_fnum(const void *a, const void *b);

int
get_enum_compare_name(const void *a, const void *b);

int
get_enum_compare_value(const void *a, const void *b);

#endif
