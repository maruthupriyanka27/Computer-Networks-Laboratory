#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <limits.h>

// Node structure for our adjacency list
typedef struct Node {
    int to;
    int weight;
    struct Node* next;
} Node;

int networkDelayTime(int** times, int timesSize, int* timesColSize, int n, int k) {
    // Step 1: Build the adjacency list (1-indexed nodes)
    Node** adj = (Node**)calloc(n + 1, sizeof(Node*));
    for (int i = 0; i < timesSize; i++) {
        int u = times[i][0];
        int v = times[i][1];
        int w = times[i][2];
        
        Node* newNode = (Node*)malloc(sizeof(Node));
        newNode->to = v;
        newNode->weight = w;
        newNode->next = adj[u];
        adj[u] = newNode;
    }
    
    // Step 2: Initialize Dijkstra distances and visited tracking array
    int* dist = (int*)malloc((n + 1) * sizeof(int));
    bool* visited = (bool*)calloc(n + 1, sizeof(bool));
    
    for (int i = 1; i <= n; i++) {
        dist[i] = INT_MAX;
    }
    dist[k] = 0; // Distance to the source node itself is 0
    
    // Step 3: Run Dijkstra's Algorithm
    for (int count = 0; count < n; count++) {
        // Find the unvisited node with the minimum distance
        int u = -1;
        int minDist = INT_MAX;
        for (int i = 1; i <= n; i++) {
            if (!visited[i] && dist[i] < minDist) {
                minDist = dist[i];
                u = i;
            }
        }
        
        // If we can't find an accessible node, break early
        if (u == -1) {
            break;
        }
        
        visited[u] = true;
        
        // Relax the edges of the selected node
        Node* curr = adj[u];
        while (curr != NULL) {
            int v = curr->to;
            int weight = curr->weight;
            
            if (!visited[v] && dist[u] + weight < dist[v]) {
                dist[v] = dist[u] + weight;
            }
            curr = curr->next;
        }
    }
    
    // Step 4: Find the maximum time taken among all reachable nodes
    int maxTime = 0;
    for (int i = 1; i <= n; i++) {
        if (dist[i] == INT_MAX) {
            maxTime = -1; // Node i is completely unreachable from source k
            break;
        }
        if (dist[i] > maxTime) {
            maxTime = dist[i];
        }
    }
    
    // Step 5: Free all dynamically allocated memory
    for (int i = 1; i <= n; i++) {
        Node* currNode = adj[i];
        while (currNode != NULL) {
            Node* toFree = currNode;
            currNode = currNode->next;
            free(toFree);
        }
    }
    free(adj);
    free(dist);
    free(visited);
    
    return maxTime;
}
