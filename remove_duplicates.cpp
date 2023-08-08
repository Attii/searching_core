#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicates;
    std::set<std::set<std::string>> words_in_documents;

    for (const int document_id : search_server) {
        std::set<std::string>  words_in_doc;

        for (const auto& [word, freq] : search_server.GetWordFrequencies(document_id)) {
            words_in_doc.insert(word);
        }

        if (words_in_documents.count(words_in_doc)) {
            duplicates.insert(document_id);
        } else {
            words_in_documents.insert(words_in_doc);
        }
    }

    for (int duplicate : duplicates) {
        std::cout << "Found duplicate document id " << duplicate << std::endl;
        search_server.RemoveDocument(duplicate);
    }
} 