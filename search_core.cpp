#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;


struct Document {
    int id; 
    int relevance;
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
            const vector<string> words = SplitIntoWordsNoStop(document);
            documents_.push_back({document_id, words});
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
        struct DocumentContent {
            int id = 0;
            vector <string> words;
        }; 

        struct Query {
            set<string> plus_words; 
            set<string> minus_words; 
        };

        vector<DocumentContent> documents_;

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

        vector<Document> FindAllDocuments(const Query& query_words) const {
            vector<Document> matched_documents;

            for (const auto& document : documents_) {
                const int relevance = MatchDocument(document, query_words); 
                if ( relevance > 0 )
                {
                    matched_documents.push_back({document.id, relevance});
                }
            }
            return matched_documents; 
        }

        static int MatchDocument(const DocumentContent& content, const Query& query_words) 
        {
            if ( query_words.plus_words.empty() )
                return 0;

            for (const string& word : content.words) {
                if ( query_words.minus_words.count(word) )
                    return 0;
            }
            
            set<string> matched_words;
            
            for ( const string& word : content.words )
            {
                if ( matched_words.count(word) )
                    continue;
                if ( query_words.plus_words.count(word) )
                    matched_words.insert(word);
            } 

            return (int)(matched_words.size());
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