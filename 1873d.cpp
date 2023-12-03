#include <iostream>

int main() {
    int t, n, k, count;
    std::string str;
    std::cin >> t;
    for (int i = 0; i < t; ++i) {
        count = 0;
        std::cin >> n >> k;
        std::cin >> str;
        for (int j = 0; j < n; ++j) {
            if (str[j] == 'B') {
                count++;
                j += k - 1;
            }
        }
        std::cout << count << std::endl;
    }
}
