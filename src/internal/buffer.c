#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/resource.h>

#include "buffer.h"

struct c_buffer {
    size_t capacity;
    size_t offset;
    uint8_t *bytes;
};

static CBuffer *
_make_space(CBuffer *buffer, size_t length)
{
    if (buffer->offset + length > buffer->capacity) {
        buffer = realloc(buffer, sizeof(CBuffer) * ((buffer->offset + sizeof(int)) * 2));
        buffer->capacity = ((buffer->offset + length) * 2);
    }
    return buffer;
}

static CBuffer *
_add_data(CBuffer *buffer, size_t length, uint8_t *bytes)
{
    memcpy(buffer->bytes + buffer->offset, bytes, length);
    buffer->offset += length;
    return buffer;
}

CBuffer *
cbuffer_Create()
{
    CBuffer *buffer = (CBuffer *) malloc(sizeof(CBuffer));
    buffer->bytes = malloc(1);
    buffer->capacity = 1;
    buffer->offset = 0;
    return buffer;
}

void
cbuffer_Delete(CBuffer **bufferP)
{
    free((*bufferP)->bytes);
    free(*bufferP);
    *bufferP = NULL;
}

CBuffer *
cbuffer_AppendInt(CBuffer *buffer, int x)
{
    size_t length = sizeof(int);
    return _add_data(_make_space(buffer, length), length, (uint8_t *) &x);
}

CBuffer *
cbuffer_AppendBytes(CBuffer *buffer, size_t length, uint8_t bytes[length])
{
    return _add_data(_make_space(buffer, length), length, bytes);
}

CBuffer *
cbuffer_AppendString(CBuffer *buffer, char *string)
{
    size_t length = strlen(string);
    return _add_data(_make_space(buffer, length), length, (uint8_t *) string);
}
