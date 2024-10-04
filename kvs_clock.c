#include "kvs_clock.h"

#include <stdlib.h>
#include <string.h>

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  int reference_bit;
  int modified;
} cache_entry_t;

struct kvs_clock {
  kvs_base_t* kvs_base;
  int capacity;
  cache_entry_t* entries;
  int count;
  int cursor;
};

kvs_clock_t* kvs_clock_new(kvs_base_t* kvs, int capacity) {
  kvs_clock_t* kvs_clock = malloc(sizeof(kvs_clock_t));
  kvs_clock->kvs_base = kvs;
  kvs_clock->capacity = capacity;
  kvs_clock->entries = calloc(capacity, sizeof(cache_entry_t));
  kvs_clock->count = 0;
  kvs_clock->cursor = 0;
  return kvs_clock;
}

void kvs_clock_free(kvs_clock_t** ptr) {
  kvs_clock_t* kvs_clock = *ptr;
  free(kvs_clock->entries);
  free(kvs_clock);
  *ptr = NULL;
}

int kvs_clock_set(kvs_clock_t* kvs_clock, const char* key, const char* value) {
  for (int i = 0; i < kvs_clock->count; ++i) {
    if (strcmp(kvs_clock->entries[i].key, key) == 0) {
      strcpy(kvs_clock->entries[i].value, value);
      kvs_clock->entries[i].reference_bit = 1;
      kvs_clock->entries[i].modified = 1;
      return SUCCESS;
    }
  }

  if (kvs_clock->count < kvs_clock->capacity) {
    strcpy(kvs_clock->entries[kvs_clock->count].key, key);
    strcpy(kvs_clock->entries[kvs_clock->count].value, value);
    kvs_clock->entries[kvs_clock->count].reference_bit = 1;
    kvs_clock->entries[kvs_clock->count].modified = 1;
    kvs_clock->count++;
    return SUCCESS;
  }

  while (kvs_clock->entries[kvs_clock->cursor].reference_bit == 1) {
    kvs_clock->entries[kvs_clock->cursor].reference_bit = 0;
    kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
  }

  if (kvs_clock->entries[kvs_clock->cursor].modified) {
    kvs_base_set(kvs_clock->kvs_base, kvs_clock->entries[kvs_clock->cursor].key,
                 kvs_clock->entries[kvs_clock->cursor].value);
  }
  strcpy(kvs_clock->entries[kvs_clock->cursor].key, key);
  strcpy(kvs_clock->entries[kvs_clock->cursor].value, value);
  kvs_clock->entries[kvs_clock->cursor].reference_bit = 1;
  kvs_clock->entries[kvs_clock->cursor].modified = 1;
  kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;

  return SUCCESS;
}

int kvs_clock_get(kvs_clock_t* kvs_clock, const char* key, char* value) {
  for (int i = 0; i < kvs_clock->count; ++i) {
    if (strcmp(kvs_clock->entries[i].key, key) == 0) {
      strcpy(value, kvs_clock->entries[i].value);
      kvs_clock->entries[i].reference_bit = 1;
      return SUCCESS;
    }
  }

  if (kvs_base_get(kvs_clock->kvs_base, key, value) == FAILURE) {
    return FAILURE;
  }

  if (kvs_clock->count < kvs_clock->capacity) {
    strcpy(kvs_clock->entries[kvs_clock->count].key, key);
    strcpy(kvs_clock->entries[kvs_clock->count].value, value);
    kvs_clock->entries[kvs_clock->count].reference_bit = 1;
    kvs_clock->entries[kvs_clock->count].modified = 0;
    kvs_clock->count++;
  } else {
    while (kvs_clock->entries[kvs_clock->cursor].reference_bit == 1) {
      kvs_clock->entries[kvs_clock->cursor].reference_bit = 0;
      kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
    }

    if (kvs_clock->entries[kvs_clock->cursor].modified) {
      kvs_base_set(kvs_clock->kvs_base,
                   kvs_clock->entries[kvs_clock->cursor].key,
                   kvs_clock->entries[kvs_clock->cursor].value);
    }
    strcpy(kvs_clock->entries[kvs_clock->cursor].key, key);
    strcpy(kvs_clock->entries[kvs_clock->cursor].value, value);
    kvs_clock->entries[kvs_clock->cursor].reference_bit = 1;
    kvs_clock->entries[kvs_clock->cursor].modified = 0;
    kvs_clock->cursor = (kvs_clock->cursor + 1) % kvs_clock->capacity;
  }

  return SUCCESS;
}

int kvs_clock_flush(kvs_clock_t* kvs_clock) {
  for (int i = 0; i < kvs_clock->count; ++i) {
    if (kvs_clock->entries[i].modified) {
      if (kvs_base_set(kvs_clock->kvs_base, kvs_clock->entries[i].key,
                       kvs_clock->entries[i].value) == FAILURE) {
        return FAILURE;
      }
      kvs_clock->entries[i].modified = 0;
    }
  }
  return SUCCESS;
}
