#ifndef CBUFF_H_
#define CBUFF_H_

#include <stdint.h>
#include <fcntl.h>

typedef struct {
	size_t head;
	size_t tail;
    size_t buff_size;
    volatile uint32_t **buffer;
} CircularBuffer;

/* 
** creates a circular buffer for holding expanded-but-unprocessed chunks.
** buff_size is rounded to the next power of 2 for efficient modular arithmetic.
*/ 
CircularBuffer *cbuff_create(size_t buff_size);

#endif /* CBUFF_H_ */