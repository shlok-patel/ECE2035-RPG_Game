/*
 Student Name: Shlok Patel
 Date:         11/25/2018

=======================
ECE 2035 Project 2-1:
=======================
This file provides definition for the structs and functions declared in the
header file. It also contains helper functions that are not accessible from
outside of the file.

FOR FULL CREDIT, BE SURE TO TRY MULTIPLE TEST CASES and DOCUMENT YOUR CODE.

===================================
Naming conventions in this file:
===================================
1. All struct names use camel case where the first letter is capitalized.
  e.g. "HashTable", or "HashTableEntry"

2. Variable names with a preceding underscore "_" will not be called directly.
  e.g. "_HashTable", "_HashTableEntry"

  Recall that in C, we have to type "struct" together with the name of the struct
  in order to initialize a new variable. To avoid this, in hash_table.h
  we use typedef to provide new "nicknames" for "struct _HashTable" and
  "struct _HashTableEntry". As a result, we can create new struct variables
  by just using:
    - "HashTable myNewTable;"
     or
    - "HashTableEntry myNewHashTableEntry;"

  The preceding underscore "_" simply provides a distinction between the names
  of the actual struct defition and the "nicknames" that we use to initialize
  new structs.
  [See Hidden Definitions section for more information.]

3. Functions, their local variables and arguments are named with camel case, where
  the first letter is lower-case.
  e.g. "createHashTable" is a function. One of its arguments is "numBuckets".
       It also has a local variable called "newTable".

4. The name of a struct member is divided by using underscores "_". This serves
  as a distinction between function local variables and struct members.
  e.g. "num_buckets" is a member of "HashTable".

*/

/****************************************************************************
* Include the Public Interface
*
* By including the public interface at the top of the file, the compiler can
* enforce that the function declarations in the the header are not in
* conflict with the definitions in the file. This is not a guarantee of
* correctness, but it is better than nothing!
***************************************************************************/
#include "hash_table.h"


/****************************************************************************
* Include other private dependencies
*
* These other modules are used in the implementation of the hash table module,
* but are not required by users of the hash table.
***************************************************************************/
#include <stdlib.h>   // For malloc and free
#include <stdio.h>    // For printf


/****************************************************************************
* Hidden Definitions
*
* These definitions are not available outside of this file. However, because
* the are forward declared in hash_table.h, the type names are
* available everywhere and user code can hold pointers to these structs.
***************************************************************************/
/**
 * This structure represents an a hash table.
 * Use "HashTable" instead when you are creating a new variable. [See top comments]
 */
struct _HashTable {
    /** The array of pointers to the head of a singly linked list, whose nodes
        are HashTableEntry objects */
    HashTableEntry** buckets;

    /** The hash function pointer */
    HashFunction hash;

    /** The number of buckets in the hash table */
    unsigned int num_buckets;
};

/**
 * This structure represents a hash table entry.
 * Use "HashTableEntry" instead when you are creating a new variable. [See top comments]
 */
struct _HashTableEntry {
    /** The key for the hash table entry */
    unsigned int key;

    /** The value associated with this hash table entry */
    void* value;

    /**
    * A pointer pointing to the next hash table entry
    * NULL means there is no next entry (i.e. this is the tail)
    */
    HashTableEntry* next;
};


/****************************************************************************
* Private Functions
*
* These functions are not available outside of this file, since they are not
* declared in hash_table.h.
***************************************************************************/
/**
* createHashTableEntry
*
* Helper function that creates a hash table entry by allocating memory for it on
* the heap. It initializes the entry with key and value, initialize pointer to
* the next entry as NULL, and return the pointer to this hash table entry.
*
* @param key The key corresponds to the hash table entry
* @param value The value stored in the hash table entry
* @return The pointer to the hash table entry
*/
static HashTableEntry* createHashTableEntry(unsigned int key, void* value)
{
    HashTableEntry* HTentry = (HashTableEntry*)malloc(sizeof(HashTableEntry));
    HTentry->key = key;                                                   // key
    HTentry->value = value;                                               // value tied to key
    HTentry->next = NULL;                                                 // next entry is defined null
    return HTentry;
}

/**
* findItem
*
* Helper function that checks whether there exists the hash table entry that
* contains a specific key.
*
* @param hashTable The pointer to the hash table.
* @param key The key corresponds to the hash table entry
* @return The pointer to the hash table entry, or NULL if key does not exist
*/
static HashTableEntry* findItem(HashTable* hashTable, unsigned int key)
{
    unsigned int i = hashTable->hash(key);                                // i = bucket index
    HashTableEntry* thisNode = hashTable->buckets[i];                     // thisNode points to item
    while(thisNode) {                                                     // while thisNode is not null
        if(thisNode->key == key) {                                        // if thisNode lookdown key equals key
            return thisNode;                                              // return thisNode
        }
        thisNode = thisNode->next;                                        // else go to next entry
    }
    return thisNode;
}



/****************************************************************************
* Public Interface Functions
*
* These functions implement the public interface as specified in the header
* file, and make use of the private functions and hidden definitions in the
* above sections.
****************************************************************************/
// The createHashTable is provided for you as a starting point.
HashTable* createHashTable(HashFunction hashFunction, unsigned int numBuckets)
{
    // The hash table has to contain at least one bucket. Exit gracefully if
    // this condition is not met.
    if (numBuckets==0) {
        printf("Hash table has to contain at least 1 bucket...\n");
        exit(1);
    }

    // Allocate memory for the new HashTable struct on heap.
    HashTable* newTable = (HashTable*)malloc(sizeof(HashTable));

    // Initialize the components of the new HashTable struct.
    newTable->hash = hashFunction;
    newTable->num_buckets = numBuckets;
    newTable->buckets = (HashTableEntry**)malloc(numBuckets*sizeof(HashTableEntry*));

    // As the new buckets contain indeterminant values, init each bucket as NULL.
    unsigned int i;
    for (i=0; i<numBuckets; ++i) {
        newTable->buckets[i] = NULL;
    }

    // Return the new HashTable struct.
    return newTable;
}

void destroyHashTable(HashTable* hashTable)
{
    HashTableEntry* Temp;
    HashTableEntry* Temp2;
    for(int i = 0; i < (hashTable->num_buckets); i++) {                   // parse through and free all items
        Temp = hashTable->buckets[i];
        Temp2 = Temp;
        while(Temp) {                                                     // free item and its value
            Temp2 = Temp;
            Temp = Temp->next;
            if(Temp2->value) {
                free(Temp2->value);
            }
            free(Temp2);
        }
    }
    free(hashTable->buckets);                                             // free buckets
    free(hashTable);                                                      // free table
}

void* insertItem(HashTable* hashTable, unsigned int key, void* value)
{
    unsigned int i = hashTable->hash(key);
    HashTableEntry* thisItem = findItem(hashTable, key);                  // find item given key
    if(thisItem) {                                                        // case 1 - if item already exists, replace existing value
        void* prevValue;
        prevValue = thisItem->value;
        thisItem->value = value;                                          // overwrite previous value
        return prevValue;                                                 // return previous value
    }
    HashTableEntry* newItem = createHashTableEntry(key, value);
    if(!newItem) return NULL;                                             // case 2 - else if item doesn't exist, create new Hash Table entry
    newItem->next = hashTable->buckets[i];
    hashTable->buckets[i] = newItem;
    return NULL;
}

void* getItem(HashTable* hashTable, unsigned int key)
{
    HashTableEntry* thisItem = findItem(hashTable,key);                   // find item
    if(thisItem) return thisItem->value;                                  // if item exists, return it
    return NULL;                                                          // else return NULL
}

void* removeItem(HashTable* hashTable, unsigned int key)
{
    unsigned int i = hashTable->hash(key);                                // bucket index
    HashTableEntry* thisNode = hashTable->buckets[i];
    if(!thisNode) return NULL;                                            // if item is null return null
    HashTableEntry* nextNode;
    void* itemValue;
    if(thisNode->key == key) {                                            // if current item is item you are looking for
        itemValue = thisNode->value;                                      // store value
        hashTable->buckets[i] =  thisNode->next;                          // stitch next item
        free(thisNode);                                                   // free old item
        return itemValue;                                                 // return stored value
    }
    while(thisNode->next) {                                               // keep going to next item
        if(thisNode->next->key == key) {                                  // look for item whose key matches desired key
            nextNode = thisNode->next;                                    // store item
            itemValue = nextNode->value;                                  // store value
            thisNode->next = thisNode->next->next;                        // stitch next next item as next item
            free(nextNode);                                               // free next item
            return itemValue;                                             // return stored value
        }
        thisNode = thisNode->next;                                        // go to next node and repeat while loop
    }
    return NULL;                                                          // return NULL if key not found
}

void deleteItem(HashTable* hashTable, unsigned int key)
{
    void* item = removeItem(hashTable,key);                               // call removeItem to free item
    free(item);                                                           // also free items value
}