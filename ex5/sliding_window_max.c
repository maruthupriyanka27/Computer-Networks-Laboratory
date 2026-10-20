#include <stdio.h>
#include <stdlib.h>

/**
 * Note: The returned array must be malloced, assume caller calls free().
 */
int* maxSlidingWindow(int* nums, int numsSize, int k, int* returnSize) {
    if (numsSize == 0 || k == 0) {
        *returnSize = 0;
        return NULL;
    }

    // Allocate memory for the result array
    *returnSize = numsSize - k + 1;
    int* result = (int*)malloc((*returnSize) * sizeof(int));

    // Allocate memory for the deque to store indices
    int* deque = (int*)malloc(numsSize * sizeof(int));
    int head = 0; // Front of the deque
    int tail = 0; // Back of the deque

    for (int i = 0; i < numsSize; i++) {
        // 1. Remove indices that are out of the current sliding window bounds
        if (head < tail && deque[head] <= i - k) {
            head++;
        }

        // 2. Remove indices of elements smaller than nums[i] from the back of deque
        //    (they will never be the maximum for the current or future windows)
        while (head < tail && nums[deque[tail - 1]] <= nums[i]) {
            tail--;
        }

        // 3. Push current element's index to the back of the deque
        deque[tail++] = i;

        // 4. The front of the deque is the index of the maximum element for window ending at `i`
        if (i >= k - 1) {
            result[i - k + 1] = nums[deque[head]];
        }
    }

    // Free temporary deque buffer
    free(deque);

    return result;
}
