#include "sha256.h"
#include "cbuff.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/stat.h>
#include <sys/mman.h>

uint8_t *readFile(char *name, size_t *size) {
	int fd = open(name, O_RDONLY);

	struct stat st;
	stat(name, &st);
	*size = st.st_size;

	return mmap(NULL, *size, PROT_READ|PROT_WRITE, MAP_PRIVATE, fd, 0);
}

uint8_t *prep(uint8_t *message, size_t *size) {
	uint64_t init_size = *size;
	uint64_t init_size_bits = init_size << 3;
	size_t padding = (512 - ((8+init_size_bits+64) & 0x1ff)) >> 3;
	*size = init_size+1+padding+sizeof(uint64_t);

	message = mremap(message, init_size, *size, MREMAP_MAYMOVE);
	memset(message+init_size, 0x80, 1);
	memset(message+init_size+1, 0x0, padding);
	memcpy(message+*size-sizeof(uint64_t), &init_size_bits, sizeof(uint64_t));

	REV_BYTES(message+*size-sizeof(uint64_t), sizeof(uint64_t));
    REV_BYTES(message, *size);

	return message;
}

uint8_t *process(uint8_t *message, size_t *size) {
	CircularBuffer *cbuff = cbuff_create(8192);
	size_t n_chunks = *size / CHUNK_SIZE_BYTES;

	void *args[] = {(void *) cbuff, (void *) &n_chunks};
	pthread_t comp_thread;

	pthread_create(&comp_thread, NULL, &loop_compress, (void *) args);
	expand_chunks(message, size, cbuff);
	pthread_join(comp_thread, (void **) &message);

	*size = 8 * WORD_SIZE_BYTES;
	return message;
}

void expand_chunks(uint8_t *message, size_t *size, volatile CircularBuffer *cbuff) {
	size_t prev_size = *size;
 	uint8_t *start;
 	uint32_t *w, s0, s1;

	for (size_t count = 0; *size > 0; count++) {
		while (cbuff->buffer[cbuff->tail] != NULL);

		start = (uint8_t *) message+*size;
		w = malloc(CHUNK_SIZE_BYTES * WORD_SIZE_BYTES);

		COPY_WORDS(w, start-CHUNK_SIZE_BYTES, CHUNK_SIZE_WORDS);
		REV_BYTES(w, CHUNK_SIZE_BYTES);
		*size -= CHUNK_SIZE_BYTES;

		for (size_t i = 16; i < CHUNK_SIZE_BYTES; ++i) {
			s0 = RROT(w[i - 15], 7) ^ RROT(w[i - 15], 18) ^ (w[i - 15] >> 3);
			s1 = RROT(w[i - 2], 17) ^ RROT(w[i - 2], 19) ^ (w[i - 2] >> 10);
			w[i] = w[i - 16] + s0 + w[i - 7] + s1;
		}

		cbuff->buffer[cbuff->tail] = w;
		cbuff->tail = (cbuff->tail+1) & (cbuff->buff_size-1);

		if ((count & 0xf) == 0) {
			mremap(start, prev_size, *size, MREMAP_FIXED);
			prev_size = *size;
		}
	}
}

void *loop_compress(void *args) {
	uint32_t h[] = {
		0x6a09e667, 0xbb67ae85, 0x3c6ef372, 0xa54ff53a, 0x510e527f, 0x9b05688c, 0x1f83d9ab, 0x5be0cd19
	};

	uint32_t k[] = {
		0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
		0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
		0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
		0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
		0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
		0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
		0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
		0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
	};

    volatile CircularBuffer *cbuff = ((void **) args)[0];
    size_t n_chunks = *((size_t *) ((void **) args)[1]);
    
	volatile uint32_t *w;
	uint32_t a[8], S0, S1, ch, temp1, temp2, maj;
	size_t i, j, count = 0;	

	while (count < n_chunks) {
		for (; cbuff->buffer[cbuff->head] != NULL; ((cbuff->head = (cbuff->head + 1) & (cbuff->buff_size-1)), count++)) {
			w = cbuff->buffer[cbuff->head];

			for (i = 0; i < 8; ++i)
				a[i] = h[i];

			for (i = 0; i < CHUNK_SIZE_BYTES; ++i) {
				S1 = RROT(a[4], 6) ^ RROT(a[4], 11) ^ RROT(a[4], 25);
				ch = (a[4] & a[5]) | (~a[4] & a[6]);
				temp1 = a[7] + S1 + ch + k[i] + w[i];
				S0 = RROT(a[0], 2) ^ RROT(a[0], 13) ^ RROT(a[0], 22);
				maj = (a[0] & a[1]) | (a[2] & (a[0] | a[1]));
				temp2 = S0 + maj;

				for (j = 8; j > 0; --j) {
					if (j == 1)
						a[j-1] = temp1 + temp2;
					else if (j == 5)
						a[j-1] = a[j-2] + temp1;
					else
						a[j-1] = a[j-2];
				}
			}

			free((void *) cbuff->buffer[cbuff->head]);
			cbuff->buffer[cbuff->head] = NULL;

			for (i = 0; i < 8; ++i)
				h[i] += a[i];
		}
	}

	uint8_t *message = malloc(8 * WORD_SIZE_BYTES);
	COPY_WORDS(message, h, sizeof(h)/WORD_SIZE_BYTES);

	return message;
}

int main(int argc, char **argv) {
	size_t size;
	uint8_t *message;

	message = readFile(argv[1], &size);
	message = prep(message, &size);
	message = process(message, &size);

	for (size_t i = 0; i < size; ++i)
		fprintf(stdout, "%02x", message[i]);
	putc('\n', stdout);

	return 0;
}