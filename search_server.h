#pragma once

#include <string>
#include <stdexcept>
#include <tuple>
#include <set>
#include <map>
#include <math.h>
#include <numeric>
#include <algorithm>
#include <execution>
#include <vector>
#include <deque>

#include "string_processing.h"
#include "document.h"

const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer {
    public:
        explicit SearchServer(const std::string& stop_words_set)
            : SearchServer(std::string_view(stop_words_set)) {
        }

        explicit SearchServer(const std::string_view stop_words_set)
            : SearchServer(SplitIntoWords(stop_words_set)) {
        }

        template <typename T>
        explicit SearchServer(const T& stop_words_set);

        std::set<int>::iterator begin();
        std::set<int>::const_iterator begin() const;
        std::set<int>::iterator end();
        std::set<int>::const_iterator end() const;

        // Defines an invalid document id
        // You can refer to this constant as SearchServer::INVALID_DOCUMENT_ID
        inline static constexpr int INVALID_DOCUMENT_ID = -1;

        void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

        std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

        std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus search_status) const;

        template <typename Function>
        std::vector<Document> FindTopDocuments(std::string_view raw_query, Function FilterDocument) const;

        int GetDocumentCount() const;

        // Full result with matched words from document with status(if query contains minus words, function returns empty vector)
        std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
        std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, 
                                                                            std::string_view raw_query, int document_id) const;
        std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, 
                                                                            std::string_view raw_query, int document_id) const;

        void RemoveDocument(int document_id);
        void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
        void RemoveDocument(std::execution::parallel_policy policy, int document_id);

        const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

    private:
        struct Query {
            std::vector<std::string_view> plus_words; 
            std::vector<std::string_view> minus_words; 
        };

        struct DocumentData {
            int rating; 
            DocumentStatus status; 
        };

        std::deque<std::string> storage_;

        std::map<int,DocumentData> documents_;

        std::set<int> documents_id_;

        std::map<std::string_view, std::map<int, double>> word_to_document_index_; // word : (document index : word term frequency in document)
        std::map<int, std::map<std::string_view, double>> document_to_word_index_; // document index : (word : word term frequency in document)

        std::set<std::string_view, std::less<>> stop_words_;

        static bool ContainsSpecialSymbols(std::string_view text);

        std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

        Query ParseQuery(std::string_view text, bool do_sort = false) const;

        static int ComputeAverageRating(const std::vector<int>& ratings);

        double ComputeWordIDF(std::string_view word) const;

        template <typename Function>
        std::vector<Document> FindAllDocuments(const Query& query_words, Function CheckFilter) const;
};

template <typename T>
SearchServer::SearchServer(const T& stop_words_set) {
    for (std::string_view word : stop_words_set) {
        if (ContainsSpecialSymbols(word)) {
            throw std::invalid_argument("Stop words can't contain special symbols");
        }
        if (word.empty()) {
            continue;
        }

        storage_.emplace_back(word);
        stop_words_.insert(word);
    }
}

template <typename Function>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, Function FilterDocument) const {
    Query parsed_query = ParseQuery(raw_query, true);

    std::vector<Document> top_documents = FindAllDocuments(parsed_query, FilterDocument);
    
    const double EPSILON = 1e-6;

    sort(top_documents.begin(), top_documents.end(), 
        [EPSILON](const Document& el1, const Document& el2){
            return el1.relevance > el2.relevance || 
            (std::abs(el1.relevance - el2.relevance) < EPSILON && el1.rating > el2.rating) ;
        });             

    if ( top_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
        top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
    }

    return top_documents;
}

template <typename Function>
std::vector<Document> SearchServer::FindAllDocuments(const Query& query_words, Function CheckFilter) const { 
    std::map<int,double> matched_documents; // [id, relevance]

    for (std::string_view query_word : query_words.plus_words) {
        if (word_to_document_index_.count(query_word)) {
            double word_IDF = ComputeWordIDF(query_word);
            for (const auto& [id, word_TF] : word_to_document_index_.at(query_word)) {  
                DocumentData document_info = documents_.at(id); 
                if (CheckFilter(id, document_info.status, document_info.rating)) {
                    matched_documents[id] += word_TF * word_IDF;
                }
            }
        }
    }

    for (std::string_view minus_word : query_words.minus_words) {
        if (word_to_document_index_.count(minus_word)) {
            for (const auto& [id, word_TF]: word_to_document_index_.at(minus_word)) {
                matched_documents.erase(id);
            }
        }
    }

    std::vector<Document> result;
    for (const auto& [id, relevance] : matched_documents) {
        result.push_back({id, relevance, documents_.at(id).rating});
    }

    return result;
}