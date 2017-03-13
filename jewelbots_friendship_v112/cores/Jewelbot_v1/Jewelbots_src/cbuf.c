#include <stdint.h>
#include <string.h>
#include "nrf_log.h"
#include "cbuf.h"

#define SENTINEL_VALUE 0xEE
static bool item_exists(cbuf_t *c, uint8_t data) {
  uint8_t maxLength = c->maxLength;
  for (uint8_t i = 0; i < maxLength; i++) {
    if (c->buffer[i] == data) {
      return true;
    }
  }
  return false;
}
static void add(cbuf_t *c, uint8_t data, uint8_t next) {
  if (!item_exists(c, data)) {
    c->buffer[c->head] = data;
    c->head = next;
  }
}
c_buf_code_t c_buf_init(cbuf_t *c, uint8_t data) {
  uint8_t next = c->head + 1;
  if (next >= c->maxLength) {
    next = 0;
  }
  if (next == c->tail) {
    return BUF_FULL;
  }
  c->buffer[c->head] = data;
  c->head = next;
  return BUF_SUCCESS;
}
c_buf_code_t c_buf_push(cbuf_t *c, uint8_t data) {
  uint8_t next = c->head + 1;
  if (next >= c->maxLength) {
    next = 0;
  }
  if (next == c->tail) {
    return BUF_FULL;
  }
  add(c, data, next);
  return BUF_SUCCESS;
}
c_buf_code_t c_buf_pop(cbuf_t *c, uint8_t *data) {
  if (c->head == c->tail) {
    return BUF_NO_ELEMENTS;
  }
  *data = c->buffer[c->tail];
  c->buffer[c->tail] = SENTINEL_VALUE;

  uint8_t next = c->tail + 1;
  if (next >= c->maxLength) {
    next = 0;
  }
  c->tail = next;
  return BUF_SUCCESS;
}

/*
IN: 	cbuf_t c | Buffer
OUT: 	uint8_t num_of_elements |
OUT Pointer to array
*/

void get_elements_in_buffer(cbuf_t *c, buf_array_t *buffer_elements) {
  uint8_t maxLength = c->maxLength;
  uint8_t elements = 0;
  for (uint8_t i = 0; i < maxLength; i++) {
    if (c->buffer[i] != SENTINEL_VALUE) {
      buffer_elements->buffer[elements] = c->buffer[i];
      elements++;
    }
  }

  buffer_elements->buf_length = elements;
}
bool has_elements(cbuf_t *c) {
  return (c->head != c->tail);
}
