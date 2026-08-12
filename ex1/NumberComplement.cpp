class Solution {
public:
    int findComplement(int num) {
        long long mask = 1;
        
        while (mask <= num) {
            mask = mask * 2;
        }
        
        mask = mask - 1;
        
        return num ^ mask;
    }
};
