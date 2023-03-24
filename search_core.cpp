#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;


struct Document {
    int id; 
    double relevance;
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

class SearchServer {
    public:
        void AddDocument(int document_id, const string& document) {
            documents_count_ += 1;

            const vector<string>& document_words = SplitIntoWordsNoStop(document);              
            for (const string& word : document_words) {
                double word_TF = (double)count(document_words.begin(), document_words.end(), word) / document_words.size();
                documents_[word].insert({document_id, word_TF});
            }            
        }

        void SetStopWords(const string& text) {
            for (const string& word : SplitIntoWords(text)) {
                stop_words_.insert(word);
            }
        }

        vector<Document> FindTopDocuments(const string& raw_query) const {
            const Query parsed_query = ParseQuery(raw_query);
        
            vector<Document> top_documents = FindAllDocuments(parsed_query);
            
            sort(top_documents.begin(), top_documents.end(), 
                [](const Document& el1, const Document& el2){
                    return el1.relevance > el2.relevance;
                }); 
            

            if ( top_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

            return top_documents;
        }

    private:
        struct Query {
            set<string> plus_words; 
            set<string> minus_words; 
        };

        map<string, map<int, double>> documents_;

        set<string> stop_words_;

        int documents_count_ = 0;

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

        vector<Document> FindAllDocuments(const Query& query_words) const {
            vector<Document> result; 
            map<int,double> matched_documents; // [id, relevance]

            for (const string& query_word : query_words.plus_words) {
                if (documents_.count(query_word)) {
                    for (const auto& [id, word_TF] : documents_.at(query_word)) {
                        double word_IDF = log((double)documents_count_ / documents_.at(query_word).size());  
                        matched_documents[id] += word_TF * word_IDF;
                    }
                }
            }

            for (const string& minus_word : query_words.minus_words) {
                if (documents_.count(minus_word)) {
                    for (const auto& [id, word_TF]: documents_.at(minus_word)) {
                        matched_documents.erase(id);
                    }
                }
            }

            for (const auto& [id, relevance] : matched_documents) {
                result.push_back({id, relevance});
            }

            return result;
        }
};

SearchServer CreateSearchServer() {
    SearchServer server;
    
    server.SetStopWords(ReadLine()); 

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; document_id++) {
        server.AddDocument(document_id, ReadLine());
    }

    return server;
}

int main() {    
    const SearchServer server = CreateSearchServer(); 

    const string query = ReadLine();
    // Выводим результаты поиска по запросу query
    for (const auto& [id, relevance] : server.FindTopDocuments(query)) {
        cout << "{ document_id = "s << id << ", relevance = "s << relevance << " }"s
             << endl;
    }
} 