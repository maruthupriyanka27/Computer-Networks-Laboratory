class Solution {
public:
    int findMaximumXOR(vector<int>& nums) {
        int maxXOR = 0;
        int mask = 0;
        
        for (int i = 30; i >= 0; i--) {
            mask = mask | (1 << i);
            unordered_set<int> prefixes;
            
            for (int num : nums) {
                prefixes.insert(num & mask);
            }
            
            int candidate = maxXOR | (1 << i);
            
            for (int prefix : prefixes) {
                if (prefixes.count(prefix ^ candidate)) {
                    maxXOR = candidate;
                    break;
                }
            }
        }
        
        return maxXOR;
    }
};
