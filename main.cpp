#include <iostream>

#include "paginator.h"
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "request_queue.h"
#include "remove_duplicates.h"

using namespace std::string_literals;

int main() {
    SearchServer search_server("and with"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
    search_server.AddDocument(2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 3});
    search_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2, 8});
    search_server.AddDocument(4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 3, 2});
    search_server.AddDocument(5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 1, 1});
    search_server.AddDocument(9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 1, 1});
    
    std::cout << "Before duplicates removed: "s << search_server.GetDocumentCount() << std::endl;
    RemoveDuplicates(search_server);
    std::cout << "After duplicates removed: "s << search_server.GetDocumentCount() << std::endl;

    return 0;
}