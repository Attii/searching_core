#include "search_server.h"

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document ID can't be negative.");
    }

    if (documents_.count(document_id)) {
        throw std::invalid_argument("You cannot add the same document ID");
    }

    const std::vector<std::string>& document_words = SplitIntoWordsNoStop(document);              
    for (const std::string& word : document_words) {
        double word_TF = 1.0 / document_words.size();
        word_index_[word][document_id] += word_TF;
    }     

    documents_[document_id] = {ComputeAverageRating(ratings), status};   
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query) const {
    return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
}

std::vector<Document> SearchServer::FindTopDocuments(const std::string& raw_query, DocumentStatus search_status) const {
    return FindTopDocuments(raw_query, [search_status](int document_id, DocumentStatus status, int rating) { return status == search_status; });
}

int SearchServer::GetDocumentCount() const {
    return static_cast<int>(documents_.size());
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(const std::string& raw_query, int document_id) const {
    Query parsed_query = ParseQuery(raw_query);

    std::vector<std::string> matched_words; 

    for (const std::string& word : parsed_query.minus_words) {
        if (word_index_.count(word) && word_index_.at(word).count(document_id)) {
            return std::tuple(matched_words, documents_.at(document_id).status);
        }
    }

    for (const std::string& word : parsed_query.plus_words) {
        if (word_index_.count(word) && word_index_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return std::tuple(matched_words, documents_.at(document_id).status);
}

int SearchServer::GetDocumentId(int index) const {
    if (index < 0 || index >= GetDocumentCount()) {
        throw std::out_of_range("Index is out of documents range."); 
    }

    int id = 0;
    for (const auto& [doc_id, data] : documents_){ 
        if (index == 0) {
            id = doc_id;
            break;
        }
        index--;
    }

    return id;
}

bool SearchServer::ContainsSpecialSymbols(const std::string& text) {
    for(const char c : text) {
        if (c >= 0 && c <= 31) {
            return true;
        }
    }
    return false;
}

std::vector<std::string> SearchServer::SplitIntoWordsNoStop(const std::string& text) const {
    if (ContainsSpecialSymbols(text)) {
        throw std::invalid_argument("Special symbols cannot be used in text.");
    }
    std::vector<std::string> words;
    for (const std::string& word : SplitIntoWords(text)) {
        if (stop_words_.count(word) == 0) {
            words.push_back(word);
        }
    }
    return words;
}

SearchServer::Query SearchServer::ParseQuery(const std::string& text) const {
    Query query_words;
    for (const std::string& word : SplitIntoWordsNoStop(text)) {
        if ( word[0] == '-' ) {
            if (word.size() == 1) {
                throw std::invalid_argument("After minus should be exception word.");
            }
            if (word[1] == '-') {
                throw std::invalid_argument("You cannot use more than one minus sign before an exception word");
            }
            std::string tmp = word.substr(1);
            if (stop_words_.count(tmp))
                continue;
            else 
                query_words.minus_words.insert(tmp);
        } 
        else {
            query_words.plus_words.insert(word);
        }
        
    }

    return query_words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty())
        return 0;

    return (std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
}

double SearchServer::ComputeWordIDF(const std::string& word) const {
    return std::log((1.0 * documents_.size() )/ word_index_.at(word).size());
}

