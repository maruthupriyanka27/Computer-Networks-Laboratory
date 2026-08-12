class Solution {
public:
    int totalHammingDistance(vector<int>& nums) {
        int total = 0;
        int n = nums.size();
        
        for (int i = 0; i < 30; i++) {
            int count_ones = 0;
            for (int num : nums) {
                count_ones += (num >> i) & 1;
            }
            total += count_ones * (n - count_ones);
        }
        
        return total;
    }
};
