#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;

enum class DocumentStatus {
    ACTUAL, 
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id; 
    double relevance;
    int rating = 0; 
};


string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<int> ReadRatings() {
    vector<int> res; 
    int tmp, num_of_ratings;
    cin >> num_of_ratings; 
    for (int i = 0; i < num_of_ratings; ++i) {
        cin >> tmp; 
        res.push_back(tmp); 
    }
    return res;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
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

void PrintMatchDocumentResult(int document_id, const vector<string>& words, DocumentStatus status) {
    cout << "{ "s
         << "document_id = "s << document_id << ", "s
         << "status = "s << static_cast<int>(status) << ", "s
         << "words ="s;
    for (const string& word : words) {
        cout << ' ' << word;
    }
    cout << "}"s << endl;
}


class SearchServer {
    public:
        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
            const vector<string>& document_words = SplitIntoWordsNoStop(document);              
            for (const string& word : document_words) {
                double word_TF = 1.0 / document_words.size();
                word_index_[word][document_id] += word_TF;
            }     

            documents_[document_id] = {ComputeAverageRating(ratings), status};   
        }

        void SetStopWords(const string& text) {
            for (const string& word : SplitIntoWords(text)) {
                stop_words_.insert(word);
            }
        }

        vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
            const Query parsed_query = ParseQuery(raw_query);
        
            vector<Document> top_documents = FindAllDocuments(parsed_query, status);
            
            sort(top_documents.begin(), top_documents.end(), 
                [](const Document& el1, const Document& el2){
                    return el1.relevance > el2.relevance;
                });             

            if ( top_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

            return top_documents;
        }

        int GetDocumentCount() const {
            return static_cast<int>(documents_.size());
        }

        tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
            const Query parsed_query = ParseQuery(raw_query); 

            set<string> plus_words_from_document; 

            for (const string& word: parsed_query.plus_words) {
                if ( parsed_query.minus_words.count(word) ) {
                    continue;
                }
                if (word_index_.count(word) && word_index_.at(word).count(document_id)) {
                    plus_words_from_document.insert(word);
                }
            }

            vector<string> result; 
            for (const string& word : plus_words_from_document) {
                result.push_back(word);
            }

            return tuple(result, documents_.at(document_id).status);
        }

    private:
        struct Query {
            set<string> plus_words; 
            set<string> minus_words; 
        };

        struct DocumentData {
            int rating; 
            DocumentStatus status; 
        };

        map<int,DocumentData> documents_;

        map<string, map<int, double>> word_index_;

        set<string> stop_words_;

        vector<string> SplitIntoWordsNoStop(const string& text) const {
            vector<string> words;
            for (const string& word : SplitIntoWords(text)) {
                if (stop_words_.count(word) == 0) {
                    words.push_back(word);
                }
            }
            return words;
        }

        Query ParseQuery(const string& text) const {
            Query query_words;
            for (const string& word : SplitIntoWordsNoStop(text)) {
                if ( word[0] == '-' ) {
                    string tmp = word.substr(1);
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

        static int ComputeAverageRating(const vector<int>& ratings) {
            if (ratings.empty())
                return 0;

            return (accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size()));
        }

        double ComputeWordIDF(const string& word) const {
            return log((1.0 * documents_.size() )/ word_index_.at(word).size());
        }

        vector<Document> FindAllDocuments(const Query& query_words, const DocumentStatus status) const { 
            map<int,double> matched_documents; // [id, relevance]

            for (const string& query_word : query_words.plus_words) {
                if (word_index_.count(query_word)) {
                    double word_IDF = ComputeWordIDF(query_word);
                    for (const auto& [id, word_TF] : word_index_.at(query_word)) {  
                        if (documents_.at(id).status == status ) {
                            matched_documents[id] += word_TF * word_IDF;
                        }
                    }
                }
            }

            for (const string& minus_word : query_words.minus_words) {
                if (word_index_.count(minus_word)) {
                    for (const auto& [id, word_TF]: word_index_.at(minus_word)) {
                        matched_documents.erase(id);
                    }
                }
            }

            vector<Document> result;
            for (const auto& [id, relevance] : matched_documents) {
                if (documents_.at(id).status == status){
                    result.push_back({id, relevance, documents_.at(id).rating});
                }
            }

            return result;
        }
};

// SearchServer CreateSearchServer() {
//     SearchServer server;
    
//     server.SetStopWords(ReadLine()); 

//     const int document_count = ReadLineWithNumber();
//     for (int document_id = 0; document_id < document_count; document_id++) {
//         server.AddDocument(document_id, ReadLine(), ReadRatings());
//     }

//     return server;
// }

// int main() {    
//     const SearchServer server = CreateSearchServer(); 

//     const string query = ReadLine();
//     // Выводим результаты поиска по запросу query
//     for (const auto& [id, relevance, rating] : server.FindTopDocuments(query)) {
//         cout << "{ document_id = "s << id << ", relevance = "s << relevance << 
//                 ", rating = "s << rating << " }"s << endl;
//     }
// } 

int main() {
    SearchServer search_server;
    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    const int document_count = search_server.GetDocumentCount();
    for (int document_id = 0; document_id < document_count; ++document_id) {
        const auto [words, status] = search_server.MatchDocument("пушистый кот"s, document_id);
        PrintMatchDocumentResult(document_id, words, status);
    }
} 