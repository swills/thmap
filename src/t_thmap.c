/*
 * Copyright (c) 2017 Mindaugas Rasiukevicius <rmind at noxt eu>
 * All rights reserved.
 *
 * Use is subject to license terms, as specified in the LICENSE file.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <inttypes.h>
#include <assert.h>

#include "thmap.h"

#define	NUM2PTR(x)	((void *)(uintptr_t)(x))

static void
test_basic(void)
{
	thmap_t *hmap;
	void *ret;

	hmap = thmap_create(0, NULL, 0);
	assert(hmap != NULL);

	ret = thmap_get(hmap, "test", 4);
	assert(ret == NULL);

	ret = thmap_put(hmap, "test", 4, NUM2PTR(0x55));
	assert(ret == NUM2PTR(0x55));

	ret = thmap_get(hmap, "test", 4);
	assert(ret == NUM2PTR(0x55));

	ret = thmap_del(hmap, "test", 4);
	assert(ret == NUM2PTR(0x55));

	ret = thmap_get(hmap, "test", 4);
	assert(ret == NULL);

	thmap_destroy(hmap);
}

static void
test_large(void)
{
	const unsigned nitems = 1024 * 1024;
	thmap_t *hmap;
	void *ret;

	hmap = thmap_create(0, NULL, 0);
	assert(hmap != NULL);

	for (unsigned i = 0; i < nitems; i++) {
		ret = thmap_put(hmap, &i, sizeof(int), NUM2PTR(i));
		assert(ret == NUM2PTR(i));

		ret = thmap_get(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		ret = thmap_get(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		ret = thmap_del(hmap, &i, sizeof(int));
		assert(ret == NUM2PTR(i));

		ret = thmap_get(hmap, &i, sizeof(int));
		assert(ret == NULL);
	}

	thmap_destroy(hmap);
}

static void
test_delete(void)
{
	const unsigned nitems = 300;
	thmap_t *hmap;
	uint64_t *keys;
	void *ret;

	hmap = thmap_create(0, NULL, 0);
	assert(hmap != NULL);

	keys = calloc(nitems, sizeof(uint64_t));
	assert(keys != NULL);

	for (unsigned i = 0; i < nitems; i++) {
		keys[i] = random() + 1;
		ret = thmap_put(hmap, &keys[i], sizeof(uint64_t), NUM2PTR(i));
		assert(ret == NUM2PTR(i));
	}

	for (unsigned i = 0; i < nitems; i++) {
		/* Delete a key. */
		ret = thmap_del(hmap, &keys[i], sizeof(uint64_t));
		assert(ret == NUM2PTR(i));

		/* Check the remaining keys. */
		for (unsigned j = i + 1; j < nitems; j++) {
			ret = thmap_get(hmap, &keys[j], sizeof(uint64_t));
			assert(ret == NUM2PTR(j));
		}
	}
	thmap_destroy(hmap);
	free(keys);
}

static void *
generate_unique_key(unsigned idx, int *rlen)
{
	const unsigned rndlen = random() % 32;
	const unsigned len = rndlen + sizeof(idx);
	unsigned char *key = malloc(len);

	for (unsigned i = 0; i < rndlen; i++) {
		key[i] = random() % 0xff;
	}
	memcpy(&key[rndlen], &idx, sizeof(idx));
	*rlen = len;
	return key;
}

#define	KEY_MAGIC_VAL(k)	NUM2PTR(((unsigned char *)(k))[0] ^ 0x55)

static void
test_random(void)
{
	const unsigned nitems = 300;
	unsigned n = 10 * 1000 * 1000;
	thmap_t *hmap;
	unsigned char **keys;
	int *lens;

	hmap = thmap_create(0, NULL, 0);
	assert(hmap != NULL);

	keys = calloc(nitems, sizeof(unsigned char *));
	assert(keys != NULL);

	lens = calloc(nitems, sizeof(int));
	assert(lens != NULL);

	srandom(1);
	while (n--) {
		const unsigned i = random() % nitems;
		void *val = keys[i] ? KEY_MAGIC_VAL(keys[i]) : NULL;
		void *ret;

		switch (random() % 3) {
		case 0:
			/* Create a unique random key. */
			if (keys[i] == NULL) {
				keys[i] = generate_unique_key(i, &lens[i]);
				val = KEY_MAGIC_VAL(keys[i]);
				ret = thmap_put(hmap, keys[i], lens[i], val);
				assert(ret == val);
			}
			break;
		case 1:
			/* Lookup a key. */
			if (keys[i]) {
				ret = thmap_get(hmap, keys[i], lens[i]);
				assert(ret == val);
			}
			break;
		case 2:
			/* Delete a key. */
			if (keys[i]) {
				ret = thmap_del(hmap, keys[i], lens[i]);
				assert(ret == val);
				free(keys[i]);
				keys[i] = NULL;
			}
			break;
		}
	}

	for (unsigned i = 0; i < nitems; i++) {
		if (keys[i]) {
			void *ret = thmap_del(hmap, keys[i], lens[i]);
			assert(KEY_MAGIC_VAL(keys[i]) == ret);
			free(keys[i]);
		}
	}

	thmap_destroy(hmap);
	free(keys);
	free(lens);
}

int
main(void)
{
	test_basic();
	test_large();
	test_delete();
	test_random();
	puts("ok");
	return 0;
}