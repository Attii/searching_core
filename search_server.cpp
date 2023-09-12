#include "search_server.h"

std::set<int>::iterator SearchServer::begin() {
    return documents_id_.begin();
}

std::set<int>::const_iterator SearchServer::begin() const{
    return documents_id_.begin();
}

std::set<int>::iterator SearchServer::end() {
    return documents_id_.end();
}

std::set<int>::const_iterator SearchServer::end() const{
    return documents_id_.end();
}

void SearchServer::AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings) {
    if (document_id < 0) {
        throw std::invalid_argument("Document ID can't be negative");
    }

    if (documents_.count(document_id)) {
        throw std::invalid_argument("You cannot add the same document ID");
    }

    const std::vector<std::string>& document_words = SplitIntoWordsNoStop(document);              
    for (const std::string& word : document_words) {
        double word_TF = 1.0 / document_words.size();
        word_to_document_index_[word][document_id] += word_TF;
        document_to_word_index_[document_id][word] += word_TF;
    }     

    documents_id_.insert(document_id);
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
    Query parsed_query = ParseQuery(raw_query, true);

    std::vector<std::string> matched_words; 

    for (const std::string& word : parsed_query.minus_words) {
        if (word_to_document_index_.count(word) && word_to_document_index_.at(word).count(document_id)) {
            return std::tuple(matched_words, documents_.at(document_id).status);
        }
    }

    for (const std::string& word : parsed_query.plus_words) {
        if (word_to_document_index_.count(word) && word_to_document_index_.at(word).count(document_id)) {
            matched_words.push_back(word);
        }
    }

    return std::tuple(matched_words, documents_.at(document_id).status);
}

std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(std::execution::sequenced_policy policy, const std::string& raw_query, int document_id) const {
    return MatchDocument(raw_query, document_id);
}


std::tuple<std::vector<std::string>, DocumentStatus> SearchServer::MatchDocument(std::execution::parallel_policy policy, const std::string& raw_query, int document_id) const {
    #include <iostream>
    
    if (documents_id_.count(document_id) == 0) {
        throw std::invalid_argument("Document ID is out of range");
    }

    Query parsed_query = ParseQuery(raw_query);

    std::vector<std::string> matched_words; 

    if (std::any_of(policy, parsed_query.minus_words.begin(), parsed_query.minus_words.end(), 
                    [&](const auto& m_word){
                        return document_to_word_index_.at(document_id).count(m_word); 
                    })) 
    {
        return std::tuple(matched_words, documents_.at(document_id).status);
    }
    
    matched_words.resize(parsed_query.plus_words.size());

    auto end_it = std::copy_if(policy, parsed_query.plus_words.begin(), parsed_query.plus_words.end(), matched_words.begin(),
                                [&](const std::string p_word){ 
                                    return document_to_word_index_.at(document_id).count(p_word) == 1;
                                });
    
    std::sort(policy, matched_words.begin(), end_it);
    matched_words.erase(std::unique(matched_words.begin(), end_it), matched_words.end());

    return std::tuple(matched_words, documents_.at(document_id).status); 
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

SearchServer::Query SearchServer::ParseQuery(const std::string& text, bool do_sort) const {
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
                query_words.minus_words.push_back(tmp);
        } 
        else {
            query_words.plus_words.push_back(word);
        }        
    }

    if(do_sort) {
        std::sort(query_words.minus_words.begin(), query_words.minus_words.end());
        auto it = std::unique(query_words.minus_words.begin(), query_words.minus_words.end());
        query_words.minus_words.erase(it, query_words.minus_words.end());

        std::sort(query_words.plus_words.begin(), query_words.plus_words.end());
        it = std::unique(query_words.plus_words.begin(), query_words.plus_words.end());
        query_words.plus_words.erase(it, query_words.plus_words.end());
    }
    
    return query_words;
}

int SearchServer::ComputeAverageRating(const std::vector<int>& ratings) {
    if (ratings.empty())
        return 0;

    return (std::accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
}

double SearchServer::ComputeWordIDF(const std::string& word) const {
    return std::log((1.0 * documents_.size() )/ word_to_document_index_.at(word).size());
}

const std::map<std::string, double>& SearchServer::GetWordFrequencies(int document_id) const {
    static const std::map<std::string, double> blank_word_frequencies;

    if(document_to_word_index_.count(document_id)) {
        return document_to_word_index_.at(document_id);
    } 

    return blank_word_frequencies;
}

void SearchServer::RemoveDocument(int document_id) {
    if(documents_id_.count(document_id)) {
        documents_.erase(document_id);

        for (const auto& [word, word_TF] : document_to_word_index_.at(document_id)) {
            word_to_document_index_.at(word).erase(document_id);
        }

        document_to_word_index_.erase(document_id);
        documents_id_.erase(document_id);
    }
}

void SearchServer::RemoveDocument(std::execution::sequenced_policy policy, int document_id) {
    RemoveDocument(document_id);
}

void SearchServer::RemoveDocument(std::execution::parallel_policy policy, int document_id){
    if(documents_id_.count(document_id)) {
        std::vector<const std::string*> vec_ptr(document_to_word_index_.at(document_id).size());

        std::transform(policy, 
                        document_to_word_index_.at(document_id).begin(), document_to_word_index_.at(document_id).end(), 
                        vec_ptr.begin(),
                        [](const auto& var){return &var.first;});

        std::for_each(policy, vec_ptr.begin(), vec_ptr.end(), [&](auto word) {
            word_to_document_index_.at(*word).erase(document_id);
        });

        documents_.erase(document_id);
        document_to_word_index_.erase(document_id);
        documents_id_.erase(document_id);
    }  
}