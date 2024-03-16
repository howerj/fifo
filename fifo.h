#ifndef FIFO_H
#define FIFO_H

#define FIFO_AUTHOR "Richard James Howe"
#define FIFO_LICENSE "The Unlicense / Public Domain"
#define FIFO_EMAIL "howe.r.j.89@gmail.com"
#define FIFO_REPO "https://github.com/howerj/fifo"
#define FIFO_PROJECT "A simple multipurpose FIFO"
#define FIFO_VERSION "v1.0.0"

/* This is a basic FIFO library, there are quite a few things that could be
 * done to improve (or over complicate) the library. Things that could be
 * added:
 *
 * - Locking
 * - Growing / options to grow instead of wrapping around.
 * - Allow values to be overwritten when full with an option.
 * - Copying around buffers instead of handles (useful as a message queue).
 * - Using every bucked instead of `size - 1` buckets.
 * - Optional turn this into either a stack, or a priority queue, keeping
 *   the interface (largely) the same. 
 *
 * See the unit tests for example code.
 */

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <stdint.h>

#ifdef FIFO_DEFINE_MAIN /* If `main()` is defined make sure unit tests are as well */
#undef FIFO_UNIT_TEST
#define FIFO_UNIT_TEST (1)
#endif

#ifndef FIFO_UNIT_TEST /* Unit tests are included by default */
#define FIFO_UNIT_TEST (1)
#endif

#ifndef FIFO_EXTERN /* Applied to function declarations only, might need to change in tandem with `FIFO_API`. */
#define FIFO_EXTERN extern
#endif

#ifndef FIFO_API /* Applied to function definition (could use to make functions `static inline` for instance).  */
#define FIFO_API
#endif

typedef struct {
	size_t head, tail, size; /* You set `size`, leave `head` and `tail` alone */
	void **data; /* You allocate this */
} fifo_t;

FIFO_EXTERN int fifo_is_full(fifo_t *f);
FIFO_EXTERN size_t fifo_depth(fifo_t *f);
FIFO_EXTERN int fifo_is_empty(fifo_t *f);
FIFO_EXTERN void *fifo_pop(fifo_t *f); /* NULL on error */
FIFO_EXTERN void *fifo_peek(fifo_t *f); /* NULL on error */
FIFO_EXTERN int fifo_push(fifo_t *f, void *handle); /* Negative on error */
FIFO_EXTERN int fifo_foreach(fifo_t *f, int (*fn)(void *param, void *handle), void *param, int reverse);
FIFO_EXTERN int fifo_unit_tests(void); /* Negative on error */

#ifdef FIFO_IMPLEMENTATION

static inline void fifo_assert(fifo_t *f) {
	assert(f);
	assert(f->data);
	assert(f->head < f->size);
	assert(f->tail < f->size);
	assert(f->size < (SIZE_MAX - 1));
}

FIFO_API int fifo_is_full(fifo_t *f) {
	fifo_assert(f);
	return ((f->head + 1) % f->size) == f->tail;
}

FIFO_API size_t fifo_depth(fifo_t *f) {
	fifo_assert(f);
	return (f->head - f->tail) % f->size;
}

FIFO_API int fifo_is_empty(fifo_t *f) {
	fifo_assert(f);
	return f->tail == f->head;
}

FIFO_API void *fifo_pop(fifo_t *f) {
	fifo_assert(f);
	if (fifo_is_empty(f))
		return NULL;
	assert(f->tail < f->size);
	void *handle = f->data[f->tail];
	f->data[f->tail++] = NULL;
	f->tail %= f->size;
	return handle;
}

FIFO_API void *fifo_peek(fifo_t *f) {
	fifo_assert(f);
	if (fifo_is_empty(f))
		return NULL;
	assert(f->tail < f->size);
	return f->data[f->tail];
}

FIFO_API int fifo_push(fifo_t *f, void *handle) {
	fifo_assert(f);
	if (fifo_is_full(f))
		return -1;
	assert(f->head < f->size);
	f->data[f->head++] = handle;
	f->head %= f->size;
	return 0;
}

FIFO_API int fifo_foreach(fifo_t *f, int (*fn)(void *param, void *handle), void *param, int reverse) {
	fifo_assert(f);
	assert(fn);
	const size_t size = fifo_depth(f);
	for (size_t i = 0; i < size; i++) {
		const size_t index = reverse ? (f->head - i - 1) : f->tail + i;
		const int r = fn(param, f->data[index % f->size]);
		if (r) /* early return on non-zero, negative usually means an error */
			return r;
	}
	return 0;
} /* We could also make an iterator if we wanted... */

static inline int fifo_unit_test_foreach_fn(void *param, void *handle) {
	size_t *v = param;
	size_t r = (size_t)(uintptr_t)handle;
	*v += r;
	return 0;
}

#define FIFO_TEST_SZ (16)
FIFO_API int fifo_unit_tests(void) {
	if (!FIFO_UNIT_TEST)
		return 0;
	void *data[FIFO_TEST_SZ] = { 0, };
	fifo_t fifo = { 0, 0, FIFO_TEST_SZ, data, }, *f = &fifo;

	if (!fifo_is_empty(f))
		return -1;

	size_t pushed = 0, popped = 0;

	if (fifo_pop(f))
		return -1;

	for (size_t i = 0; i < FIFO_TEST_SZ; i++) {
		if (fifo_push(f, (void*)(i + 1)) < 0) {
			if (i == (FIFO_TEST_SZ - 1))
				break;
			return -1;
		}
		if (fifo_is_empty(f))
			return -1;
		pushed++;
	}
	if (!fifo_is_full(f))
		return -1;

	size_t sum = 0, n = FIFO_TEST_SZ, expect = (n * (n - 1)) / 2;
	if (fifo_foreach(f, fifo_unit_test_foreach_fn, &sum, 0) < 0)
		return -1;
	if (sum != expect)
		return -1;

	for (size_t i = 0; i < FIFO_TEST_SZ; i++) {
		void *p = fifo_peek(f);
		void *r = fifo_pop(f);
		if (p != r)
			return -1;
		if (!r)
			break;
		if ((i + 1) != (size_t)r)
			return -1;
		if (fifo_is_full(f))
			return -1;
		popped++;
	}

	if (!fifo_is_empty(f))
		return -1;
	if (fifo_is_full(f))
		return -1;

	if (pushed != popped)
		return -1;

	for (size_t i = 0; i < FIFO_TEST_SZ * 4; i++) {
		if (fifo_push(f, (void*)(i + 1)) < 0)
			return -1;
		void *r = fifo_pop(f);
		if (!r)
			return -1;
		if ((i + 1) != (size_t)r)
			return -1;
	}

	return 0;
}
#undef FIFO_TEST_SZ

#ifdef FIFO_DEFINE_MAIN
int main(void) {
	const char *msg ="\n\
Project: A simple multipurpose FIFO\n\
Version: v1.0.0\n\
Author:  Richard James Howe\n\
License: The Unlicense / Public Domain\n\
Email:   howe.r.j.89@gmail.com\n\
Repo:    https://github.com/howerj/fifo\n\
";
	if (puts(msg) < 0)
		return 1;
	const int r = fifo_unit_tests();
	if (printf("FIFO UNIT TESTS %s\n", r < 0 ? "FAIL" : "PASS") < 0)
		return 1;
	return r < 0;
}
#endif
#endif
#endif

