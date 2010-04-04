#include <cstdio>
#include <vector>
#include <string>

using std::string;
using std::vector;

enum { MAX_INPUT_SIZE = 1 << 20 };

vector<size_t> compute_prefix_function(const string &str) {
    vector<size_t> prefix_function(str.size(), 0);
    for (size_t i = 1; i < str.size(); i++) {
        for (prefix_function[i] = prefix_function[i - 1]; prefix_function[i] > 0 && str[prefix_function[i]] != str[i];)
            prefix_function[i] = prefix_function[prefix_function[i] - 1];
        prefix_function[i] = str[prefix_function[i]] == str[i] ? prefix_function[i] + 1 : 0; 
    }
    return prefix_function;
}

int find_period(const string &str) {
    vector<size_t> prefix_function = compute_prefix_function(str + str);
    for (size_t begin = 1; ; begin++) {
        size_t end = begin + str.size() - 1;
        if (prefix_function[end] == str.size())  // if str appears in position begin of str+str
            return str.size() / begin;
    }
    return 0;  // not going to happen
}

int main()
{
    static char input[MAX_INPUT_SIZE];
    scanf("%s", input);
    printf("%d\n", find_period(input));
}
