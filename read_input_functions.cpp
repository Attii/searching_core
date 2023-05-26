#include "read_input_functions.h"

#include <iostream>

std::string ReadLine() {
    std::string s;
    std::getline(std::cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    std::cin >> result;
    ReadLine();
    return result;
}

std::vector<int> ReadRatings() {
    std::vector<int> res; 
    int tmp, num_of_ratings;
    std::cin >> num_of_ratings; 
    for (int i = 0; i < num_of_ratings; ++i) {
        std::cin >> tmp; 
        res.push_back(tmp); 
    }
    return res;
}