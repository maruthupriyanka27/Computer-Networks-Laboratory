#include <stdlib.h>
#include <string.h>

// A basic hash map implementation tracking delayed deletions
typedef struct {
    int key;
    int count;
} HashEntry;

typedef struct {
    HashEntry* buckets;
    int size;
} HashMap;

HashMap* createMap(int capacity) {
    HashMap* map = (HashMap*)malloc(sizeof(HashMap));
    map->size = capacity * 2; // Simple allocation
    map->buckets = (HashEntry*)calloc(map->size, sizeof(HashEntry));
    return map;
}

void mapInsert(HashMap* map, int key) {
    unsigned int h = ((unsigned int)key) % map->size;
    while (map->buckets[h].count > 0 && map->buckets[h].key != key) {
        h = (h + 1) % map->size;
    }
    map->buckets[h].key = key;
    map->buckets[h].count++;
}

int mapGetAndDecrement(HashMap* map, int key) {
    unsigned int h = ((unsigned int)key) % map->size;
    while (map->buckets[h].count > 0 || map->buckets[h].key == key) {
        if (map->buckets[h].key == key && map->buckets[h].count > 0) {
            map->buckets[h].count--;
            return 1;
        }
        h = (h + 1) % map->size;
    }
    return 0;
}

void freeMap(HashMap* map) {
    free(map->buckets);
    free(map);
}

// Max-Heap (Left) and Min-Heap (Right) Implementation
typedef struct {
    int* data;
    int size;
    int isMinHeap;
} Heap;

Heap* createHeap(int capacity, int isMinHeap) {
    Heap* heap = (Heap*)malloc(sizeof(Heap));
    heap->data = (int*)malloc((capacity + 1) * sizeof(int));
    heap->size = 0;
    heap->isMinHeap = isMinHeap;
    return heap;
}

int compare(int a, int b, int isMinHeap) {
    if (isMinHeap) return a < b;
    return a > b; // Max heap
}

void push(Heap* heap, int val) {
    heap->size++;
    int i = heap->size;
    while (i > 1 && compare(val, heap->data[i / 2], heap->isMinHeap)) {
        heap->data[i] = heap->data[i / 2];
        i /= 2;
    }
    heap->data[i] = val;
}

void pop(Heap* heap) {
    if (heap->size == 0) return;
    int last = heap->data[heap->size--];
    int i = 1, child;
    while (i * 2 <= heap->size) {
        child = i * 2;
        if (child + 1 <= heap->size && compare(heap->data[child + 1], heap->data[child], heap->isMinHeap)) {
            child++;
        }
        if (compare(last, heap->data[child], heap->isMinHeap)) break;
        heap->data[i] = heap->data[child];
        i = child;
    }
    heap->data[i] = last;
}

int top(Heap* heap) {
    return heap->data[1];
}

void prune(Heap* heap, HashMap* map) {
    while (heap->size > 0) {
        int t = top(heap);
        unsigned int h = ((unsigned int)t) % map->size;
        int found = 0;
        while (map->buckets[h].count > 0 || map->buckets[h].key == t) {
            if (map->buckets[h].key == t && map->buckets[h].count > 0) {
                found = 1;
                break;
            }
            h = (h + 1) % map->size;
        }
        if (found) {
            mapGetAndDecrement(map, t);
            pop(heap);
        } else {
            break;
        }
    }
}

double* medianSlidingWindow(int* nums, int numsSize, int k, int* returnSize) {
    int numResults = numsSize - k + 1;
    *returnSize = numResults;
    double* result = (double*)malloc(numResults * sizeof(double));
    
    Heap* maxHeap = createHeap(numsSize, 0); // Stores smaller half
    Heap* minHeap = createHeap(numsSize, 1); // Stores larger half
    HashMap* delayedDeletes = createMap(numsSize);

    int maxHeapSize = 0, minHeapSize = 0;

    // Initialize with first k elements
    for (int i = 0; i < k; i++) {
        push(maxHeap, nums[i]);
        maxHeapSize++;
    }
    for (int i = 0; i < k / 2; i++) {
        push(minHeap, top(maxHeap));
        pop(maxHeap);
        maxHeapSize--;
        minHeapSize++;
    }

    // Process all windows
    for (int i = 0; i < numResults; i++) {
        if (k % 2 == 1) {
            result[i] = (double)top(maxHeap);
        } else {
            result[i] = ((double)top(maxHeap) + (double)top(minHeap)) / 2.0;
        }

        if (i + k < numsSize) {
            int toRemove = nums[i];
            int toAdd = nums[i + k];
            int balance = 0;

            // Determine balance adjustments based on which heap target belongs to
            balance += (toRemove <= top(maxHeap)) ? -1 : 1;
            mapInsert(delayedDeletes, toRemove);

            if (maxHeap->size > 0 && toAdd <= top(maxHeap)) {
                push(maxHeap, toAdd);
                balance++;
            } else {
                push(minHeap, toAdd);
                balance--;
            }

            // Rebalance the halves
            if (balance > 0) {
                push(minHeap, top(maxHeap));
                pop(maxHeap);
                prune(maxHeap, delayedDeletes);
            } else if (balance < 0) {
                push(maxHeap, top(minHeap));
                pop(minHeap);
                prune(minHeap, delayedDeletes);
            }

            prune(maxHeap, delayedDeletes);
            prune(minHeap, delayedDeletes);
        }
    }

    // Cleanup memory
    free(maxHeap->data); free(maxHeap);
    free(minHeap->data); free(minHeap);
    freeMap(delayedDeletes);

    return result;
}
