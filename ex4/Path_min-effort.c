#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <limits.h>

// Structure to hold grid cell data inside the Min-Heap
typedef struct {
    int r;
    int c;
    int effort;
} HeapNode;

// Structure for the Min-Heap
typedef struct {
    HeapNode* data;
    int size;
    int capacity;
} MinHeap;

// Min-Heap utility functions
MinHeap* createMinHeap(int capacity) {
    MinHeap* heap = (MinHeap*)malloc(sizeof(MinHeap));
    heap->data = (HeapNode*)malloc(capacity * sizeof(HeapNode));
    heap->size = 0;
    heap->capacity = capacity;
    return heap;
}

void swap(HeapNode* a, HeapNode* b) {
    HeapNode temp = *a;
    *a = *b;
    *b = temp;
}

void push(MinHeap* heap, HeapNode node) {
    heap->data[heap->size] = node;
    int i = heap->size;
    heap->size++;
    
    // Bubble up
    while (i > 0 && heap->data[i].effort < heap->data[(i - 1) / 2].effort) {
        swap(&heap->data[i], &heap->data[(i - 1) / 2]);
        i = (i - 1) / 2;
    }
}

HeapNode pop(MinHeap* heap) {
    HeapNode root = heap->data[0];
    heap->size--;
    heap->data[0] = heap->data[heap->size];
    
    int i = 0;
    // Bubble down
    while (2 * i + 1 < heap->size) {
        int left = 2 * i + 1;
        int right = 2 * i + 2;
        int smallest = left;
        
        if (right < heap->size && heap->data[right].effort < heap->data[left].effort) {
            smallest = right;
        }
        if (heap->data[i].effort <= heap->data[smallest].effort) {
            break;
        }
        swap(&heap->data[i], &heap->data[smallest]);
        i = smallest;
    }
    return root;
}

int minimumEffortPath(int** heights, int heightsSize, int* heightsColSize) {
    int rows = heightsSize;
    int cols = heightsColSize[0];
    
    // Matrix to store the minimum effort to reach each cell
    int** efforts = (int**)malloc(rows * sizeof(int*));
    for (int i = 0; i < rows; i++) {
        efforts[i] = (int*)malloc(cols * sizeof(int));
        for (int j = 0; j < cols; j++) {
            efforts[i][j] = INT_MAX;
        }
    }
    
    // Priority queue initialization
    MinHeap* heap = createMinHeap(rows * cols);
    
    // Starting at (0, 0) requires 0 effort
    efforts[0][0] = 0;
    push(heap, (HeapNode){0, 0, 0});
    
    // Direction vectors for moving: Up, Down, Left, Right
    int dr[] = {-1, 1, 0, 0};
    int dc[] = {0, 0, -1, 1};
    
    while (heap->size > 0) {
        HeapNode curr = pop(heap);
        int r = curr.r;
        int c = curr.c;
        int currEffort = curr.effort;
        
        // If we reached the target bottom-right cell, return the effort
        if (r == rows - 1 && c == cols - 1) {
            // Cleanup memory before returning
            int ans = currEffort;
            free(heap->data);
            free(heap);
            for (int i = 0; i < rows; i++) free(efforts[i]);
            free(efforts);
            return ans;
        }
        
        // If we found a path with worse effort than already recorded, skip it
        if (currEffort > efforts[r][c]) {
            continue;
        }
        
        // Explore all 4 adjacent directions
        for (int i = 0; i < 4; i++) {
            int nr = r + dr[i];
            int nc = c + dc[i];
            
            // Check boundaries
            if (nr >= 0 && nr < rows && nc >= 0 && nc < cols) {
                // The effort for this specific step is the absolute difference in height
                int stepEffort = abs(heights[r][c] - heights[nr][nc]);
                // The total route effort to the neighbor is the maximum edge seen so far
                int newEffort = currEffort > stepEffort ? currEffort : stepEffort;
                
                // Relax edge if a path with lesser effort is discovered
                if (newEffort < efforts[nr][nc]) {
                    efforts[nr][nc] = newEffort;
                    push(heap, (HeapNode){nr, nc, newEffort});
                }
            }
        }
    }
    
    return 0; // Fallback
}
