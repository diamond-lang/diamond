#ifndef SHARED_HPP
#define SHARED_HPP

#include <vector>
#include <cassert>
#include <variant>
#include <filesystem>

struct Error {
    std::string value;

    Error() : value("") {}
    Error(std::string value) : value(value) {}
    ~Error() {}
};

using Errors = std::vector<Error>;

struct Ok {};

template <class T1, class T2>
struct Result {
    std::variant<T1, T2> value;

    Result() {}
    Result(T1 value) : value(value) {}
    Result(T2 error) : value(error) {}
    ~Result() {}

    bool is_ok()    {return std::holds_alternative<T1>(this->value);}
    bool is_error() {return !std::holds_alternative<T1>(this->value);}
    T1 get_value()  {
        assert(this->is_ok() == true);
        return std::get<T1>(this->value);
    }
    T2 get_error() {
        assert(this->is_error() == true);
        return std::get<T2>(this->value);
    }
    T2 get_errors()  {
        assert(this->is_error() == true);
        return std::get<T2>(this->value);
    }
};

struct Location {
    size_t line;
    size_t column;
    std::filesystem::path file;

    Location(size_t line, size_t column, std::filesystem::path file) : line(line), column(column), file(file) {}
};

#define todo() assert(false);

template<typename V, typename T, size_t I = 0>
constexpr size_t getIndex() {
    if constexpr (I >= std::variant_size_v<V>) {
        return (std::variant_size_v<V>);
    } else {    
        if constexpr (std::is_same_v<std::variant_alternative_t<I, V>, T>) {
            return (I);
        } else {
            return (getIndex<V, T, I + 1>());
        }
    }
}

#endif
