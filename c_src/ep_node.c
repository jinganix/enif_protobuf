/*
 * Copyright (c) jg_513@163.com, https://github.com/jg513
 */

#include "enif_protobuf.h"

ERL_NIF_TERM
fill_msg_field(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field);

static int
sort_compare_msg_field(const void *a, const void *b)
{
    return (int) (((field_t *) a)->rnum - ((field_t *) b)->rnum);
}

static int
sort_compare_msg_v_field(const void *a, const void *b)
{
    return (int) (((field_t *) a)->fnum - ((field_t *) b)->fnum);
}

node_t *
make_node(int fields_n, node_type_e n_type)
{
    node_t     *node = _calloc(sizeof(node_t), 1);

    if (node == NULL) {
        return NULL;
    }

    if (fields_n > 0) {

        if (n_type == node_msg || n_type == node_oneof) {
            node->fields = _calloc(sizeof(field_t), fields_n);
            node->v_fields = _calloc(sizeof(field_t), fields_n);

        } else if (n_type == node_map) {
            node->fields = _calloc(sizeof(field_t), fields_n);

        } else if (n_type == node_enum) {
            node->fields = _calloc(sizeof(enum_field_t), fields_n);
            node->v_fields = _calloc(sizeof(enum_field_t), fields_n);
        }

        if (node->fields == NULL) {
            _free(node);
            return NULL;
        }
    }

    return node;
}

/*
 * enum
 */
ERL_NIF_TERM
fill_enum_field(ErlNifEnv *env, ERL_NIF_TERM term, enum_field_t *field)
{
    int32_t         arity, value;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array)) || (arity != 2 && arity != 3)) {
        return_error(env, term);
    }

    if (arity == 2) {

        if (enif_is_atom(env, array[0]) && enif_get_int(env, array[1], &value)) {
            field->name = array[0];
            field->value = value;

        } else {
            return_error(env, term);
        }
    }

    return RET_OK;
}

static int
sort_compare_enum_field(const void *a, const void *b)
{
    return (int) (((enum_field_t *) a)->name - ((enum_field_t *) b)->name);
}

static int
sort_compare_enum_v_field(const void *a, const void *b)
{
    return (int) (((enum_field_t *) a)->value - ((enum_field_t *) b)->value);
}

ERL_NIF_TERM
parse_enum_fields(ErlNifEnv *env, ERL_NIF_TERM term, node_t *node)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity, allow_alias = 0;
    enum_field_t   *field, *v_field, *vf;
    ERL_NIF_TERM   *array;
    ERL_NIF_TERM    head, tail, tmp, ret;

    tmp = term;
    while (enif_get_list_cell(env, tmp, &head, &tail)) {
        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            return_error(env, term);
        }

        if (arity == 3) {
            if (array[0] == state->atom_option && array[1] == state->atom_allow_alias) {

                if (array[2] == state->atom_true) {
                    allow_alias = 1;
                    break;

                } else if (array[2] == state->atom_false) {
                    allow_alias = 0;
                    break;

                } else {
                    return_error(env, head);
                }

            } else {
                return_error(env, head);
            }
        }

        tmp = tail;
    }

    field = node->fields;
    v_field = node->v_fields;
    while (enif_get_list_cell(env, term, &head, &tail)) {
        check_ret(ret, fill_enum_field(env, head, field));

        v_field->name = field->name;
        v_field->value = field->value;

        for (vf = node->v_fields; vf < v_field; vf++) {
            if (vf->value == v_field->value) {

                if (allow_alias) {
                    v_field--;
                    break;

                } else {
                    return_error(env, head);
                }
            }
        }
        field->proto_v = node->proto_v;
        field++;
        v_field++;
        term = tail;
    }

    qsort(node->fields, node->size, sizeof(enum_field_t), sort_compare_enum_field);

    node->v_size = v_field - (enum_field_t *) node->v_fields;
    qsort(node->v_fields, node->v_size, sizeof(enum_field_t), sort_compare_enum_v_field);

    return RET_OK;
}

/*
 * msg
 */
ERL_NIF_TERM
parse_field_basic(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity;
    ERL_NIF_TERM   *array;

    if (term == state->atom_int32)          { field->f_type.type = field_int32; return RET_OK; }
    else if (term == state->atom_int64)     { field->f_type.type = field_int64; return RET_OK; }
    else if (term == state->atom_uint32)    { field->f_type.type = field_uint32; return RET_OK; }
    else if (term == state->atom_uint64)    { field->f_type.type = field_uint64; return RET_OK; }
    else if (term == state->atom_sint32)    { field->f_type.type = field_sint32; return RET_OK; }
    else if (term == state->atom_sint64)    { field->f_type.type = field_sint64; return RET_OK; }
    else if (term == state->atom_fixed32)   { field->f_type.type = field_fixed32; return RET_OK; }
    else if (term == state->atom_fixed64)   { field->f_type.type = field_fixed64; return RET_OK; }
    else if (term == state->atom_sfixed32)  { field->f_type.type = field_sfixed32; return RET_OK; }
    else if (term == state->atom_sfixed64)  { field->f_type.type = field_sfixed64; return RET_OK; }
    else if (term == state->atom_bool)      { field->f_type.type = field_bool; return RET_OK; }
    else if (term == state->atom_float)     { field->f_type.type = field_float; return RET_OK; }
    else if (term == state->atom_double)    { field->f_type.type = field_double; return RET_OK; }
    else if (term == state->atom_string)    { field->f_type.type = field_string; return RET_OK; }
    else if (term == state->atom_bytes)     { field->f_type.type = field_bytes; return RET_OK; }

    else if (enif_get_tuple(env, term, &arity, to_const(array))) {

        if (arity == 2 && array[0] == state->atom_enum && enif_is_atom(env, array[1])) {
            field->f_type.type = field_enum;
            field->f_type.name = array[1];
            return RET_OK;

        } else if (arity == 2 && array[0] == state->atom_msg && enif_is_atom(env, array[1])) {
            field->f_type.type = field_msg;
            field->f_type.name = array[1];
            return RET_OK;
        }
    }

    return RET_ERROR;
}

ERL_NIF_TERM
parse_map_type(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity;
    field_t        *f;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        return_error(env, term);
    }

    if (arity != 3 || array[0] != state->atom_map) {
        return_error(env, term);
    }

    field->f_type.type = field_map;
    field->f_type.node = make_node(2, node_map);
    if (field->f_type.node == NULL) {
        return RET_ERROR;
    }

    field->f_type.node->n_type = node_map;
    field->f_type.node->size = 2;

    f = field->f_type.node->fields;
    if (array[1] == state->atom_int32)          { f->f_type.type = field_int32; }
    else if (array[1] == state->atom_int64)     { f->f_type.type = field_int64; }
    else if (array[1] == state->atom_uint32)    { f->f_type.type = field_uint32; }
    else if (array[1] == state->atom_uint64)    { f->f_type.type = field_uint64; }
    else if (array[1] == state->atom_sint32)    { f->f_type.type = field_sint32; }
    else if (array[1] == state->atom_sint64)    { f->f_type.type = field_sint64; }
    else if (array[1] == state->atom_fixed32)   { f->f_type.type = field_fixed32; }
    else if (array[1] == state->atom_fixed64)   { f->f_type.type = field_fixed64; }
    else if (array[1] == state->atom_sfixed32)  { f->f_type.type = field_sfixed32; }
    else if (array[1] == state->atom_sfixed64)  { f->f_type.type = field_sfixed64; }
    else if (array[1] == state->atom_bool)      { f->f_type.type = field_bool; }
    else if (array[1] == state->atom_double)    { f->f_type.type = field_double; }
    else if (array[1] == state->atom_string)    { f->f_type.type = field_string; }
    else {
        return RET_ERROR;
    }
    f->fnum = 1;
    f->rnum = 1;

    f++;
    f->fnum = 2;
    f->rnum = 2;
    if (parse_field_basic(env, array[2], f) != RET_OK) {
        return RET_ERROR;
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_field_type(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
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
parse_occurrence_type(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);

    if (term == state->atom_required)       { field->o_type = occurrence_required; return RET_OK; }
    else if (term == state->atom_optional)  { field->o_type = occurrence_optional; return RET_OK; }
    else if (term == state->atom_repeated)  { field->o_type = occurrence_repeated; return RET_OK; }

    return RET_ERROR;
}

ERL_NIF_TERM
parse_opts(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity;
    ERL_NIF_TERM   *array;
    ERL_NIF_TERM    head, tail;

    while (enif_get_list_cell(env, term, &head, &tail)) {

        if (head == state->atom_packed) {
            field->opts.packed = TRUE;

        } else if (enif_get_tuple(env, head, &arity, to_const(array))
                && arity == 2 && array[0] == state->atom_default) {

            field->opts.defaut_value = array[1];

        } else {
            return RET_ERROR;
        }

        term = tail;
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_oneof_fields(ErlNifEnv *env, ERL_NIF_TERM term, node_t *node)
{
    field_t        *field;
    int32_t         arity;
    state_t        *state = (state_t *) enif_priv_data(env);
    ERL_NIF_TERM    head, tail, ret;
    ERL_NIF_TERM   *array;

    field = node->fields;

    if (!enif_is_list(env, term)) {
        return_error(env, term);
    }
    while (enif_get_list_cell(env, term, &head, &tail)) {

        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            return_error(env, term);
        }

        if (arity == 7 && array[0] == state->atom_field) {
            check_ret(ret, fill_msg_field(env, head, field));

        } else {
            return_error(env, term);
        }

        field++;
        term = tail;
    }

    qsort(node->fields, node->size, sizeof(field_t), sort_compare_msg_field);

    node->v_size = node->size;
    qsort(node->v_fields, node->v_size, sizeof(field_t), sort_compare_msg_v_field);

    return RET_OK;
}

ERL_NIF_TERM
fill_oneof_field(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity;
    uint32_t        len;
    ERL_NIF_TERM   *array, ret;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        return_error(env, term);
    }

    if (arity != 4 || array[0] != state->atom_gpb_oneof) {
        return_error(env, term);
    }
    field->name = array[1];

    if (!enif_get_int(env, array[2], &(field->rnum))) {
        return_error(env, term);
    }

    field->f_type.type = field_oneof;

    if (!enif_get_list_length(env, array[3], &len) || len == 0) {
        return_error(env, term);
    }
    field->f_type.node = make_node(len, node_oneof);

    field->f_type.node->n_type = node_oneof;
    field->f_type.node->size = len;
    check_ret(ret, parse_oneof_fields(env, array[3], field->f_type.node));
    field->fnum = ((field_t *) (field->f_type.node->fields))->fnum;

    return RET_OK;
}

ERL_NIF_TERM
fill_msg_field(ErlNifEnv *env, ERL_NIF_TERM term, field_t *field)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity;
    ERL_NIF_TERM   *array;

    if (!enif_get_tuple(env, term, &arity, to_const(array))) {
        return_error(env, term);
    }

    if (arity != 7 || array[0] != state->atom_field) {
        return_error(env, term);
    }

    if (!enif_is_atom(env, array[1])) {
        return_error(env, term);
    }
    field->name = array[1];

    if (!enif_get_int(env, array[2], &(field->fnum))
            || !enif_get_int(env, array[3], &(field->rnum))) {
        return_error(env, term);
    }

    if (parse_field_type(env, array[4], field) != RET_OK) {
        return_error(env, term);
    }

    if (parse_occurrence_type(env, array[5], field) != RET_OK) {
        return_error(env, term);
    }

    if (parse_opts(env, array[6], field) != RET_OK) {
        return_error(env, term);
    }

    return RET_OK;
}

ERL_NIF_TERM
parse_msg_fields(ErlNifEnv *env, ERL_NIF_TERM term, node_t *node)
{
    field_t        *field;
    int32_t         arity;
    state_t        *state = (state_t *) enif_priv_data(env);
    ERL_NIF_TERM    head, tail, ret;
    ERL_NIF_TERM   *array;

    field = node->fields;
    while (enif_get_list_cell(env, term, &head, &tail)) {

        if (!enif_get_tuple(env, head, &arity, to_const(array))) {
            return_error(env, head);
        }

        if (arity == 7 && array[0] == state->atom_field) {
            check_ret(ret, fill_msg_field(env, head, field));

        } else if (arity == 4 && array[0] == state->atom_gpb_oneof) {
            check_ret(ret, fill_oneof_field(env, head, field));

        } else {
            return_error(env, head);
        }

        field->proto_v = node->proto_v;
        field++;
        term = tail;
    }

    qsort(node->fields, node->size, sizeof(field_t), sort_compare_msg_field);

    node->v_size = node->size;
    qsort(node->v_fields, node->v_size, sizeof(field_t), sort_compare_msg_v_field);

    return RET_OK;
}

ERL_NIF_TERM
parse_node(ErlNifEnv *env, ERL_NIF_TERM term, node_t **node, uint32_t proto_v, ERL_NIF_TERM syn_list)
{
    state_t        *state = (state_t *) enif_priv_data(env);
    int32_t         arity, msg_arity;
    uint32_t        len;
    node_type_e     n_type;
    ERL_NIF_TERM    head, tail, *array, *msg_array, ret;

    if (!enif_get_tuple(env, term, &arity, to_const(array)) || arity != 2) {
        return_error(env, term);
    }

    if (!enif_get_tuple(env, array[0], &msg_arity, to_const(msg_array)) || msg_arity != 2) {
        return_error(env, term);
    }

    if (!enif_get_list_length(env, array[1], &len)) {
        return_error(env, term);
    }

    if (msg_array[0] == state->atom_msg) {
        n_type = node_msg;

    } else if (msg_array[0] == state->atom_enum) {
        n_type = node_enum;

    } else {
        return_error(env, term);
    }

    *node = make_node(len, n_type);
    if (*node == NULL) {
        return_exception(env, state->atom_error);
    }

    if (!enif_is_atom(env, msg_array[1])) {
        return_error(env, term);
    }
    (*node)->name = msg_array[1];

    (*node)->proto_v = 2;
    if (syn_list) {

        while (enif_get_list_cell(env, syn_list, &head, &tail)) {

            if ((*node)->name == head) {
                (*node)->proto_v = proto_v;
            }
            syn_list = tail;
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
prelink_nodes(ErlNifEnv *env)
{
    size_t          i, j;
    node_t         *node;
    field_t        *f;
    state_t        *state = (state_t *) enif_priv_data(env);

    for (i = 0; i < state->cache->size; i++) {

        node = state->cache->names[i].node;
        if (node->n_type == node_msg) {

            for (j = 0; j < node->size; j++) {

                f = (field_t *) (node->fields) + j;
                if (f->f_type.type == field_msg || f->f_type.type == field_enum) {
                    f->f_type.node = get_node_by_name(f->f_type.name, state->cache);

                    if (f->f_type.node == NULL) {
                        return_error(env, f->f_type.name);
                    }
                }
            }

            for (j = 0; j < node->v_size; j++) {

                f = (field_t *) (node->v_fields) + j;
                if (f->f_type.type == field_msg || f->f_type.type == field_enum) {
                    f->f_type.node = get_node_by_name(f->f_type.name, state->cache);

                    if (f->f_type.node == NULL) {
                        return_error(env, f->f_type.name);
                    }
                }
            }
        }
    }

    return RET_OK;
}

int
get_field_compare_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((field_t *) b)->name);
}

int
get_enum_compare_name(const void *a, const void *b)
{
    return (int) (*((ERL_NIF_TERM *) a) - ((enum_field_t *) b)->name);
}
