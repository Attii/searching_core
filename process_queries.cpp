#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(
    const SearchServer& search_server,
    const std::vector<std::string>& queries) {
        std::vector<std::vector<Document>> result(queries.size()); 

        std::transform(std::execution::par, 
                        queries.begin(), queries.end(),
                        result.begin(), 
                        [&](std::string query){
                            return search_server.FindTopDocuments(query);
                        });
        
        return result;
    }

std::list<Document> ProcessQueriesJoined(
    const SearchServer& search_server,
    const std::vector<std::string>& queries){
        std::list<Document> result;

        for (const auto& results : ProcessQueries(search_server, queries)) {
            for (const auto& document : results) {
                result.push_back(document);
            }
        } 

        return result;
    }     
