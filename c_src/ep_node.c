/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

#define A_ALLOW_ALIAS   "allow_alias"
#define A_DEFAULT       "default"
#define A_ONEOF         "gpb_oneof"
#define A_PACKED        "packed"
#define A_DEPRECATED    "deprecated"
#define A_EBIN          "ebin"

ERL_NIF_TERM
fill_msg_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ep_node_t *
make_node(int fields_n, node_type_e n_type);

ERL_NIF_TERM
link_sub_node(ErlNifEnv *env, ep_cache_t *cache, ep_field_t *field);

ERL_NIF_TERM
do_prelink_nodes(ErlNifEnv *env, ep_cache_t *cache, ep_node_t *node);

ERL_NIF_TERM
parse_enum_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node);

ERL_NIF_TERM
parse_field_basic(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_map_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_field_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_occurrence_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_opts(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_oneof_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node);

ERL_NIF_TERM
fill_oneof_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field);

ERL_NIF_TERM
parse_msg_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node);

ERL_NIF_TERM
stack_ensure_size(ErlNifEnv *env, ep_stack_t *stack, size_t size);

static int
sort_compare_sub_name_field(const void *a, const void *b)
{
    return (int) (((ep_field_t *) a)->sub_name - ((ep_field_t *) b)->sub_name);
}

static int
sort_compare_msg_field(const void *a, const void *b)
{
    return (int) (((ep_field_t *) a)->rnum - ((ep_field_t *) b)->rnum);
}

static int
sort_compare_fnum_field(const void *a, const void *b)
{
    return (int) (((ep_fnum_field_t *) a)->fnum - ((ep_fnum_field_t *) b)->fnum);
}

ep_node_t *
make_node(int fields_n, node_type_e n_type)
{
    ep_node_t      *node = _calloc(sizeof(ep_node_t), 1);

    if (node == NULL) {
        return NULL;
    }

    if (fields_n > 0) {
        if (n_type == node_msg || n_type == node_oneof) {
            node->fields = _calloc(sizeof(ep_field_t), fields_n);
            if (node->fields == NULL) {
                free_node(node);
                return NULL;
            }
        } else if (n_type == node_map) {
            node->fields = _calloc(sizeof(ep_field_t), fields_n);
            if (node->fields == NULL) {
                free_node(node);
                return NULL;
            }
        } else if (n_type == node_enum) {
            node->fields = _calloc(sizeof(ep_enum_field_t), fields_n);
            node->v_fields = _calloc(sizeof(ep_enum_field_t), fields_n);
            if (node->fields == NULL || node->v_fields == NULL) {
                free_node(node);
                return NULL;
            }
        }
    }

    return node;
}

void
free_node(ep_node_t *node)
{
    size_t          i;
    ep_field_t     *field;

    if (node->fields != NULL) {
        for (i = 0; i < node->size; i++) {
            if (node->n_type == node_msg || node->n_type == node_map) {
                field = &(((ep_field_t *) node->fields)[i]);
                if (field->type == field_map || field->type == field_oneof) {
                    if (field->sub_node != NULL) {
                        free_node(field->sub_node);
                        field->sub_node = NULL;
                    }
                }
            }
        }
        _free(node->fields);
        node->fields = NULL;
    }

    if (node->v_fields != NULL) {
        _free(node->v_fields);
        node->v_fields = NULL;
    }

    _free(node);
}

static int
sort_compare_enum_field(const void *a, const void *b)
{
    return (int) (((ep_enum_field_t *) a)->name - ((ep_enum_field_t *) b)->name);
}

static int
sort_compare_enum_v_field(const void *a, const void *b)
{
    return (int) (((ep_enum_field_t *) a)->value - ((ep_enum_field_t *) b)->value);
}

ERL_NIF_TERM
parse_enum_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node)
{
    int32_t             arity, allow_alias = 0,value;
    ep_state_t         *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM       *array;
    ERL_NIF_TERM        head, tail, tmp;
    ep_enum_field_t    *field, *v_field, *vf;

    tmp = term;
    while (enif_get_list_cell(env, tmp, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            raise_exception(env, term);
        }

        if (arity == 3) {
            if (array[0] == state->atom_option && array[1] == make_atom(env, A_ALLOW_ALIAS)) {
                if (array[2] == state->atom_true) {
                    allow_alias = 1;
                    break;
                } else if (array[2] == state->atom_false) {
                    allow_alias = 0;
                    break;
                } else {
                    raise_exception(env, head);
                }
            } else {
                raise_exception(env, head);
            }
        } else if (arity != 2) {
            raise_exception(env, head);
        }
        tmp = tail;
    }

    field = node->fields;
    v_field = node->v_fields;
    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            raise_exception(env, term);
        }

        if (arity == 2) {
            if (enif_is_atom(env, array[0]) && enif_get_int(env, array[1], &value)) {
                field->name = array[0];
                field->value = value;
            } else {
                raise_exception(env, term);
            }
        } else {
            term = tail;
            continue;
        }

        v_field->name = field->name;
        v_field->value = field->value;
        for (vf = node->v_fields; vf < v_field; vf++) {
            if (vf->value == v_field->value) {
                if (allow_alias) {
                    v_field--;
                    break;
                } else {
                    raise_exception(env, head);
                }
            }
        }
        field->proto_v = node->proto_v;
        field++;
        v_field++;

        term = tail;
    }

    qsort(node->fields, node->size, sizeof(ep_enum_field_t), sort_compare_enum_field);
    node->v_size = (uint32_t) (v_field - (ep_enum_field_t *) node->v_fields);
    qsort(node->v_fields, node->v_size, sizeof(ep_enum_field_t), sort_compare_enum_v_field);

    return RET_OK;
}

/*
 * msg
 */
ERL_NIF_TERM
parse_field_basic(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    int32_t         arity;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM   *array;

    if (term == state->atom_int32)          { field->type = field_int32; return RET_OK; }
    else if (term == state->atom_int64)     { field->type = field_int64; return RET_OK; }
    else if (term == state->atom_uint32)    { field->type = field_uint32; return RET_OK; }
    else if (term == state->atom_uint64)    { field->type = field_uint64; return RET_OK; }
    else if (term == state->atom_sint32)    { field->type = field_sint32; return RET_OK; }
    else if (term == state->atom_sint64)    { field->type = field_sint64; return RET_OK; }
    else if (term == state->atom_fixed32)   { field->type = field_fixed32; return RET_OK; }
    else if (term == state->atom_fixed64)   { field->type = field_fixed64; return RET_OK; }
    else if (term == state->atom_sfixed32)  { field->type = field_sfixed32; return RET_OK; }
    else if (term == state->atom_sfixed64)  { field->type = field_sfixed64; return RET_OK; }
    else if (term == state->atom_bool)      { field->type = field_bool; return RET_OK; }
    else if (term == state->atom_float)     { field->type = field_float; return RET_OK; }
    else if (term == state->atom_double)    { field->type = field_double; return RET_OK; }
    else if (term == state->atom_string)    { field->type = field_string; return RET_OK; }
    else if (term == state->atom_bytes)     { field->type = field_bytes; return RET_OK; }

    else if (enif_get_tuple(env, term, &arity, to_const(array))) {
        if (arity == 2 && array[0] == state->atom_enum && enif_is_atom(env, array[1])) {
            field->type = field_enum;
            field->sub_name = array[1];
            field->sub_node = NULL;
            return RET_OK;
        } else if (arity == 2 && array[0] == state->atom_msg && enif_is_atom(env, array[1])) {
            field->type = field_msg;
            field->sub_name = array[1];
            field->sub_node = NULL;
            return RET_OK;
        }
    }

    return RET_ERROR;
}

ERL_NIF_TERM
parse_map_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    int32_t         arity;
    ep_node_t      *node;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        raise_exception(env, term);
    }

    if (arity != 3 || array[0] != state->atom_map) {
        raise_exception(env, term);
    }

    field->type = field_map;
    field->sub_node = make_node(2, node_msg);
    if (field->sub_node == NULL) {
        raise_exception(env, state->atom_error);
    }

    node = field->sub_node;
    node->name = field->name;
    node->proto_v = field->proto_v;
    node->id = 0;
    node->size = 2;
    node->n_type = node_map;

    if (array[1] != state->atom_int32
        && array[1] != state->atom_int64
        && array[1] != state->atom_uint32
        && array[1] != state->atom_uint64
        && array[1] != state->atom_sint32
        && array[1] != state->atom_sint64
        && array[1] != state->atom_fixed32
        && array[1] != state->atom_fixed64
        && array[1] != state->atom_sfixed32
        && array[1] != state->atom_sfixed64
        && array[1] != state->atom_bool
        && array[1] != state->atom_double
        && array[1] != state->atom_string
    ) {
        raise_exception(env, array[1]);
    }

    parse_msg_fields(env, enif_make_list2(env,
        enif_make_tuple7(env,
            state->atom_field,
            make_atom(env, "k"),
            enif_make_uint(env, 1),
            enif_make_uint(env, 2),
            array[1],
            state->atom_optional,
            enif_make_list(env, 0)
        ),
        enif_make_tuple7(env,
            state->atom_field,
            make_atom(env, "v"),
            enif_make_uint(env, 2),
            enif_make_uint(env, 3),
            array[2],
            state->atom_optional,
            enif_make_list(env, 0)
        )
    ), node);
    return RET_OK;
}

ERL_NIF_TERM
parse_field_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    if (parse_field_basic(env, term, field) == RET_OK) {
        return RET_OK;
    }

    if (parse_map_type(env, term, field) == RET_OK) {
        return RET_OK;
    }

    return RET_ERROR;
}

ERL_NIF_TERM
parse_occurrence_type(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    ep_state_t *state = (ep_state_t *) enif_priv_data(env);

    if (term == state->atom_required)       { field->o_type = occurrence_required; return RET_OK; }
    else if (term == state->atom_optional)  { field->o_type = occurrence_optional; return RET_OK; }
    else if (term == state->atom_defaulty)  { field->o_type = occurrence_defaulty; return RET_OK; }
    else if (term == state->atom_repeated)  { field->o_type = occurrence_repeated; return RET_OK; }

    return RET_ERROR;
}

ERL_NIF_TERM
parse_opts(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    int32_t         arity;
    ERL_NIF_TERM   *array;
    ERL_NIF_TERM    head, tail;

    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (head == make_atom(env, A_PACKED)) {
            field->packed = TRUE;
        } else if (enif_get_tuple(env, head, &arity, to_const(array))
                && arity == 2 && array[0] == make_atom(env, A_DEFAULT)) {
            field->defaut_value = array[1];
        } else if (head == make_atom(env, A_DEPRECATED)) {
            /* skip */
        }  else if (enif_get_tuple(env, head, &arity, to_const(array))
                && arity == 2 && array[0] == make_atom(env, A_EBIN)
                && array[1] == state->atom_true) {
            field->ebin = TRUE;
        } else {
            return RET_ERROR;
        }

        term = tail;
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_oneof_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node)
{
    ep_field_t        *field;
    int32_t         arity;
    ep_state_t        *state = (ep_state_t *) enif_priv_data(env);
    ERL_NIF_TERM    head, tail, ret;
    ERL_NIF_TERM   *array;

    field = node->fields;

    if (!enif_is_list(env, term)) {
        raise_exception(env, term);
    }
    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            raise_exception(env, term);
        }
        if (arity == 7 && array[0] == state->atom_field) {
            check_ret(ret, fill_msg_field(env, head, field));
        } else {
            raise_exception(env, term);
        }
        field++;
        term = tail;
    }

    qsort(node->fields, node->size, sizeof(ep_field_t), sort_compare_sub_name_field);
    return RET_OK;
}

ERL_NIF_TERM
fill_oneof_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    int32_t         arity;
    uint32_t        len;
    ERL_NIF_TERM   *array, ret;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        raise_exception(env, term);
    }

    if (array[0] != make_atom(env, A_ONEOF)) {
        raise_exception(env, term);
    }
    field->name = array[1];

    if (!enif_get_uint(env, array[2], &(field->rnum))) {
        raise_exception(env, term);
    }
    (field->rnum)--;

    field->type = field_oneof;

    if (!enif_get_list_length(env, array[3], &len) || len == 0) {
        raise_exception(env, term);
    }
    field->sub_node = make_node(len, node_oneof);

    field->sub_node->n_type = node_oneof;
    field->sub_node->size = len;
    check_ret(ret, parse_oneof_fields(env, array[3], field->sub_node));
    field->fnum = ((ep_field_t *) (field->sub_node->fields))->fnum;

    return RET_OK;
}

ERL_NIF_TERM
fill_msg_field(ErlNifEnv *env, ERL_NIF_TERM term, ep_field_t *field)
{
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    int32_t         arity;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        raise_exception(env, term);
    }

    if (arity != 7 || array[0] != state->atom_field) {
        raise_exception(env, term);
    }

    if (!enif_is_atom(env, array[1])) {
        raise_exception(env, term);
    }
    field->name = array[1];

    if (!enif_get_uint(env, array[2], &(field->fnum))
        || !enif_get_uint(env, array[3], &(field->rnum))) {
        raise_exception(env, term);
    }
    (field->rnum)--;

    if (parse_field_type(env, array[4], field) != RET_OK) {
        raise_exception(env, term);
    }

    if (parse_occurrence_type(env, array[5], field) != RET_OK) {
        raise_exception(env, term);
    }

    if (parse_opts(env, array[6], field) != RET_OK) {
        raise_exception(env, term);
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_msg_fields(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t *node)
{
    ep_field_t         *field, *f;
    int32_t             arity;
    ep_state_t         *state = (ep_state_t *) enif_priv_data(env);
    uint32_t            i, j;
    ep_fnum_field_t    *ff;
    ERL_NIF_TERM        head, tail, ret;
    ERL_NIF_TERM       *array;

    field = node->fields;
    node->v_size = 0;
    while (enif_get_list_cell(env, term, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            raise_exception(env, head);
        }

        if (arity == 7 && array[0] == state->atom_field) {
            check_ret(ret, fill_msg_field(env, head, field));
            node->v_size++;
        } else if (array[0] == make_atom(env, A_ONEOF)) {
            check_ret(ret, fill_oneof_field(env, head, field));
            node->v_size += field->sub_node->size;
        } else {
            raise_exception(env, head);
        }

        field->proto_v = node->proto_v;
        field++;
        term = tail;
    }

    qsort(node->fields, node->size, sizeof(ep_field_t), sort_compare_msg_field);

    if (node->v_size > 0) {
        node->v_fields = _calloc(sizeof(ep_fnum_field_t), node->v_size);
        ff = node->v_fields;
        field = node->fields;
        for (i = 0; i < node->size; i++) {
            if (field->type == field_oneof) {
                f = field->sub_node->fields;
                for (j = 0; j < field->sub_node->size; j++) {
                    ff->fnum = f->fnum;
                    ff->field = f;
                    f->is_oneof = TRUE;
                    ff++;
                    f++;
                }
            } else {
                ff->fnum = field->fnum;
                ff->field = field;
                ff++;
            }
            field++;
        }

        ff = node->v_fields;
        for (i = 0; i < node->v_size; i++) {
            ff++;
        }

        qsort(node->v_fields, node->v_size, sizeof(ep_fnum_field_t), sort_compare_fnum_field);
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_node(ErlNifEnv *env, ERL_NIF_TERM term, ep_node_t **node, uint32_t proto_v, ERL_NIF_TERM proto3_list)
{
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);
    int32_t         arity, msg_arity;
    uint32_t        len;
    node_type_e     n_type;
    ERL_NIF_TERM    head, tail, *array, *msg_array, ret;

    *node = NULL;
    if (!enif_get_tuple(env, term, &arity, to_const(array)) || arity != 2) {
        return RET_OK;
    }

    if (!enif_get_tuple(env, array[0], &msg_arity, to_const(msg_array)) || msg_arity != 2) {
        return RET_OK;
    }

    if (!enif_get_list_length(env, array[1], &len)) {
        return RET_OK;
    }

    if (msg_array[0] == state->atom_msg) {
        n_type = node_msg;
    } else if (msg_array[0] == state->atom_enum) {
        n_type = node_enum;
    } else {
        return RET_OK;
    }

    *node = make_node(len, n_type);
    if (*node == NULL) {
        raise_exception(env, state->atom_error);
    }

    if (!enif_is_atom(env, msg_array[1])) {
        raise_exception(env, term);
    }
    (*node)->name = msg_array[1];

    (*node)->proto_v = proto_v;
    if (proto3_list) {
        while (enif_get_list_cell(env, proto3_list, &head, &tail)) {
            if ((*node)->name == head) {
                (*node)->proto_v = 3;
            }
            proto3_list = tail;
        }
    }

    (*node)->id = 0;
    (*node)->size = len;
    (*node)->n_type = n_type;

    if (n_type == node_msg) {
        check_ret(ret, parse_msg_fields(env, array[1], (*node)));
    } else {
        check_ret(ret, parse_enum_fields(env, array[1], (*node)));
    }
    return RET_OK;
}

ERL_NIF_TERM
link_sub_node(ErlNifEnv *env, ep_cache_t *cache, ep_field_t *field)
{
    ERL_NIF_TERM    ret;

    if (field->type == field_msg || field->type == field_map || field->type == field_enum) {
        if (field->sub_node == NULL) {
            field->sub_node = get_node_by_name(field->sub_name, cache);
            if (field->sub_node == NULL) {
                raise_exception(env, field->sub_name);
            }
        } else {
            check_ret(ret, do_prelink_nodes(env, cache, field->sub_node));
        }
    }
    return RET_OK;
}

ERL_NIF_TERM
do_prelink_nodes(ErlNifEnv *env, ep_cache_t *cache, ep_node_t *node)
{
    uint32_t		    i;
    ep_field_t         *field;
    ERL_NIF_TERM        ret;
    ep_fnum_field_t    *fnum_field;

    if (node->n_type == node_msg || node->n_type == node_map) {
        for (i = 0; i < node->size; i++) {
            field = (ep_field_t *) (node->fields) + i;
            check_ret(ret, link_sub_node(env, cache, field));
        }

        for (i = 0; i < node->v_size; i++) {
            fnum_field = (ep_fnum_field_t *) (node->v_fields) + i;
            check_ret(ret, link_sub_node(env, cache, fnum_field->field));
        }
    }
    return RET_OK;
}

ERL_NIF_TERM
prelink_nodes(ErlNifEnv *env, ep_cache_t *cache)
{
    size_t          i;
    ep_node_t      *node;
    ERL_NIF_TERM    ret;

    for (i = 0; i < cache->used; i++) {
        node = cache->names[i].node;
        check_ret(ret, do_prelink_nodes(env, cache, node));
    }

    return RET_OK;
}

ERL_NIF_TERM
stack_ensure(ErlNifEnv *env, ep_stack_t *stack, ep_spot_t **spot)
{
    ep_spot_t      *spots;
    size_t          size;

    if ((*spot) >= stack->end) {
        size = stack->size * 2;
        spots = _realloc(stack->spots, sizeof(ep_spot_t) * size);
        if (spots == NULL) {
            raise_exception(env, enif_make_string(env, "realloc failed", ERL_NIF_LATIN1));
        }

        *spot = spots + (*spot - stack->spots);

        memset(spots + stack->size, 0x00, sizeof(ep_spot_t) * (size - stack->size));
        stack->spots = spots;
        stack->size = size;
        stack->end = stack->spots + stack->size;
    }

    return RET_OK;
}

ERL_NIF_TERM
stack_ensure_size(ErlNifEnv *env, ep_stack_t *stack, size_t size)
{
    ep_spot_t          *spots;

    if (stack->size >= size) {
        return RET_OK;
    }

    spots = _realloc(stack->spots, sizeof(ep_spot_t) * size);
    if (spots == NULL) {
        raise_exception(env, enif_make_string(env, "realloc failed", ERL_NIF_LATIN1));
    }

    memset(spots + stack->size, 0x00, sizeof(ep_spot_t) * (size - stack->size));
    stack->spots = spots;
    stack->size = size;
    stack->end = stack->spots + stack->size;

    return RET_OK;
}

void
stack_ensure_all(ErlNifEnv *env, ep_cache_t *cache)
{
    size_t          i, j, stack_size;
    ep_spot_t      *spot;
    ep_field_t     *field;
    ep_stack_t     *stack;
    ep_state_t     *state = (ep_state_t *) enif_priv_data(env);

    stack = &(state->tdata[0].stack);

    for (i = 0; i < cache->used; i++) {
        spot = stack->spots;
        spot->type = spot_tuple;
        spot->node = cache->names[i].node;
        spot->pos = 0;

        while (spot >= stack->spots) {
            if (spot->type == spot_tuple) {
                if (spot->pos == spot->node->size) {
                    spot->type = spot_default;
                    spot->pos = 0;
                    spot--;
                    continue;
                }

                // skip over enums - they can't be nested
                if (spot->node->n_type == node_enum) {
                    spot->pos = spot->node->size;
                    continue;
                }

                for (j = spot->pos; j < (size_t) (spot->node->size); j++) {
                    spot->pos = j + 1;
                    field = ((ep_field_t *) (spot->node->fields)) + j;
                    if (field->o_type == occurrence_repeated) {
                        if (field->type == field_msg || field->type == field_map) {
                            spot++;
                            stack_ensure(env, stack, &spot);

                            spot->type = spot_list;
                            spot->pos = 0;
                            spot->node = field->sub_node;
                            break;
                        }
                    } else if (field->type == field_oneof
                            || field->type == field_msg
                            || field->type == field_map) {
                        spot++;
                        stack_ensure(env, stack, &spot);

                        spot->type = spot_tuple;
                        spot->pos = 0;
                        spot->node = field->sub_node;
                        break;
                    }
                }
            } else if (spot->type == spot_list) {
                if (spot->pos > 0) {
                    spot--;
                    continue;
                }
                spot->pos = 1;
                spot++;
                stack_ensure(env, stack, &spot);

                spot->node = (spot - 1)->node;
                spot->type = spot_tuple;
                spot->pos = 0;
            }
        }
    }

    stack_size = stack->size;
    for (i = 1; i < state->lock_n; i++) {
        stack = &(state->tdata[i].stack);
        stack_ensure_size(env, stack, stack_size);
    }
}

int
get_field_compare_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((ep_field_t *) b)->name);
}

int
get_field_compare_sub_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((ep_field_t *) b)->sub_name);
}

int
get_map_field_compare_fnum(const void *a, const void *b)
{
    return (int) (*((int32_t *) a) - ((ep_field_t *) b)->fnum);
}

int
get_field_compare_fnum(const void *a, const void *b)
{
    return (int) (*((int32_t *) a) - ((ep_fnum_field_t *) b)->fnum);
}

int
get_enum_compare_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((ep_enum_field_t *) b)->name);
}

int
get_enum_compare_value(const void *a, const void *b)
{
    return (int) (*((int32_t *) a) - ((ep_enum_field_t *) b)->value);
}
