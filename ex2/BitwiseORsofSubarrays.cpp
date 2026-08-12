class Solution {
public:
    int subarrayBitwiseORs(vector<int>& arr) {
        unordered_set<int> result;
        unordered_set<int> current;
        
        for (int x : arr) {
            unordered_set<int> next;
            next.insert(x);
            for (int y : current) {
                next.insert(y | x);
            }
            current = next;
            for (int y : current) {
                result.insert(y);
            }
        }
        
        return result.size();
    }
};
