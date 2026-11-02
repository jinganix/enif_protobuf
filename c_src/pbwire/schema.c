/* Descriptor model: schema registry, field metadata, name resolution, lookup
 * tables. */

#include "internal.h"

#include <stdio.h>

static char *
dup_str(const char *s)
{
    size_t len;
    char *copy;

    if (s == NULL) {
        return NULL;
    }
    len = strlen(s) + 1;
    copy = (char *)malloc(len);
    if (copy != NULL) {
        memcpy(copy, s, len);
    }
    return copy;
}

void
pbw_descriptor_free(pbw_descriptor *desc)
{
    size_t i;

    if (desc == NULL) {
        return;
    }
    for (i = 0; i < desc->field_count; i++) {
        free(desc->fields[i].name);
        free(desc->fields[i].type_name);
    }
    free(desc->fields);
    for (i = 0; i < desc->enum_count; i++) {
        free(desc->enum_values[i].name);
    }
    free(desc->enum_values);
    free(desc->fnum_map);
    free(desc->name);
    free(desc);
}

pbw_schema *
pbw_schema_new(void)
{
    return (pbw_schema *)calloc(1, sizeof(pbw_schema));
}

void
pbw_schema_free(pbw_schema *schema)
{
    size_t i;

    if (schema == NULL) {
        return;
    }
    for (i = 0; i < schema->count; i++) {
        pbw_descriptor_free(schema->descs[i]);
    }
    free(schema->descs);
    free(schema);
}

static pbw_descriptor *
add_descriptor(pbw_schema *schema, const char *name,
               int is_enum)
{
    pbw_descriptor *desc;
    pbw_descriptor **descs;

    if (schema == NULL || name == NULL || pbw_schema_find(schema, name) != NULL) {
        return NULL;
    }
    if (schema->count == schema->cap) {
        size_t cap = schema->cap ? schema->cap * 2 : 8;
        descs = (pbw_descriptor **)realloc(schema->descs, cap * sizeof(*descs));
        if (descs == NULL) {
            return NULL;
        }
        schema->descs = descs;
        schema->cap = cap;
    }
    desc = (pbw_descriptor *)calloc(1, sizeof(*desc));
    if (desc == NULL) {
        return NULL;
    }
    desc->name = dup_str(name);
    if (desc->name == NULL) {
        free(desc);
        return NULL;
    }
    desc->is_enum = is_enum;
    desc->schema = schema;
    schema->descs[schema->count++] = desc;
    schema->linked = 0;
    return desc;
}

pbw_descriptor *
pbw_schema_add_message(pbw_schema *schema, const char *name)
{
    return add_descriptor(schema, name, 0);
}

pbw_descriptor *
pbw_schema_add_enum(pbw_schema *schema, const char *name)
{
    return add_descriptor(schema, name, 1);
}

pbw_descriptor *
pbw_schema_find(const pbw_schema *schema, const char *name)
{
    size_t i;

    if (schema == NULL || name == NULL) {
        return NULL;
    }
    for (i = 0; i < schema->count; i++) {
        if (strcmp(schema->descs[i]->name, name) == 0) {
            return schema->descs[i];
        }
    }
    return NULL;
}

void
pbw_enum_add_value(pbw_descriptor *enum_desc, const char *name,
                   int32_t number)
{
    struct pbw_enum_value *values;
    char *copy;

    if (enum_desc == NULL || !enum_desc->is_enum || name == NULL) {
        return;
    }
    copy = dup_str(name);
    if (copy == NULL) {
        return;
    }
    values = (struct pbw_enum_value *)realloc(
        enum_desc->enum_values, (enum_desc->enum_count + 1) * sizeof(*values));
    if (values == NULL) {
        free(copy);
        return;
    }
    enum_desc->enum_values = values;
    values[enum_desc->enum_count].name = copy;
    values[enum_desc->enum_count].number = number;
    enum_desc->enum_count++;
}

int
pbw_enum_find_name(const pbw_descriptor *enum_desc, int32_t number,
                   const char **name)
{
    size_t i;

    if (enum_desc == NULL || !enum_desc->is_enum) {
        return 0;
    }
    for (i = 0; i < enum_desc->enum_count; i++) {
        if (enum_desc->enum_values[i].number == number) {
            if (name != NULL) {
                *name = enum_desc->enum_values[i].name;
            }
            return 1;
        }
    }
    return 0;
}

int
pbw_enum_find_value(const pbw_descriptor *enum_desc, const char *name,
                    int32_t *number)
{
    size_t i;

    if (enum_desc == NULL || !enum_desc->is_enum || name == NULL) {
        return 0;
    }
    for (i = 0; i < enum_desc->enum_count; i++) {
        if (strcmp(enum_desc->enum_values[i].name, name) == 0) {
            if (number != NULL) {
                *number = enum_desc->enum_values[i].number;
            }
            return 1;
        }
    }
    return 0;
}

pbw_field *
pbw_message_add_field(pbw_descriptor *msg, const char *name,
                      uint32_t number, pbw_type type,
                      pbw_label label)
{
    pbw_field *fields;
    pbw_field *field;
    size_t i;

    if (msg == NULL || msg->is_enum || name == NULL) {
        return NULL;
    }
    if (number == 0 || number > PBW_MAX_FIELD_NUMBER) {
        return NULL;
    }
    for (i = 0; i < msg->field_count; i++) {
        if (msg->fields[i].number == number ||
            strcmp(msg->fields[i].name, name) == 0) {
            return NULL;
        }
    }
    fields = (pbw_field *)realloc(msg->fields,
                                  (msg->field_count + 1) * sizeof(*fields));
    if (fields == NULL) {
        return NULL;
    }
    msg->fields = fields;
    field = &fields[msg->field_count];
    memset(field, 0, sizeof(*field));
    field->name = dup_str(name);
    if (field->name == NULL) {
        return NULL;
    }
    field->schema = msg->schema;
    field->number = number;
    field->type = type;
    field->label = label;
    msg->field_count++;
    msg->schema->linked = 0;
    return field;
}

void
pbw_field_set_type_name(pbw_field *field, const char *type_name)
{
    char *copy;

    if (field == NULL) {
        return;
    }
    copy = dup_str(type_name);
    if (type_name != NULL && copy == NULL) {
        return;
    }
    free(field->type_name);
    field->type_name = copy;
    field->sub = NULL;
    if (field->schema != NULL) {
        field->schema->linked = 0;
    }
}

void
pbw_field_set_packed(pbw_field *field, int packed)
{
    if (field != NULL) {
        field->packed = packed ? 1u : 0u;
        if (field->schema != NULL) {
            field->schema->linked = 0;
        }
    }
}

void
pbw_field_set_oneof(pbw_field *field, uint32_t oneof_index)
{
    if (field != NULL) {
        field->oneof_index = oneof_index;
        if (field->schema != NULL) {
            field->schema->linked = 0;
        }
    }
}

static int
compare_fields(const void *a, const void *b)
{
    const pbw_field *fa = (const pbw_field *)a;
    const pbw_field *fb = (const pbw_field *)b;

    if (fa->number < fb->number) {
        return -1;
    }
    return fa->number > fb->number ? 1 : 0;
}

static pbw_status
link_message(pbw_schema *schema, pbw_descriptor *msg)
{
    size_t i;
    uint32_t max_fnum = 0;

    for (i = 0; i < msg->field_count; i++) {
        pbw_field *field = &msg->fields[i];

        if (field->type == PBW_TYPE_MESSAGE || field->type == PBW_TYPE_ENUM) {
            pbw_descriptor *sub;

            if (field->type_name == NULL) {
                snprintf(schema->error, sizeof(schema->error),
                         "field %s.%s needs a type name", msg->name, field->name);
                return PBW_ERR_INVALID;
            }
            sub = pbw_schema_find(schema, field->type_name);
            if (sub == NULL) {
                snprintf(schema->error, sizeof(schema->error),
                         "unknown type %s for field %s.%s", field->type_name, msg->name,
                         field->name);
                return PBW_ERR_NOT_FOUND;
            }
            if (sub->is_enum != (field->type == PBW_TYPE_ENUM)) {
                snprintf(schema->error, sizeof(schema->error),
                         "type %s does not match field %s.%s", field->type_name,
                         msg->name, field->name);
                return PBW_ERR_TYPE;
            }
            field->sub = sub;
        } else if (field->type == PBW_TYPE_GROUP) {
            snprintf(schema->error, sizeof(schema->error),
                     "group field %s.%s is not supported", msg->name, field->name);
            return PBW_ERR_TYPE;
        }
        if (field->packed &&
            (field->label != PBW_LABEL_REPEATED || !pbw_is_packable(field->type))) {
            snprintf(schema->error, sizeof(schema->error),
                     "field %s.%s cannot be packed", msg->name, field->name);
            return PBW_ERR_INVALID;
        }
        if (field->oneof_index > msg->oneof_count) {
            msg->oneof_count = field->oneof_index;
        }
        if (field->number > max_fnum) {
            max_fnum = field->number;
        }
    }

    qsort(msg->fields, msg->field_count, sizeof(*msg->fields), compare_fields);
    for (i = 1; i < msg->field_count; i++) {
        if (msg->fields[i - 1].number == msg->fields[i].number) {
            snprintf(schema->error, sizeof(schema->error),
                     "duplicate field number %u in %s", msg->fields[i].number,
                     msg->name);
            return PBW_ERR_INVALID;
        }
    }

    /* Dense field-number lookup table for the decoder; sparse messages use
     * bsearch. */
    free(msg->fnum_map);
    msg->fnum_map = NULL;
    msg->fnum_map_size = 0;
    if (max_fnum <= PBW_FNUM_MAP_LIMIT &&
        max_fnum <= (uint32_t)(msg->field_count * 8 + 8)) {
        uint32_t *map = (uint32_t *)calloc(max_fnum + 1, sizeof(*map));
        if (map == NULL) {
            return PBW_ERR_NOMEM;
        }
        for (i = 0; i < msg->field_count; i++) {
            map[msg->fields[i].number] = (uint32_t)(i + 1);
        }
        msg->fnum_map = map;
        msg->fnum_map_size = max_fnum + 1;
    }
    return PBW_OK;
}

pbw_status
pbw_schema_link(pbw_schema *schema)
{
    size_t i;
    pbw_status status;

    if (schema == NULL) {
        return PBW_ERR_INVALID;
    }
    if (schema->linked) {
        return PBW_OK;
    }
    schema->error[0] = '\0';
    for (i = 0; i < schema->count; i++) {
        if (schema->descs[i]->is_enum) {
            continue;
        }
        status = link_message(schema, schema->descs[i]);
        if (status != PBW_OK) {
            return status;
        }
    }
    schema->linked = 1;
    return PBW_OK;
}

const char *
pbw_schema_error(const pbw_schema *schema)
{
    return schema == NULL ? "" : schema->error;
}

const char *
pbw_descriptor_name(const pbw_descriptor *desc)
{
    return desc == NULL ? NULL : desc->name;
}

int
pbw_descriptor_is_enum(const pbw_descriptor *desc)
{
    return desc != NULL && desc->is_enum;
}

size_t
pbw_descriptor_field_count(const pbw_descriptor *desc)
{
    return desc == NULL ? 0 : desc->field_count;
}

const pbw_field *
pbw_descriptor_field_at(const pbw_descriptor *desc,
                        size_t index)
{
    if (desc == NULL || index >= desc->field_count) {
        return NULL;
    }
    return &desc->fields[index];
}

pbw_field *
pbw_descriptor_field_mut(const pbw_descriptor *desc, size_t index)
{
    if (desc == NULL || index >= desc->field_count) {
        return NULL;
    }
    return &desc->fields[index];
}

const pbw_field *
pbw_descriptor_find_field(const pbw_descriptor *desc, uint32_t number)
{
    if (desc == NULL || desc->is_enum ||
        (desc->schema != NULL && !desc->schema->linked)) {
        return NULL;
    }
    return pbw_find_field(desc, number);
}

size_t
pbw_descriptor_field_index(const pbw_descriptor *desc, uint32_t number)
{
    const pbw_field *field = pbw_descriptor_find_field(desc, number);

    return field == NULL ? (size_t)-1 : (size_t)(field - desc->fields);
}

const pbw_field *
pbw_descriptor_find_field_name(const pbw_descriptor *desc,
                               const char *name)
{
    size_t i;

    if (desc == NULL || desc->is_enum || name == NULL) {
        return NULL;
    }
    for (i = 0; i < desc->field_count; i++) {
        if (strcmp(desc->fields[i].name, name) == 0) {
            return &desc->fields[i];
        }
    }
    return NULL;
}

const char *
pbw_field_name(const pbw_field *field)
{
    return field == NULL ? NULL : field->name;
}

uint32_t
pbw_field_number(const pbw_field *field)
{
    return field == NULL ? 0 : field->number;
}

pbw_type
pbw_field_type(const pbw_field *field)
{
    return field == NULL ? PBW_TYPE_INT32 : field->type;
}

pbw_label
pbw_field_label(const pbw_field *field)
{
    return field == NULL ? PBW_LABEL_OPTIONAL : field->label;
}

int
pbw_field_is_packed(const pbw_field *field)
{
    return field != NULL && field->packed != 0;
}

uint32_t
pbw_field_oneof_index(const pbw_field *field)
{
    return field == NULL ? 0 : field->oneof_index;
}

const pbw_descriptor *
pbw_field_message_type(const pbw_field *field)
{
    return field == NULL ? NULL : field->sub;
}
