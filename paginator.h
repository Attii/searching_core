#pragma once

#include <vector>
#include <algorithm>

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
            return distance(begin_it_, end_it_);
        }
    
    private:
        It begin_it_;
        It end_it_;
};

template <typename It>
class Paginator {
    public:
        Paginator(It begin_it, It end_it, size_t page_size) {
            for (auto iter = begin_it; iter != end_it; 
                    advance(iter, std::min(page_size, static_cast<size_t>(distance(iter, end_it))))) {
                auto page_start = iter;
                auto page_end = next(iter, std::min(page_size, static_cast<size_t>(distance(iter, end_it))));;
                pages_.push_back(IteratorRange(page_start, page_end));
            }
        }
    
        typename std::vector<IteratorRange<It>>::const_iterator begin() const {
            return pages_.begin();
        }

        typename std::vector<IteratorRange<It>>::const_iterator end() const {
            return pages_.end();
        }

        typename std::vector<IteratorRange<It>>::const_iterator size() const {
            return pages_.size();
        }

    private:
        std::vector<IteratorRange<It>> pages_;
};

template <typename Container>
auto Paginate(const Container& c, size_t page_size) {
    return Paginator(begin(c), end(c), page_size);
}

template <typename It>
std::ostream& operator<<(std::ostream& out, IteratorRange<It> page) {
    for (auto it = page.begin(); it != page.end(); ++it) {
        out << *it;
    }
    return out;
}