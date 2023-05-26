#pragma once

#include <string>
#include <vector>
#include <deque>

#include "search_server.h"

class RequestQueue {
public:
    explicit RequestQueue(const SearchServer& search_server)
        : search_server_(search_server)
    {
    }
    
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);

    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);

    std::vector<Document> AddFindRequest(const std::string& raw_query);

    int GetNoResultRequests() const;

private:
    const SearchServer& search_server_;
    struct QueryResult {
        bool isEmpty;
    };
    std::deque<QueryResult> requests_;
    const static int min_in_day_ = 1440;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
    if (requests_.size() >= min_in_day_) {
        requests_.pop_front();
    }

    std::vector<Document> found_documents = search_server_.FindTopDocuments(raw_query, document_predicate);

    if (found_documents.size() == 0) {
        requests_.push_back({true});
    }
    else {
        requests_.push_back({false});
    }

    return found_documents;
}