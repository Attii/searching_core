#pragma once

#include <vector>
#include <iostream>

enum class DocumentStatus {
    ACTUAL, 
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id; 
    double relevance;
    int rating;

    Document(); 

    Document(int id_p, double rel_p, int rating_p);
}; 

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status);

std::ostream& operator<<(std::ostream& output, Document  doc); // except PrintDocument