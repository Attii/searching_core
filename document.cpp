#include "document.h"

Document::Document()
    : id(0)
    , relevance(0.0)
    , rating(0)  {
    } 

Document::Document(int id_p, double rel_p, int rating_p)
    : id(id_p)
    , relevance(rel_p)
    , rating(rating_p)  {
    } 

void PrintMatchDocumentResult(int document_id, const std::vector<std::string>& words, DocumentStatus status) {
    std::cout << "{ "
         << "document_id = " << document_id << ", "
         << "status = " << static_cast<int>(status) << ", "
         << "words =";
    for (const std::string& word : words) {
        std::cout << ' ' << word;
    }
    std::cout << "}" << std::endl;
}

std::ostream& operator<<(std::ostream& output, Document  doc) {
    output << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
    return output;
}