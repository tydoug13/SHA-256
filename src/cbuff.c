#include "cbuff.h"

CircularBuffer *cbuff_create(size_t buff_size) {
	CircularBuffer *cbuff = calloc(1, sizeof(CircularBuffer));

	size_t N = buff_size-1;
	size_t logN = 0;
	
	while (N > 0) {
		N = N >> 1;
		logN++;
	}

	cbuff->buff_size = 1 << logN;
	cbuff->buffer = calloc(cbuff->buff_size, sizeof(uint32_t *));

	return cbuff;
}