#include "intrinsics.hpp"

std::unordered_map<std::string, std::pair<std::vector<ast::Type>, ast::Type>> interfaces = {
    {"+", 
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("$t")}
    },
    {"*", 
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("$t")}
    },
    {"/", 
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("$t")}
    },
    {"-",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("$t")}
    },
    {"<",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {"<=",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {">",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {">=",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {"==",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {"!=",
        {{ast::Type("$t"), ast::Type("$t")}, ast::Type("bool")}
    },
    {"print",
        {{ast::Type("$t")}, ast::Type("void")}
    }
};

std::unordered_map<std::string, std::vector<std::pair<std::vector<ast::Type>, ast::Type>>> intrinsics = {
    {"+", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")}
    }},
    {"*", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")}
    }},
    {"/", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")}
    }},
    {"%", {
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")}
    }},
    {"-", {
        {{ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64")}, ast::Type("int64")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("float64")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("int64")},
    }},
    {"<", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {"<=", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {">", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {">=", {
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {"==", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {"!=", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")},
        {{ast::Type("float64"), ast::Type("float64")}, ast::Type("bool")},
        {{ast::Type("int64"), ast::Type("int64")}, ast::Type("bool")}
    }},
    {"not", {
        {{ast::Type("bool")}, ast::Type("bool")}
    }},
    {"and", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")}
    }},
    {"or", {
        {{ast::Type("bool"), ast::Type("bool")}, ast::Type("bool")}
    }},
    {"print", {
        {{ast::Type("float64")}, ast::Type("void")},
        {{ast::Type("int64")}, ast::Type("void")},
        {{ast::Type("bool")}, ast::Type("void")}
    }}
};