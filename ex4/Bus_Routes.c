#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Struct for adjacency list mapping a stop to the buses that visit it
typedef struct Node {
    int bus;
    struct Node* next;
} Node;

// Struct for the BFS queue entries
typedef struct {
    int bus;
    int depth;
} QueueItem;

int numBusesToDestination(int** routes, int routesSize, int* routesColSize, int source, int target) {
    // Edge case: If you are already at the destination, 0 buses are needed
    if (source == target) {
        return 0;
    }

    // Step 1: Find the maximum stop ID to properly size our array maps
    int maxStop = 0;
    for (int i = 0; i < routesSize; i++) {
        for (int j = 0; j < routesColSize[i]; j++) {
            if (routes[i][j] > maxStop) {
                maxStop = routes[i][j];
            }
        }
    }

    // If source or target is outside the maximum stop index seen, it's unreachable
    if (source > maxStop || target > maxStop) {
        return -1;
    }

    // Step 2: Build the stop-to-bus adjacency list
    Node** stopToBuses = (Node**)calloc(maxStop + 1, sizeof(Node*));
    for (int i = 0; i < routesSize; i++) {
        for (int j = 0; j < routesColSize[i]; j++) {
            int stop = routes[i][j];
            Node* newNode = (Node*)malloc(sizeof(Node));
            newNode->bus = i;
            newNode->next = stopToBuses[stop];
            stopToBuses[stop] = newNode;
        }
    }

    // Step 3: Initialize BFS structures
    QueueItem* queue = (QueueItem*)malloc(routesSize * sizeof(QueueItem));
    int head = 0, tail = 0;
    
    bool* visitedBuses = (bool*)calloc(routesSize, sizeof(bool));
    bool* visitedStops = (bool*)calloc(maxStop + 1, sizeof(bool));

    // Seed the queue with all buses that pass through the source stop
    Node* curr = stopToBuses[source];
    while (curr != NULL) {
        int bus = curr->bus;
        if (!visitedBuses[bus]) {
            visitedBuses[bus] = true;
            queue[tail++] = (QueueItem){bus, 1};
        }
        curr = curr->next;
    }
    visitedStops[source] = true;

    // Step 4: Execute BFS
    int result = -1;
    while (head < tail) {
        QueueItem current = queue[head++];
        int currBus = current.bus;
        int depth = current.depth;

        // Check all stops reachable by this specific bus
        for (int i = 0; i < routesColSize[currBus]; i++) {
            int stop = routes[currBus][i];

            // If we found the destination stop, return the total bus count
            if (stop == target) {
                result = depth;
                goto cleanup;
            }

            // If the stop hasn't been fully explored yet, check its transferring buses
            if (!visitedStops[stop]) {
                visitedStops[stop] = true;
                
                Node* nextBusNode = stopToBuses[stop];
                while (nextBusNode != NULL) {
                    int nextBus = nextBusNode->bus;
                    if (!visitedBuses[nextBus]) {
                        visitedBuses[nextBus] = true;
                        queue[tail++] = (QueueItem){nextBus, depth + 1};
                    }
                    nextBusNode = nextBusNode->next;
                }
            }
        }
    }

cleanup:
    // Step 5: Free dynamically allocated memory to avoid memory leaks
    for (int i = 0; i <= maxStop; i++) {
        Node* node = stopToBuses[i];
        while (node != NULL) {
            Node* temp = node;
            node = node->next;
            free(temp);
        }
    }
    free(stopToBuses);
    free(queue);
    free(visitedBuses);
    free(visitedStops);

    return result;
}
