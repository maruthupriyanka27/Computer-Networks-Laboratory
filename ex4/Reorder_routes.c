#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>

// Node structure for our adjacency list
typedef struct Node {
    int to;
    int isOriginal; // 1 if original direction (a -> b), 0 if artificial reverse direction (b -> a)
    struct Node* next;
} Node;

int minReorder(int n, int** connections, int connectionsSize, int* connectionsColSize) {
    // Step 1: Initialize adjacency list
    Node** adj = (Node**)calloc(n, sizeof(Node*));
    
    for (int i = 0; i < connectionsSize; i++) {
        int u = connections[i][0];
        int v = connections[i][1];
        
        // Add original directed edge u -> v (isOriginal = 1)
        Node* edge1 = (Node*)malloc(sizeof(Node));
        edge1->to = v;
        edge1->isOriginal = 1;
        edge1->next = adj[u];
        adj[u] = edge1;
        
        // Add artificial reverse edge v -> u (isOriginal = 0)
        Node* edge2 = (Node*)malloc(sizeof(Node));
        edge2->to = u;
        edge2->isOriginal = 0;
        edge2->next = adj[v];
        adj[v] = edge2;
    }
    
    // Step 2: Initialize BFS Queue and Visited array
    int* queue = (int*)malloc(n * sizeof(int));
    bool* visited = (bool*)calloc(n, sizeof(bool));
    
    int head = 0, tail = 0;
    int reorderCount = 0;
    
    // Start BFS from the capital (city 0)
    queue[tail++] = 0;
    visited[0] = true;
    
    // Step 3: Execute BFS
    while (head < tail) {
        int curr = queue[head++];
        
        Node* temp = adj[curr];
        while (temp != NULL) {
            int neighbor = temp->to;
            
            if (!visited[neighbor]) {
                visited[neighbor] = true;
                
                // If it's an original edge pointing AWAY from the capital hierarchy, 
                // we must reverse it.
                if (temp->isOriginal == 1) {
                    reorderCount++;
                }
                
                queue[tail++] = neighbor;
            }
            temp = temp->next;
        }
    }
    
    // Step 4: Clean up dynamically allocated memory
    for (int i = 0; i < n; i++) {
        Node* currNode = adj[i];
        while (currNode != NULL) {
            Node* toFree = currNode;
            currNode = currNode->next;
            free(toFree);
        }
    }
    free(adj);
    free(queue);
    free(visited);
    
    return reorderCount;
}
