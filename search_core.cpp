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
#include <optional>
#include <stdexcept>

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
        : id(id_p)
        , relevance(rel_p)
        , rating(rating_p)
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
        explicit SearchServer(const string& stop_words_set)
            :   SearchServer(SplitIntoWords(stop_words_set))
        {
        }

        template <typename T>
        explicit SearchServer(const T& stop_words_set) {
            for (const string& word : stop_words_set) {
                if (ContainsSpecialSymbols(word)) {
                    throw invalid_argument("Stop words can't contain special symbols"s);
                }
                if (word.empty()) {
                    continue;
                }

                stop_words_.insert(word);
            }
        }

        // Defines an invalid document id
        // You can refer to this constant as SearchServer::INVALID_DOCUMENT_ID
        inline static constexpr int INVALID_DOCUMENT_ID = -1;

        void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
            if (document_id < 0) {
                throw invalid_argument("Document ID can't be negative."s);
            }

            if (documents_.count(document_id)) {
                throw invalid_argument("You cannot add the same document ID"s);
            }

            const vector<string>& document_words = SplitIntoWordsNoStop(document);              
            for (const string& word : document_words) {
                double word_TF = 1.0 / document_words.size();
                word_index_[word][document_id] += word_TF;
            }     

            documents_[document_id] = {ComputeAverageRating(ratings), status};   
        }

        vector<Document> FindTopDocuments(const string& raw_query) const {
            return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
        }

        vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus search_status) const {
            return FindTopDocuments(raw_query, [search_status](int document_id, DocumentStatus status, int rating) { return status == search_status; });
        }

        template <typename Function>
        vector<Document> FindTopDocuments(const string& raw_query, Function FilterDocument) const {
            Query parsed_query = ParseQuery(raw_query);
        
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

        // Full result with matched words from document with status(if query contains minus words, function returns empty vector)
        tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
            Query parsed_query = ParseQuery(raw_query);

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

        int GetDocumentId(int index) const {
            if (index < 0 || index >= GetDocumentCount()) {
                throw out_of_range("Index is out of documents range."s); 
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

        map<string, map<int, double>> word_index_; // word : document index : word term frequency in document

        set<string> stop_words_;

        static bool ContainsSpecialSymbols(const string& text) {
            for(const char c : text) {
                if (c >= 0 && c <= 31) {
                    return true;
                }
            }
            return false;
        }

        vector<string> SplitIntoWordsNoStop(const string& text) const {
            if (ContainsSpecialSymbols(text)) {
                throw invalid_argument("Special symbols cannot be used in text.");
            }
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
                    if (word.size() == 1) {
                        throw invalid_argument("After minus should be exception word."s);
                    }
                    if (word[1] == '-') {
                        throw invalid_argument("You cannot use more than one minus sign before an exception word"s);
                    }
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

//-----------------------------------------------------


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
         << "rating = "s << document.rating << " }"s << endl;
}

template <typename It>
class IteratorRange {
    public:
        explicit IteratorRange(It begin_it, It end_it)
            : begin_it_(begin_it)
            , end_it_(end_it) 
        {            
        }
    
        It begin() {
            return begin_it_;
        }

        It end() {
            return end_it_;
        }

        size_t size() {
            return distance (begin_it_, end_it_);
        }
    
    private:
        It begin_it_;
        It end_it_;
};

template <typename It>
class Paginator {
    public:
        Paginator(It begin_it, It end_it, size_t page_size) {
            for (auto iter = begin_it; iter != end_it; advance(iter, min(page_size, static_cast<size_t>(distance(iter, end_it))))) {
                auto page_start = iter;
                auto page_end = next(iter, min(page_size, static_cast<size_t>(distance(iter, end_it))));;
                pages_.push_back(IteratorRange(page_start, page_end));
            }
        }
    
        typename vector<IteratorRange<It>>::const_iterator begin() const {
            return pages_.begin();
        }

        typename vector<IteratorRange<It>>::const_iterator end() const {
            return pages_.end();
        }

        typename vector<IteratorRange<It>>::const_iterator size() const {
            return pages_.size();
        }

    private:
        vector<IteratorRange<It>> pages_;
};

ostream& operator<<(ostream& output, Document  doc) {
    output << "{ document_id = " << doc.id << ", relevance = " << doc.relevance << ", rating = " << doc.rating << " }";
    return output;
}

template <typename It>
ostream& operator<<(ostream& out, IteratorRange<It> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}
int main() {
    SearchServer search_server("and with"s);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "big cat nasty hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "big dog cat Vladislav"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "big dog hamster Borya"s, DocumentStatus::ACTUAL, {1, 1, 1});
    const auto search_results = search_server.FindTopDocuments("curly dog"s);
    int page_size = 2;
    const auto pages = Paginate(search_results, page_size);
    // Выводим найденные документы по страницам
    for (auto page = pages.begin(); page != pages.end(); ++page) {
        cout << *page << endl;
        cout << "Page break"s << endl;
    }
} 