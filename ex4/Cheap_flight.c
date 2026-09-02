#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>

int findCheapestPrice(int n, int** flights, int flightsSize, int* flightsColSize, int src, int dst, int k) {
    // Arrays to hold minimum costs. Using a value short of INT_MAX to prevent overflow additions.
    int const INF = 1e9;
    int* dist = (int*)malloc(n * sizeof(int));
    int* prevDist = (int*)malloc(n * sizeof(int));
    
    // Initialize distances from source
    for (int i = 0; i < n; i++) {
        dist[i] = INF;
    }
    dist[src] = 0;

    // We can take at most k stops, which translates to k + 1 flights (edges)
    for (int i = 0; i <= k; i++) {
        // Copy current distances to prevDist to prevent multi-hop updates within the same iteration
        memcpy(prevDist, dist, n * sizeof(int));
        
        for (int j = 0; j < flightsSize; j++) {
            int u = flights[j][0];
            int v = flights[j][1];
            int price = flights[j][2];
            
            // If the starting city of this flight was reachable in the previous level
            if (prevDist[u] != INF) {
                if (prevDist[u] + price < dist[v]) {
                    dist[v] = prevDist[u] + price;
                }
            }
        }
    }

    int result = dist[dst] == INF ? -1 : dist[dst];
    
    // Clean up allocated memory
    free(dist);
    free(prevDist);
    
    return result;
}
