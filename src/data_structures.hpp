#ifndef DATA_STRUCTURES_HPP
#define DATA_STRUCTURES_HPP

#include <vector>

// Set
template <typename T>
struct Set {
    std::vector<T> elements;

    Set() {}
    
    Set(std::vector<T> elements) {
        for (size_t i = 0; i < elements.size(); i++) {
            this->insert(elements[i]);
        }
    }

    void insert(T element) {
        for (size_t i = 0; i < this->elements.size(); i++) {
            if (this->elements[i] == element) {
                return;
            }
        }
        this->elements.push_back(element);
    }

    bool contains(T element) {
        for (size_t i = 0; i < this->elements.size(); i++) {
            if (this->elements[i] == element) return true;
        }
        return false;
    }

    size_t size() const {
        return this->elements.size();
    }

    void merge(Set<T> b) {
        for (size_t i = 0; i < b.elements.size(); i++) {
            this->insert(b.elements[i]);
        }
    }

    Set<T> intersect(Set<T> b) {
        Set<T> intersection;
        for (auto& element: this->elements) {
            if (b.contains(element)) {
                intersection.elements.push_back(element);
            }
        }
        return intersection;
    }
};

template <typename T>
std::vector<Set<T>> merge_sets_with_shared_elements(std::vector<Set<T>> sets) {
    size_t i = 0;
    while (i < sets.size()) {
        bool merged = false;

        for (size_t j = i + 1; j < sets.size(); j++) {
            Set<T> intersection = sets[i].intersect(sets[j]);
            if (intersection.size() != 0) {
                // Merge sets
                sets[i].merge(sets[j]);

                // Erase duplicated set
                sets.erase(sets.begin() + j);
                merged = true;
                break;
            }
        }

        if (!merged) i++;
    }

    return sets;
}

#endif