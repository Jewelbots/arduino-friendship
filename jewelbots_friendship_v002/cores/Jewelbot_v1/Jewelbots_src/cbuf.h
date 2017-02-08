#ifndef CBUF_H_
#define CBUF_H_
#include "jewelbot_types.h"
#include <stdbool.h>
#include <stdint.h>
#define CIRCBUF_DEF(x, y)                                                      \
  uint8_t x##_space[y];                                                        \
  cbuf_t x = {x##_space, 0xAF, 0, y}

typedef struct {
  uint8_t *const buffer;
  int head;
  int tail;
  const int maxLength;
} cbuf_t;

typedef struct {
  uint8_t buffer[COLORS_UNLOCKED];
  uint8_t buf_length;
} buf_array_t;
enum c_buff_error_t { BUF_SUCCESS, BUF_ERROR, BUF_FULL, BUF_NO_ELEMENTS };

typedef uint8_t c_buf_code_t;

c_buf_code_t c_buf_push(cbuf_t *c, uint8_t data);
c_buf_code_t c_buf_pop(cbuf_t *c, uint8_t *data);
c_buf_code_t c_buf_init(cbuf_t *c, uint8_t data);
void get_elements_in_buffer(cbuf_t *c, buf_array_t *buffer_elements);
bool has_elements(cbuf_t *c);
// todo: add check if empty

#endif
