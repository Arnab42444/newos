#include <string.h>
#include <vm.h>
#include <khash.h>
#include <debug.h>

#define TOUCH(a) ((a) = (a))

#define malloc kmalloc
#define free kfree

struct hash_table {
	struct hash_elem **table;
	int next_ptr_offset;
	int table_size;
	int num_elems;
	int flags;
	int (*compare_func)(void *e, void *key);
	int (*hash_func)(void *e, void *key, int range);
};

struct hash_iterator {
	int bucket;
	void *ptr;
};	

// XXX gross hack
#define NEXT_ADDR(t, e) ((void *)(((unsigned int)(e)) + (t)->next_ptr_offset))
#define NEXT(t, e) ((void *)(*(int *)NEXT_ADDR(t, e)))
#define PUT_IN_NEXT(t, e, val) (*(int *)NEXT_ADDR(t, e) = (int)(val))

void *hash_init(int table_size, int next_ptr_offset,
	int compare_func(void *e, void *key),
	int hash_func(void *e, void *key, int range))
{
	struct hash_table *t;
	int i;
	
	t = (struct hash_table *)malloc(sizeof(struct hash_table));
	if(t == NULL) {
		return NULL;
	}

	t->table = (struct hash_elem **)malloc(sizeof(void *) * table_size);
	for(i = 0; i<table_size; i++)
		t->table[i] = NULL;
	t->table_size = table_size;
	t->next_ptr_offset = next_ptr_offset;
	t->flags = 0;
	t->num_elems = 0;
	t->compare_func = compare_func;
	t->hash_func = hash_func;

	return t;
}

int hash_uninit(void *_hash_table)
{
	struct hash_table *t = _hash_table;
	
#if 0
	if(t->num_elems > 0) {
		return -1;
	}
#endif	
	
	free(t->table);
	free(t);
	
	return 0;	
}

int hash_insert(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	int hash;

	hash = t->hash_func(e, NULL, t->table_size);
	PUT_IN_NEXT(t, e, t->table[hash]);
	t->table[hash] = e;
	t->num_elems++;

	return 0;
}

int hash_remove(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	void *i, *last_i;
	int hash;

	hash = t->hash_func(e, NULL, t->table_size);
	last_i = NULL;
	for(i = t->table[hash]; i != NULL; last_i = i, i = NEXT(t, i)) {
		if(i == e) {
			if(last_i != NULL)
				PUT_IN_NEXT(t, last_i, NEXT(t, i));
			else
				t->table[hash] = NEXT(t, i);
			t->num_elems--;
			return 0;
		}
	}

	return -1;
}

void *hash_find(void *_hash_table, void *e)
{
	struct hash_table *t = _hash_table;
	void *i;
	int hash;

	hash = t->hash_func(e, NULL, t->table_size);
	for(i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if(i == e) {
			return i;
		}
	}

	return NULL;
}

void *hash_lookup(void *_hash_table, void *key)
{
	struct hash_table *t = _hash_table;
	void *i;
	int hash;

	if(t->compare_func == NULL)
		return NULL;

	hash = t->hash_func(NULL, key, t->table_size);
	for(i = t->table[hash]; i != NULL; i = NEXT(t, i)) {
		if(t->compare_func(i, key) == 0) {
			return i;
		}
	}

	return NULL;
}

void *hash_open(void *_hash_table)
{
	struct hash_table *t = _hash_table;
	struct hash_iterator *i;

	i = (struct hash_iterator *)malloc(sizeof(struct hash_iterator));
	if(i == NULL)
		return NULL;

	hash_rewind(t, i);

	return i;
}

void hash_close(void *_hash_table, void *_iterator)
{
	struct hash_iterator *i = _iterator;
	TOUCH(_hash_table);
	
	free(i);
}

void hash_rewind(void *_hash_table, void *_iterator)
{
	struct hash_table *t = _hash_table;
	struct hash_iterator *i = _iterator;
	int index;

	for(index = 0; index < t->table_size; index++) {
		if(t->table[index] != NULL) {
			i->bucket = index;
			i->ptr = t->table[index];
			return;
		}
	}

	i->bucket = t->table_size;
	i->ptr = NULL;
}

void *hash_next(void *_hash_table, void *_iterator)
{
	struct hash_table *t = _hash_table;
	struct hash_iterator *i = _iterator;
	void *e;

	e = NULL;
findnext:
	if(i->ptr != NULL) {
		e = i->ptr;
		if(NEXT(t, i->ptr) != NULL) {
			i->ptr = NEXT(t, i->ptr);
			return e;
		}
	}

	if(i->bucket >= t->table_size)
		return NULL;
	for(i->bucket++; i->bucket < t->table_size; i->bucket++) {
		if(t->table[i->bucket] != NULL) {
			i->ptr = t->table[i->bucket];
			if(e != NULL) 
				return e;
			goto findnext;
		}
	}
	i->ptr = NULL;

	return e;
}

