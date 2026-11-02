/* Dynamic message value tree: allocations, accessors, field storage, oneof
 * handling. */

#include "internal.h"

static pbw_value *
value_alloc_a(pbw_arena *arena, pbw_value_kind kind)
{
    pbw_value *value;

    if (arena != NULL) {
        /* No zeroing: each constructor below initialises exactly the fields it
         * uses, and the union is always written before it is read. */
        value = (pbw_value *)pbw_arena_alloc(arena, sizeof(pbw_value));
    } else {
        value = (pbw_value *)calloc(1, sizeof(pbw_value));
    }
    if (value != NULL) {
        value->kind = kind;
        value->arena = arena;
    }
    return value;
}

static void
value_free(pbw_value *value)
{
    size_t i;

    if (value == NULL || value->arena != NULL) {
        /* Context-owned values die with pbw_ctx_reset(), not one by one. */
        return;
    }
    switch (value->kind) {
    case PBW_VALUE_BYTES:
        free(value->as.bytes.data);
        break;
    case PBW_VALUE_MESSAGE:
        pbw_message_free(value->as.msg);
        break;
    case PBW_VALUE_LIST:
        for (i = 0; i < value->as.list.len; i++) {
            value_free(value->as.list.items[i]);
        }
        free(value->as.list.items);
        break;
    case PBW_VALUE_UINT:
    default:
        break;
    }
    free(value);
}

void
pbw_value_free(pbw_value *value)
{
    value_free(value);
}

void
pbw_value_clear(pbw_value *value)
{
    if (value == NULL) {
        return;
    }
    /* Free the payload in place so the caller keeps its pbw_value pointer. */
    switch (value->kind) {
    case PBW_VALUE_BYTES:
        free(value->as.bytes.data);
        value->as.bytes.data = NULL;
        value->as.bytes.len = 0;
        break;
    case PBW_VALUE_MESSAGE:
        pbw_message_free(value->as.msg);
        value->as.msg = NULL;
        break;
    default:
        break;
    }
}

pbw_value *
pbw_value_new_uint_a(pbw_arena *arena, uint64_t bits)
{
    pbw_value *value = value_alloc_a(arena, PBW_VALUE_UINT);
    if (value != NULL) {
        value->as.u64 = bits;
    }
    return value;
}

pbw_value *
pbw_value_new_uint(uint64_t bits)
{
    return pbw_value_new_uint_a(NULL, bits);
}

pbw_value *
pbw_value_new_int(int64_t number)
{
    return pbw_value_new_uint((uint64_t)number);
}

pbw_value *
pbw_value_new_bool(int flag)
{
    return pbw_value_new_uint(flag ? 1u : 0u);
}

pbw_value *
pbw_value_new_float(float number)
{
    uint32_t bits;

    memcpy(&bits, &number, sizeof(bits));
    return pbw_value_new_uint(bits);
}

pbw_value *
pbw_value_new_double(double number)
{
    uint64_t bits;

    memcpy(&bits, &number, sizeof(bits));
    return pbw_value_new_uint(bits);
}

pbw_value *
pbw_value_new_bytes_a(pbw_arena *arena, const void *data, size_t len)
{
    pbw_value *value = value_alloc_a(arena, PBW_VALUE_BYTES);

    if (value == NULL) {
        return NULL;
    }
    /* Always allocate len + 1 and terminate: string and bytes share one value
     * kind, and decoded strings must be usable as C strings. */
    if (arena != NULL) {
        value->as.bytes.data = (uint8_t *)pbw_arena_alloc(arena, len + 1);
    } else {
        value->as.bytes.data = (uint8_t *)malloc(len + 1);
    }
    if (value->as.bytes.data == NULL) {
        value_free(value);
        return NULL;
    }
    if (len > 0) {
        memcpy(value->as.bytes.data, data, len);
    }
    value->as.bytes.data[len] = '\0';
    value->as.bytes.len = len;
    return value;
}

pbw_value *
pbw_value_new_bytes(const void *data, size_t len)
{
    return pbw_value_new_bytes_a(NULL, data, len);
}

pbw_value *
pbw_value_new_string(const char *str)
{
    return str == NULL ? NULL : pbw_value_new_bytes(str, strlen(str));
}

pbw_value *
pbw_value_new_message_a(pbw_arena *arena, pbw_message *msg)
{
    pbw_value *value = value_alloc_a(arena, PBW_VALUE_MESSAGE);

    if (value == NULL) {
        return NULL;
    }
    value->as.msg = msg;
    return value;
}

pbw_value *
pbw_value_new_message(pbw_message *msg)
{
    return pbw_value_new_message_a(NULL, msg);
}

pbw_value *
pbw_value_new_list_a(pbw_arena *arena)
{
    pbw_value *value = value_alloc_a(arena, PBW_VALUE_LIST);

    if (value != NULL) {
        value->as.list.items = NULL;
        value->as.list.len = 0;
        value->as.list.cap = 0;
    }
    return value;
}

pbw_value *
pbw_value_new_list(void)
{
    return pbw_value_new_list_a(NULL);
}

int
pbw_value_list_push(pbw_value *list, pbw_value *item)
{
    pbw_value **items;

    if (list == NULL || item == NULL || list->kind != PBW_VALUE_LIST) {
        return 0;
    }
    if (list->as.list.len == list->as.list.cap) {
        size_t cap = list->as.list.cap ? list->as.list.cap * 2 : 8;
        if (list->arena != NULL) {
            /* No realloc in an arena: move to a bigger block, the old one is
             * recycled by pbw_ctx_reset(). */
            items = (pbw_value **)pbw_arena_alloc(list->arena,
                                                  cap * sizeof(*items));
            if (items == NULL) {
                return 0;
            }
            if (list->as.list.len > 0) {
                memcpy(items, list->as.list.items,
                       list->as.list.len * sizeof(*items));
            }
        } else {
            items =
                (pbw_value **)realloc(list->as.list.items, cap * sizeof(*items));
            if (items == NULL) {
                return 0;
            }
        }
        list->as.list.items = items;
        list->as.list.cap = cap;
    }
    list->as.list.items[list->as.list.len++] = item;
    return 1;
}

pbw_value_kind
pbw_value_get_kind(const pbw_value *value)
{
    return value == NULL ? PBW_VALUE_UINT : value->kind;
}

uint64_t
pbw_value_get_uint(const pbw_value *value)
{
    return value == NULL ? 0 : value->as.u64;
}

int64_t
pbw_value_get_int(const pbw_value *value)
{
    return (int64_t)pbw_value_get_uint(value);
}

int32_t
pbw_value_get_int32(const pbw_value *value)
{
    return (int32_t)pbw_value_get_uint(value);
}

uint32_t
pbw_value_get_uint32(const pbw_value *value)
{
    return (uint32_t)pbw_value_get_uint(value);
}

int
pbw_value_get_bool(const pbw_value *value)
{
    return pbw_value_get_uint(value) != 0;
}

float
pbw_value_get_float(const pbw_value *value)
{
    uint32_t bits = (uint32_t)pbw_value_get_uint(value);
    float number;

    memcpy(&number, &bits, sizeof(number));
    return number;
}

double
pbw_value_get_double(const pbw_value *value)
{
    uint64_t bits = pbw_value_get_uint(value);
    double number;

    memcpy(&number, &bits, sizeof(number));
    return number;
}

const void *
pbw_value_get_bytes(const pbw_value *value, size_t *len)
{
    if (value == NULL || value->kind != PBW_VALUE_BYTES) {
        if (len != NULL) {
            *len = 0;
        }
        return NULL;
    }
    if (len != NULL) {
        *len = value->as.bytes.len;
    }
    return value->as.bytes.data;
}

const char *
pbw_value_get_string(const pbw_value *value)
{
    return (const char *)pbw_value_get_bytes(value, NULL);
}

pbw_message *
pbw_value_get_message(const pbw_value *value)
{
    return value != NULL && value->kind == PBW_VALUE_MESSAGE ? value->as.msg
                                                             : NULL;
}

size_t
pbw_value_count(const pbw_value *value)
{
    return value != NULL && value->kind == PBW_VALUE_LIST ? value->as.list.len
                                                          : 0;
}

pbw_value *
pbw_value_at(const pbw_value *value, size_t index)
{
    if (value == NULL || value->kind != PBW_VALUE_LIST ||
        index >= value->as.list.len) {
        return NULL;
    }
    return value->as.list.items[index];
}

pbw_message *
pbw_message_new_a(pbw_arena *arena, const pbw_descriptor *desc)
{
    pbw_message *msg;

    if (desc == NULL || desc->is_enum) {
        return NULL;
    }
    if (arena != NULL) {
        msg = (pbw_message *)pbw_arena_calloc(arena, sizeof(*msg));
    } else {
        msg = (pbw_message *)calloc(1, sizeof(*msg));
    }
    if (msg == NULL) {
        return NULL;
    }
    msg->desc = desc;
    msg->arena = arena;
    if (desc->field_count > 0) {
        if (arena != NULL) {
            msg->values = (pbw_value **)pbw_arena_calloc(
                arena, desc->field_count * sizeof(*msg->values));
        } else {
            msg->values =
                (pbw_value **)calloc(desc->field_count, sizeof(*msg->values));
        }
        if (msg->values == NULL) {
            if (arena == NULL) {
                free(msg);
            }
            return NULL;
        }
    }
    if (desc->oneof_count > 0) {
        size_t size = (desc->oneof_count + 1) * sizeof(*msg->oneof_case);

        if (arena != NULL) {
            msg->oneof_case = (uint32_t *)pbw_arena_calloc(arena, size);
        } else {
            msg->oneof_case = (uint32_t *)calloc(1, size);
        }
        if (msg->oneof_case == NULL) {
            if (arena == NULL) {
                free(msg->values);
                free(msg);
            }
            return NULL;
        }
    }
    return msg;
}

pbw_message *
pbw_message_new(const pbw_descriptor *desc)
{
    return pbw_message_new_a(NULL, desc);
}

int
pbw_message_add_unknown(pbw_message *msg, const uint8_t *data, size_t len)
{
    size_t need = msg->unknown_len + len;
    uint8_t *grown;

    if (len == 0) {
        return PBW_OK;
    }
    if (need > msg->unknown_cap) {
        size_t cap = msg->unknown_cap == 0 ? 64 : msg->unknown_cap;

        while (cap < need) {
            cap *= 2;
        }
        if (msg->arena != NULL) {
            grown = (uint8_t *)pbw_arena_alloc(msg->arena, cap);
            if (grown != NULL && msg->unknown_len > 0) {
                memcpy(grown, msg->unknown, msg->unknown_len);
            }
        } else {
            grown = (uint8_t *)realloc(msg->unknown, cap);
        }
        if (grown == NULL) {
            return PBW_ERR_NOMEM;
        }
        msg->unknown = grown;
        msg->unknown_cap = cap;
    }
    memcpy(msg->unknown + msg->unknown_len, data, len);
    msg->unknown_len = need;
    return PBW_OK;
}

void
pbw_message_free(pbw_message *msg)
{
    size_t i;

    if (msg == NULL || msg->arena != NULL) {
        /* Context-owned messages die with pbw_ctx_reset(). */
        return;
    }
    for (i = 0; i < msg->desc->field_count; i++) {
        value_free(msg->values[i]);
    }
    free(msg->values);
    free(msg->oneof_case);
    free(msg->unknown);
    free(msg);
}

const pbw_descriptor *
pbw_message_descriptor(const pbw_message *msg)
{
    return msg == NULL ? NULL : msg->desc;
}

static size_t
field_index(const pbw_message *msg, const pbw_field *field)
{
    return (size_t)(field - msg->desc->fields);
}

pbw_value *
pbw_message_get(const pbw_message *msg, uint32_t number)
{
    const pbw_field *field;

    if (msg == NULL) {
        return NULL;
    }
    field = pbw_descriptor_find_field(msg->desc, number);
    return field == NULL ? NULL : msg->values[field_index(msg, field)];
}

pbw_value *
pbw_message_get_field(const pbw_message *msg, const pbw_field *field)
{
    if (msg == NULL || field == NULL) {
        return NULL;
    }
    return msg->values[field_index(msg, field)];
}

pbw_value *
pbw_message_get_index(const pbw_message *msg, size_t index)
{
    if (msg == NULL || index >= msg->desc->field_count) {
        return NULL;
    }
    return msg->values[index];
}

size_t
pbw_message_field_count(const pbw_message *msg)
{
    size_t i;
    size_t count = 0;

    if (msg == NULL) {
        return 0;
    }
    for (i = 0; i < msg->desc->field_count; i++) {
        if (msg->values[i] != NULL) {
            count++;
        }
    }
    return count;
}

const pbw_field *
pbw_message_field_at(const pbw_message *msg, size_t index,
                     pbw_value **value)
{
    size_t i;
    size_t seen = 0;

    if (msg == NULL) {
        return NULL;
    }
    for (i = 0; i < msg->desc->field_count; i++) {
        if (msg->values[i] == NULL) {
            continue;
        }
        if (seen == index) {
            if (value != NULL) {
                *value = msg->values[i];
            }
            return &msg->desc->fields[i];
        }
        seen++;
    }
    return NULL;
}

static int
value_matches(const pbw_field *field, const pbw_value *value)
{
    switch (field->type) {
    case PBW_TYPE_STRING:
    case PBW_TYPE_BYTES:
        return value->kind == PBW_VALUE_BYTES;
    case PBW_TYPE_MESSAGE:
        return value->kind == PBW_VALUE_MESSAGE && value->as.msg != NULL &&
               (field->sub == NULL || value->as.msg->desc == field->sub);
    default:
        return value->kind == PBW_VALUE_UINT;
    }
}

static void clear_field(pbw_message *msg, size_t index);

void
pbw_message_clear(pbw_message *msg, uint32_t number)
{
    const pbw_field *field;

    if (msg == NULL || (msg->arena != NULL && msg->arena->readonly)) {
        return;
    }
    field = pbw_descriptor_find_field(msg->desc, number);
    if (field == NULL) {
        return;
    }
    clear_field(msg, field_index(msg, field));
    if (field->oneof_index > 0) {
        msg->oneof_case[field->oneof_index] = 0;
    }
}

static void
clear_field(pbw_message *msg, size_t index)
{
    if (msg->values[index] != NULL) {
        value_free(msg->values[index]);
        msg->values[index] = NULL;
    }
}

/* Store a single value for a field, honoring oneof semantics. Takes ownership
 * of value. */
int
pbw_message_store(pbw_message *msg, const pbw_field *field,
                  pbw_value *value)
{
    size_t index = field_index(msg, field);
    size_t i;

    if (field->oneof_index > 0) {
        uint32_t number = msg->oneof_case[field->oneof_index];
        if (number != 0 && number != field->number) {
            for (i = 0; i < msg->desc->field_count; i++) {
                if (msg->desc->fields[i].number == number) {
                    clear_field(msg, i);
                    break;
                }
            }
        }
        msg->oneof_case[field->oneof_index] = field->number;
    }
    clear_field(msg, index);
    msg->values[index] = value;
    return 1;
}

/* Get-or-create the list storage of a repeated field. */
pbw_value *
pbw_message_list_for(pbw_message *msg, const pbw_field *field)
{
    size_t index = field_index(msg, field);
    pbw_value *list = msg->values[index];

    if (list != NULL && list->kind == PBW_VALUE_LIST) {
        return list;
    }
    clear_field(msg, index);
    list = pbw_value_new_list_a(msg->arena);
    if (list != NULL) {
        msg->values[index] = list;
    }
    return list;
}

pbw_status
pbw_message_set(pbw_message *msg, uint32_t number,
                pbw_value *value)
{
    const pbw_field *field;

    if (msg == NULL || value == NULL) {
        return PBW_ERR_INVALID;
    }
    if (msg->arena != NULL && msg->arena->readonly) {
        /* Messages decoded through a context are read-only: a stored value would
         * outlive the arena that owns its siblings. */
        return PBW_ERR_INVALID;
    }
    field = pbw_descriptor_find_field(msg->desc, number);
    if (field == NULL) {
        return PBW_ERR_NOT_FOUND;
    }
    if (field->label == PBW_LABEL_REPEATED) {
        pbw_value *list;
        if (!value_matches(field, value)) {
            return PBW_ERR_TYPE;
        }
        list = pbw_message_list_for(msg, field);
        if (list == NULL || !pbw_value_list_push(list, value)) {
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    if (!value_matches(field, value)) {
        return PBW_ERR_TYPE;
    }
    pbw_message_store(msg, field, value);
    return PBW_OK;
}

pbw_status
pbw_message_set_field(pbw_message *msg, const pbw_field *field,
                      pbw_value *value)
{
    uintptr_t p;
    uintptr_t base;

    if (msg == NULL || field == NULL || value == NULL) {
        return PBW_ERR_INVALID;
    }
    if (msg->arena != NULL && msg->arena->readonly) {
        return PBW_ERR_INVALID;
    }
    p = (uintptr_t)field;
    base = (uintptr_t)msg->desc->fields;
    if (p < base || (p - base) % sizeof(*field) != 0 ||
        (p - base) / sizeof(*field) >= msg->desc->field_count) {
        return PBW_ERR_NOT_FOUND;
    }
    if (field->label == PBW_LABEL_REPEATED) {
        pbw_value *list;

        if (!value_matches(field, value)) {
            return PBW_ERR_TYPE;
        }
        list = pbw_message_list_for(msg, field);
        if (list == NULL || !pbw_value_list_push(list, value)) {
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    if (!value_matches(field, value)) {
        return PBW_ERR_TYPE;
    }
    pbw_message_store(msg, field, value);
    return PBW_OK;
}

pbw_status
pbw_message_set_index(pbw_message *msg, size_t index, pbw_value *value)
{
    const pbw_field *field;

    if (msg == NULL || value == NULL || index >= msg->desc->field_count) {
        return PBW_ERR_INVALID;
    }
    if (msg->arena != NULL && msg->arena->readonly) {
        return PBW_ERR_INVALID;
    }
    field = &msg->desc->fields[index];
    if (field->label == PBW_LABEL_REPEATED) {
        pbw_value *list;

        if (!value_matches(field, value)) {
            return PBW_ERR_TYPE;
        }
        list = pbw_message_list_for(msg, field);
        if (list == NULL || !pbw_value_list_push(list, value)) {
            return PBW_ERR_NOMEM;
        }
        return PBW_OK;
    }
    if (!value_matches(field, value)) {
        return PBW_ERR_TYPE;
    }
    pbw_message_store(msg, field, value);
    return PBW_OK;
}

pbw_message *
pbw_message_set_message(pbw_message *msg, uint32_t number)
{
    const pbw_field *field;
    pbw_value *value;
    pbw_message *nested;

    if (msg == NULL || (msg->arena != NULL && msg->arena->readonly)) {
        return NULL;
    }
    field = pbw_descriptor_find_field(msg->desc, number);
    if (field == NULL || field->type != PBW_TYPE_MESSAGE || field->sub == NULL) {
        return NULL;
    }
    if (field->label != PBW_LABEL_REPEATED) {
        pbw_value *existing = msg->values[field_index(msg, field)];
        if (existing != NULL && existing->kind == PBW_VALUE_MESSAGE) {
            return existing->as.msg;
        }
    }
    nested = pbw_message_new(field->sub);
    if (nested == NULL) {
        return NULL;
    }
    value = pbw_value_new_message(nested);
    if (value == NULL) {
        pbw_message_free(nested);
        return NULL;
    }
    if (pbw_message_set(msg, number, value) != PBW_OK) {
        pbw_value_free(value);
        return NULL;
    }
    return nested;
}
