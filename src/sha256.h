#ifndef SHA256_H_
#define SHA256_H_

#define _GNU_SOURCE

#include "cbuff.h"
#include <stdint.h>

#define WORD_SIZE_BYTES sizeof(uint32_t)
#define WORD_SIZE_BITS (WORD_SIZE_BYTES << 3)
#define CHUNK_SIZE_BYTES 64
#define CHUNK_SIZE_WORDS (CHUNK_SIZE_BYTES/WORD_SIZE_BYTES)

/* rotates word W to the right by R bits */
#define RROT(W, R) (((W) >> (R)) | ((W) << (WORD_SIZE_BITS-(R))))

/* reverses buffer B containing L bytes */
#define REV_BYTES(B, L) \
	for(size_t ii=0; B+ii<B+L-ii-1; ++ii) \
		(*((uint8_t *) B+ii)	 ^= *((uint8_t *) B+L-ii-1), \
		 *((uint8_t *) B+L-ii-1) ^= *((uint8_t *) B+ii), \
		 *((uint8_t *) B+ii)	 ^= *((uint8_t *) B+L-ii-1))

/* reverses endianness of each word in buffer B containing NW words */
#define REV_ENDIAN(B, NW) \
	for (size_t jj=0; jj<(NW); ++jj) \
		REV_BYTES(B+jj*WORD_SIZE_BYTES, WORD_SIZE_BYTES)

/* copies NW words from B0 to buffer B1 in reverse byte order */
#define COPY_WORDS(B1, B0, NW) \
	{ memcpy(B1, B0, (NW)*WORD_SIZE_BYTES); \
	  REV_ENDIAN(B1, (NW)); } 

uint8_t *readFile(char *name, size_t *size);
uint8_t *prep(uint8_t *message, size_t *size);
uint8_t *process(uint8_t *message, size_t *size);
void expand_chunks(uint8_t *message, size_t *size, volatile CircularBuffer *cbuff);
void *loop_compress(void *args);

#endif /* SHA256_H_ */