#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <numeric>
#include <cassert>

using namespace std;


const int MAX_RESULT_DOCUMENT_COUNT = 5;

enum class DocumentStatus {
    ACTUAL, 
    IRRELEVANT,
    BANNED,
    REMOVED
};

struct Document {
    int id = 0; 
    double relevance = 0.0;
    int rating = 0;

    Document() = default; 

    Document(int id_p, double rel_p, int rating_p) 
        : id(id_p), relevance(rel_p), rating(rating_p)
    {
    } 
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

class SearchServer {
    public:
        SearchServer(const string& stop_words_set) {
            string word;
            for (const char c : stop_words_set) {
                if (c == ' ') {
                    if (!word.empty()) {
                        stop_words_.insert(word);
                        word.clear();
                    }
                } else {
                    word += c;
                }
            }
            if (!word.empty()) {
                stop_words_.insert(word);
            }
        }

        template <typename T>
        SearchServer(const T& stop_words_set) {
            for (const string& word : stop_words_set) {
                if (word.empty()) {
                    continue;
                }

                stop_words_.insert(word);
            }
        }

        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
            const vector<string>& document_words = SplitIntoWordsNoStop(document);              
            for (const string& word : document_words) {
                double word_TF = 1.0 / document_words.size();
                word_index_[word][document_id] += word_TF;
            }     

            documents_[document_id] = {ComputeAverageRating(ratings), status};   
        }

        vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus search_status = DocumentStatus::ACTUAL) const {
            return FindTopDocuments(raw_query, [search_status](int document_id, DocumentStatus status, int rating) { return status == search_status; });
        }

        template <typename Function>
        vector<Document> FindTopDocuments(const string& raw_query, Function FilterDocument) const {
            const Query parsed_query = ParseQuery(raw_query);
        
            vector<Document> top_documents = FindAllDocuments(parsed_query, FilterDocument);
            
            const double EPSILON = 1e-6;

            sort(top_documents.begin(), top_documents.end(), 
                [EPSILON](const Document& el1, const Document& el2){
                    return el1.relevance > el2.relevance || 
                    (abs(el1.relevance - el2.relevance) < EPSILON && el1.rating > el2.rating) ;
                });             

            if ( top_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
                top_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
            }

            return top_documents;
        }

        int GetDocumentCount() const {
            return static_cast<int>(documents_.size());
        }

        // Return matched words from document with status(if query contains minus words, function returns empty vector)
        tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
            const Query parsed_query = ParseQuery(raw_query); 

            vector<string> matched_words; 

            for (const string& word : parsed_query.minus_words) {
                if (word_index_.count(word) && word_index_.at(word).count(document_id)) {
                    return tuple(matched_words, documents_.at(document_id).status);
                }
            }

            for (const string& word : parsed_query.plus_words) {
                if (word_index_.count(word) && word_index_.at(word).count(document_id)) {
                    matched_words.push_back(word);
                }
            }

            return tie(matched_words, documents_.at(document_id).status);
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

        template <typename Function>
        vector<Document> FindAllDocuments(const Query& query_words, Function CheckFilter) const { 
            map<int,double> matched_documents; // [id, relevance]

            for (const string& query_word : query_words.plus_words) {
                if (word_index_.count(query_word)) {
                    double word_IDF = ComputeWordIDF(query_word);
                    for (const auto& [id, word_TF] : word_index_.at(query_word)) {  
                        DocumentData document_info = documents_.at(id); 
                        if (CheckFilter(id, document_info.status, document_info.rating)) {
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
                result.push_back({id, relevance, documents_.at(id).rating});
            }

            return result;
        }
};

//----------------Framework for tests----------------------

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const string& t_str, const string& u_str, const string& file,
                     const string& func, unsigned line, const string& hint) {
    if (t != u) {
        cout << boolalpha;
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
        cout << t << " != "s << u << "."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const string& expr_str, const string& file, const string& func, unsigned line,
                const string& hint) {
    if (!value) {
        cout << file << "("s << line << "): "s << func << ": "s;
        cout << "ASSERT("s << expr_str << ") failed."s;
        if (!hint.empty()) {
            cout << " Hint: "s << hint;
        }
        cout << endl;
        abort();
    }
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)

#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

template <typename F>
void RunTestImpl(F func, const string& t_func) {
    func(); 
    cerr << t_func <<" OK"s << endl;
}

#define RUN_TEST(func) RunTestImpl(func, #func)

//--------------------------------------------------------

//Тест проверяет, что поисковая система исключает стоп-слова при добавлении документов
void TestExcludeStopWordsFromAddedDocumentContent() {
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = {1, 2, 3};

    SearchServer server{"in the"s};
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);

}

// void TestAddingDocument() {
//     const int doc_id = 11; 
//     const string content = "from Russia with love"s;
//     const vector<int> rating = {5, 4, 2};     
    
//     SearchServer server;
//     ASSERT_HINT(server.FindTopDocuments("Russia").empty(), "Initially nothing to look for"s);

//     server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
//     const vector<Document> found_doc = server.FindTopDocuments("love"s); 
//     ASSERT_EQUAL(found_doc[0].id, doc_id);   
// }

// void TestMinusWords() {
//     const int doc_id = 13; 
//     const string content = "from Russia with love"s;
//     const vector<int> rating = {5, 4, 2}; 

//     SearchServer server; 
//     server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, rating);
//     const vector<Document> found_doc = server.FindTopDocuments("with"s);

//     ASSERT_HINT(server.FindTopDocuments("Only friendship -love").empty(), 
//                 "Documents containing minus words from a search query should not be included in search results."s);
//     ASSERT_EQUAL(found_doc[0].id, doc_id);
// }

// void TestMatchDocument() {
//     const int doc1_id = 1;
//     const string content1 = "dogs and cats are friends"s;
//     const int doc2_id = 2;
//     const string content2 = "not all frogs are green"s;
//     const vector<int> rating = {4,4,4};

//     SearchServer server;
//     server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, rating);
//     server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, rating);

//     {
//         const auto [matched_words, status] = server.MatchDocument("white bears and green -dogs", doc1_id);
//         ASSERT_HINT(matched_words.empty(), "No matched words"s);
//     }

//     {
//         const auto [matched_words, status] = server.MatchDocument("white bears and green -dogs", doc2_id);
//         ASSERT_EQUAL(matched_words[0], "green"s);
//     }

// }

// void TestRelevance(){
//     const int doc1_id = 1;
//     const string content1 = "dogs and cats are friends"s;
//     const int doc2_id = 2;
//     const string content2 = "dogs are not green"s;
//     const int doc3_id = 3;
//     const string content3 = "he didn't know her friends"s;

//     const vector<int> rating = {1,3,2};

//     SearchServer server; 
//     server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, rating);
//     server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, rating);
//     server.AddDocument(doc3_id, content3, DocumentStatus::ACTUAL, rating);

//     const auto found_docs = server.FindTopDocuments("dogs are friends"); 
    
//     double relevance_doc1 = 0.0;
//     relevance_doc1 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 5.0); //dogs
//     relevance_doc1 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 5.0); //are 
//     relevance_doc1 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 5.0); //friends 

//     double relevance_doc2 = 0.0; 
//     relevance_doc2 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4.0); //dogs
//     relevance_doc2 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 4.0); //are

//     double relevance_doc3 = 0.0; 
//     relevance_doc3 += log(server.GetDocumentCount() * 1.0 / 2) * (1.0 / 5.0); //friends

//     ASSERT_EQUAL(found_docs.size(), 3u);
//     ASSERT_EQUAL(found_docs[0].relevance, relevance_doc1);
//     ASSERT_EQUAL(found_docs[1].relevance, relevance_doc2);
//     ASSERT_EQUAL(found_docs[2].relevance, relevance_doc3);
//     // check if documents were sorted correctly
//     ASSERT(found_docs[0].relevance >= found_docs[1].relevance);
//     ASSERT(found_docs[1].relevance >= found_docs[2].relevance);

// }

// void TestRating() {
//     const int doc1_id = 1;
//     const string content1 = "dogs and cats are friends"s;
//     const vector<int> rating1 = {1,3,2};
//     const int doc2_id = 2;
//     const string content2 = "dogs are not green"s;
//     const vector<int> rating2 = {4,3,1};

//     SearchServer server; 
//     server.AddDocument(doc1_id, content1, DocumentStatus::ACTUAL, rating1);
//     server.AddDocument(doc2_id, content2, DocumentStatus::ACTUAL, rating2);

//     const auto found_docs = server.FindTopDocuments("dogs are friends"); 

//     ASSERT_EQUAL(found_docs[0].rating, ((1+3+2) / static_cast<int>(rating1.size())));
//     ASSERT_EQUAL(found_docs[1].rating, ((4+3+1) / static_cast<int>(rating2.size())));
// }

// void TestUsersPredicateFunction() {
//     SearchServer search_server;
//     search_server.SetStopWords("и в на"s);
//     search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
//     search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
//     search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
//     search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

//     for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
//         ASSERT_EQUAL(document.id % 2, 0);
//     }
// }

// void TestDocStatusSort() {
//     SearchServer search_server;
//     search_server.SetStopWords("и в на"s);
//     search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
//     search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
//     search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
//     search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});

//     for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
//         ASSERT_EQUAL(document.id, 3);
//     }
// }

// // Функция TestSearchServer является точкой входа для запуска тестов
void TestSearchServer() {
    RUN_TEST(TestExcludeStopWordsFromAddedDocumentContent);
    // RUN_TEST(TestAddingDocument);
    // RUN_TEST(TestMinusWords);
    // RUN_TEST(TestMatchDocument);
    // RUN_TEST(TestRelevance);
    // RUN_TEST(TestRating);
    // RUN_TEST(TestUsersPredicateFunction);
    // RUN_TEST(TestDocStatusSort);
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

void PrintDocument(const Document& document) {
    cout << "{ "s
         << "document_id = "s << document.id << ", "s
         << "relevance = "s << document.relevance << ", "s
         << "rating = "s << document.rating
         << " }"s << endl;
}

int main() {
    TestSearchServer();

    // SearchServer search_server;
    // search_server.SetStopWords("и в на"s);
    // search_server.AddDocument(0, "белый кот и модный ошейник"s,        DocumentStatus::ACTUAL, {8, -3});
    // search_server.AddDocument(1, "пушистый кот пушистый хвост"s,       DocumentStatus::ACTUAL, {7, 2, 7});
    // search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, {5, -12, 2, 1});
    // search_server.AddDocument(3, "ухоженный скворец евгений"s,         DocumentStatus::BANNED, {9});
    // cout << "ACTUAL by default:"s << endl;
    // for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s)) {
    //     PrintDocument(document);
    // }
    // cout << "BANNED:"s << endl;
    // for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, DocumentStatus::BANNED)) {
    //     PrintDocument(document);
    // }
    // cout << "Even ids:"s << endl;
    // for (const Document& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
    //     PrintDocument(document);
    // }
    return 0;
} 