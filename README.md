
#  **FileCache-KVS**

This project is a **file-based Key-Value Store (KVS)** implemented in C, with an **in-memory caching layer** to improve performance by reducing direct disk I/O operations. The cache layer uses **three different replacement algorithms** — FIFO, Clock, and LRU — to handle scenarios where the cache becomes full, ensuring efficient data access and memory management.

## Key Features
- **File-Based KVS**: Supports basic `SET` and `GET` operations, where keys are stored as filenames, and values are the file contents.
- **In-Memory Caching**: Caches key-value pairs in memory to reduce disk access and improve the overall performance.
- **Page Replacement Algorithms**:
  - **FIFO (First-In, First-Out)**: Evicts the earliest cached entry when the cache reaches its capacity.
  - **Clock**: Uses a circular buffer and reference bits to evict the least recently used entry.
  - **LRU (Least Recently Used)**: Evicts the least recently accessed entry when the cache is full.
- **Command-Line Interface**: Interact with the KVS through a simple CLI for performing operations like `SET`, `GET`, and `FLUSH`.

## Project Structure

- **`kvs_base.c`**: Implements the base key-value store where operations interact directly with the file system.
- **`kvs_fifo.c`**: Implements the FIFO-based cached key-value store.
- **`kvs_clock.c`**: Implements the Clock-based cached key-value store.
- **`kvs_lru.c`**: Implements the LRU-based cached key-value store.
- **`client.c`**: Provides a command-line interface to interact with the key-value store.

## How it Works

### File-Based KVS

- The **SET** operation stores a key-value pair by creating a file with the key name and writing the value into it.
- The **GET** operation retrieves the value associated with a key by reading the contents of the corresponding file.

### In-Memory Caching
The in-memory cache stores key-value pairs for quick access. However, since memory is limited, we use cache replacement strategies to decide which entries to evict when the cache is full.

- **FIFO**: The first entry that was added is the first to be removed when the cache reaches its limit.
- **Clock**: Uses reference bits to determine whether an entry has been recently used. If all entries are marked as recently used, the one at the current position of the clock is replaced.
- **LRU**: The entry that has not been accessed for the longest time is evicted when the cache is full.

### Command-Line Interface

You can interact with the KVS using the `client` executable:

```bash
make
./client DIRECTORY POLICY CAPACITY
```

- **DIRECTORY**: Directory where the key-value store files are saved.
- **POLICY**: Caching policy (`NONE`, `FIFO`, `CLOCK`, `LRU`).
- **CAPACITY**: Size of the cache (number of key-value pairs stored in memory).

Supported commands:
```bash
SET {KEY} {VALUE}    # Stores the key-value pair
GET {KEY}            # Retrieves the value associated with the key
FLUSH                # Persists in-memory changes to the disk
```

## Testing
A set of tests is included to validate the functionality of the KVS with each replacement strategy. The tests can be run using the `client.c` file and various combinations of caching policies and capacities.

### Example Test
```bash
./client test_data LRU 2
SET file1.txt data1
SET file2.txt data2
GET file1.txt  # Load file1 into cache
SET file3.txt data3  # Evict file1 using LRU, add file3
```

## Memory Management
The project is designed to prevent memory leaks. Use **Valgrind** to check for memory errors:

```bash
valgrind ./client data LRU 3
```

## License
This project is licensed under the MIT License.

