#include "kvs_lru.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  bool modified;
  struct cache_entry* prev;
  struct cache_entry* next;
} cache_entry_t;

struct kvs_lru {
  kvs_base_t* kvs_base;
  int capacity;
  int size;
  cache_entry_t* head;
  cache_entry_t* tail;
  cache_entry_t* table;
};

static void move_to_head(kvs_lru_t* kvs_lru, cache_entry_t* entry) {
  if (entry == kvs_lru->head) {
    return;
  }
  if (entry->prev) {
    entry->prev->next = entry->next;
  }
  if (entry->next) {
    entry->next->prev = entry->prev;
  }
  if (entry == kvs_lru->tail) {
    kvs_lru->tail = entry->prev;
  }
  entry->next = kvs_lru->head;
  entry->prev = NULL;
  if (kvs_lru->head) {
    kvs_lru->head->prev = entry;
  }
  kvs_lru->head = entry;
  if (!kvs_lru->tail) {
    kvs_lru->tail = kvs_lru->head;
  }
}

static void remove_tail(kvs_lru_t* kvs_lru) {
  if (kvs_lru->tail) {
    if (kvs_lru->tail->modified) {
      kvs_base_set(kvs_lru->kvs_base, kvs_lru->tail->key, kvs_lru->tail->value);
    }
    if (kvs_lru->tail->prev) {
      kvs_lru->tail->prev->next = NULL;
    }
    cache_entry_t* old_tail = kvs_lru->tail;
    kvs_lru->tail = kvs_lru->tail->prev;
    free(old_tail);
    kvs_lru->size--;
  }
}

kvs_lru_t* kvs_lru_new(kvs_base_t* kvs, int capacity) {
  kvs_lru_t* kvs_lru = malloc(sizeof(kvs_lru_t));
  if (!kvs_lru) return NULL;

  kvs_lru->kvs_base = kvs;
  kvs_lru->capacity = capacity;
  kvs_lru->size = 0;
  kvs_lru->head = NULL;
  kvs_lru->tail = NULL;
  kvs_lru->table = calloc(capacity, sizeof(cache_entry_t));

  return kvs_lru;
}

void kvs_lru_free(kvs_lru_t** ptr) {
  if (ptr && *ptr) {
    cache_entry_t* current = (*ptr)->head;
    while (current) {
      cache_entry_t* next = current->next;
      free(current);
      current = next;
    }
    free((*ptr)->table);
    free(*ptr);
    *ptr = NULL;
  }
}

int kvs_lru_set(kvs_lru_t* kvs_lru, const char* key, const char* value) {
  for (cache_entry_t* entry = kvs_lru->head; entry; entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      strcpy(entry->value, value);
      entry->modified = true;
      move_to_head(kvs_lru, entry);
      return SUCCESS;
    }
  }

  if (kvs_lru->size == kvs_lru->capacity) {
    remove_tail(kvs_lru);
  }

  cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
  if (!new_entry) return FAILURE;
  strcpy(new_entry->key, key);
  strcpy(new_entry->value, value);
  new_entry->modified = true;
  new_entry->prev = NULL;
  new_entry->next = kvs_lru->head;

  if (kvs_lru->head) {
    kvs_lru->head->prev = new_entry;
  }
  kvs_lru->head = new_entry;
  if (!kvs_lru->tail) {
    kvs_lru->tail = new_entry;
  }
  kvs_lru->size++;

  return SUCCESS;
}

int kvs_lru_get(kvs_lru_t* kvs_lru, const char* key, char* value) {
  for (cache_entry_t* entry = kvs_lru->head; entry; entry = entry->next) {
    if (strcmp(entry->key, key) == 0) {
      strcpy(value, entry->value);
      move_to_head(kvs_lru, entry);
      return SUCCESS;
    }
  }

  int result = kvs_base_get(kvs_lru->kvs_base, key, value);
  if (result == SUCCESS) {
    if (kvs_lru->size == kvs_lru->capacity) {
      remove_tail(kvs_lru);
    }

    cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
    if (!new_entry) return FAILURE;
    strcpy(new_entry->key, key);
    strcpy(new_entry->value, value);
    new_entry->modified = false;
    new_entry->prev = NULL;
    new_entry->next = kvs_lru->head;

    if (kvs_lru->head) {
      kvs_lru->head->prev = new_entry;
    }
    kvs_lru->head = new_entry;
    if (!kvs_lru->tail) {
      kvs_lru->tail = new_entry;
    }
    kvs_lru->size++;
  }

  return result;
}

int kvs_lru_flush(kvs_lru_t* kvs_lru) {
  cache_entry_t* current = kvs_lru->head;
  while (current) {
    if (current->modified) {
      kvs_base_set(kvs_lru->kvs_base, current->key, current->value);
      current->modified = false;
    }
    current = current->next;
  }

  return SUCCESS;
}
