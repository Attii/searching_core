#include "string_processing.h"

std::vector<std::string> SplitIntoWords(const std::string& text) {
    std::vector<std::string> words;
    std::string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        } else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }

    return words;
}

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
    std::vector<std::string_view> result;
    str.remove_prefix(std::min(str.find_first_not_of(" "), str.size()));  

    while (!str.empty()) {
        int64_t space = str.find(' ');
        result.push_back(str.substr(str.find_first_not_of(" "), space - str.find_first_not_of(" ")));
        str.remove_prefix(std::min(str.size(), str.find_first_not_of(" ", space)));
    }

    return result;
}