#include "kvs_fifo.h"

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "constants.h"

typedef struct cache_entry {
  char key[KVS_KEY_MAX];
  char value[KVS_VALUE_MAX];
  bool modified;
  struct cache_entry* next;
} cache_entry_t;

struct kvs_fifo {
  kvs_base_t* kvs_base;
  int capacity;
  int size;
  cache_entry_t* cache;
  cache_entry_t* front;
  cache_entry_t* rear;
};

kvs_fifo_t* kvs_fifo_new(kvs_base_t* kvs, int capacity) {
  kvs_fifo_t* kvs_fifo = malloc(sizeof(kvs_fifo_t));
  if (!kvs_fifo) return NULL;

  kvs_fifo->kvs_base = kvs;
  kvs_fifo->capacity = capacity;
  kvs_fifo->size = 0;
  kvs_fifo->cache = malloc(capacity * sizeof(cache_entry_t));
  kvs_fifo->front = NULL;
  kvs_fifo->rear = NULL;

  return kvs_fifo;
}

void kvs_fifo_free(kvs_fifo_t** ptr) {
  if (ptr && *ptr) {
    free((*ptr)->cache);
    free(*ptr);
    *ptr = NULL;
  }
}

cache_entry_t* find_cache_entry(kvs_fifo_t* kvs_fifo, const char* key) {
  cache_entry_t* current = kvs_fifo->front;
  while (current) {
    if (strcmp(current->key, key) == 0) {
      return current;
    }
    current = current->next;
  }
  return NULL;
}

int kvs_fifo_set(kvs_fifo_t* kvs_fifo, const char* key, const char* value) {
  cache_entry_t* existing_entry = find_cache_entry(kvs_fifo, key);

  if (existing_entry) {
    strcpy(existing_entry->value, value);
    existing_entry->modified = true;
    return SUCCESS;
  }

  if (kvs_fifo->size == kvs_fifo->capacity) {
    cache_entry_t* old_front = kvs_fifo->front;
    if (old_front->modified) {
      kvs_base_set(kvs_fifo->kvs_base, old_front->key, old_front->value);
    }

    kvs_fifo->front = old_front->next;
    if (!kvs_fifo->front) {
      kvs_fifo->rear = NULL;
    }

    free(old_front);
    kvs_fifo->size--;
  }

  cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
  strcpy(new_entry->key, key);
  strcpy(new_entry->value, value);
  new_entry->modified = true;
  new_entry->next = NULL;

  if (kvs_fifo->rear) {
    kvs_fifo->rear->next = new_entry;
  } else {
    kvs_fifo->front = new_entry;
  }
  kvs_fifo->rear = new_entry;
  kvs_fifo->size++;

  return SUCCESS;
}

int kvs_fifo_get(kvs_fifo_t* kvs_fifo, const char* key, char* value) {
  cache_entry_t* existing_entry = find_cache_entry(kvs_fifo, key);

  if (existing_entry) {
    strcpy(value, existing_entry->value);
    return SUCCESS;
  }

  int result = kvs_base_get(kvs_fifo->kvs_base, key, value);
  if (result == SUCCESS) {
    if (kvs_fifo->size == kvs_fifo->capacity) {
      cache_entry_t* old_front = kvs_fifo->front;
      if (old_front->modified) {
        kvs_base_set(kvs_fifo->kvs_base, old_front->key, old_front->value);
      }

      kvs_fifo->front = old_front->next;
      if (!kvs_fifo->front) {
        kvs_fifo->rear = NULL;
      }

      free(old_front);
      kvs_fifo->size--;
    }

    cache_entry_t* new_entry = malloc(sizeof(cache_entry_t));
    strcpy(new_entry->key, key);
    strcpy(new_entry->value, value);
    new_entry->modified = false;
    new_entry->next = NULL;

    if (kvs_fifo->rear) {
      kvs_fifo->rear->next = new_entry;
    } else {
      kvs_fifo->front = new_entry;
    }
    kvs_fifo->rear = new_entry;
    kvs_fifo->size++;
  }

  return result;
}

int kvs_fifo_flush(kvs_fifo_t* kvs_fifo) {
  cache_entry_t* current = kvs_fifo->front;
  while (current) {
    if (current->modified) {
      kvs_base_set(kvs_fifo->kvs_base, current->key, current->value);
      current->modified = false;
    }
    current = current->next;
  }

  while (kvs_fifo->front) {
    cache_entry_t* temp = kvs_fifo->front;
    kvs_fifo->front = kvs_fifo->front->next;
    free(temp);
  }
  kvs_fifo->rear = NULL;
  kvs_fifo->size = 0;

  return SUCCESS;
}
