#include <cstdint>
#include <cstdio>
#include <stdexcept>

#ifndef FIXED_HASHMAP_HPP
#define FIXED_HASHMAP_HPP

#include <cstdint>
#include <cstdio>

#define TABLE_SIZE 1024  // Fixed size of the hash table

// Structure for a hash table entry
struct HashEntry {
    intptr_t key;
    int64_t value;
    bool occupied;  // Indicates whether the slot is occupied
};

// Fixed-size hash table
class FixedHashTable {
private:
    HashEntry entries[TABLE_SIZE];  // Array of entries

    // Hash function to map a key to an index
    size_t hash(intptr_t key) const {
        return static_cast<size_t>(key) % TABLE_SIZE;
    }

public:
    // Constructor to initialize the hash table
    FixedHashTable() {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            entries[i].occupied = false;
            entries[i].key = 0;
            entries[i].value = 0;
        }
    }

    // Insert a key-value pair into the hash table
    bool insert(intptr_t key, int64_t value) {
        size_t index = hash(key);
        size_t original_index = index;

        do {
            if (!entries[index].occupied) {
                entries[index].key = key;
                entries[index].value = value;
                entries[index].occupied = true;
                return true;  // Successfully inserted
            }
            index = (index + 1) % TABLE_SIZE;  // Linear probing
        } while (index != original_index);     // Stop if we loop back to the start

        return false;  // Hash table is full
    }

    // Look up a value by key
    bool get(intptr_t key, int64_t* value) const {
        size_t index = hash(key);
        size_t original_index = index;

        do {
            if (entries[index].occupied && entries[index].key == key) {
                *value = entries[index].value;
                return true;  // Key found
            }
            index = (index + 1) % TABLE_SIZE;  // Linear probing
        } while (index != original_index);     // Stop if we loop back to the start

        return false;  // Key not found
    }

    // Remove a key-value pair from the hash table
    bool remove(intptr_t key) {
        size_t index = hash(key);
        size_t original_index = index;

        do {
            if (entries[index].occupied && entries[index].key == key) {
                entries[index].occupied = false;
                entries[index].key = 0;
                entries[index].value = 0;
                return true;  // Successfully removed
            }
            index = (index + 1) % TABLE_SIZE;  // Linear probing
        } while (index != original_index);     // Stop if we loop back to the start

        return false;  // Key not found
    }

    // Overload the [] operator
    int64_t& operator[](intptr_t key) {
        size_t index = hash(key);
        size_t original_index = index;

        do {
            if (entries[index].occupied && entries[index].key == key) {
                return entries[index].value;  // Key found, return reference to value
            }
            if (!entries[index].occupied) {
                // Key not found, insert it with a default value (0)
                entries[index].key = key;
                entries[index].value = 0;
                entries[index].occupied = true;
                return entries[index].value;  // Return reference to newly inserted value
            }
            index = (index + 1) % TABLE_SIZE;  // Linear probing
        } while (index != original_index);     // Stop if we loop back to the start

        // If the table is full, throw an error (or handle it as needed)
        throw std::runtime_error("Hash table is full");
    }

    // Print the contents of the hash table
    void print() const {
        for (size_t i = 0; i < TABLE_SIZE; ++i) {
            if (entries[i].occupied) {
                std::printf("Index %zu: Key = %ld, Value = %ld\n", i, entries[i].key, entries[i].value);
            } else {
                std::printf("Index %zu: Empty\n", i);
            }
        }
    }
};

#endif
