#include "remove_duplicates.h"

void RemoveDuplicates(SearchServer& search_server) {
    std::set<int> duplicates;
    
    for (const int document_id : search_server) {
        if (duplicates.count(document_id)) continue; 
        const std::map<std::string, double> doc_to_comp = search_server.GetWordFrequencies(document_id);

        for (const int next_document_id : search_server) {
            if(!duplicates.count(next_document_id) && next_document_id != document_id) {
                const auto doc_to_comp2 = search_server.GetWordFrequencies(next_document_id);
                if (doc_to_comp.size() == doc_to_comp2.size() && 
                    std::equal(doc_to_comp.begin(), doc_to_comp.end(), doc_to_comp2.begin(), 
                    [](auto a, auto b) { return a.first == b.first; })) 
                {
                    duplicates.insert(next_document_id);
                }
            }
        }
    }

    for (int duplicate : duplicates) {
        std::cout << "Found duplicate document id " << duplicate << std::endl;
        search_server.RemoveDocument(duplicate);
    }
} 