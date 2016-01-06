#ifndef libcool_internal_buffer_
#define libcool_internal_buffer_

struct c_buffer;
typedef struct c_buffer CBuffer;

CBuffer *cbuffer_Create();
void cbuffer_Delete(CBuffer **bufferP);

CBuffer *cbuffer_AppendInt(CBuffer *buffer, int x);
CBuffer *cbuffer_AppendBytes(CBuffer *buffer, size_t length, uint8_t bytes[length]);
CBuffer *cbuffer_AppendString(CBuffer *buffer, char *string);

#endif
