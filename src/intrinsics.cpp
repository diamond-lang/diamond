#include "intrinsics.hpp"

std::unordered_map<std::string, std::pair<std::vector<Ast::Type>, Ast::Type>> interfaces = {
    {"+", 
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("$t")}
    },
    {"*", 
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("$t")}
    },
    {"/", 
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("$t")}
    },
    {"-",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("$t")}
    },
    {"<",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {"<=",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {">",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {">=",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {"==",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {"!=",
        {{Ast::Type("$t"), Ast::Type("$t")}, Ast::Type("bool")}
    },
    {"print",
        {{Ast::Type("$t")}, Ast::Type("void")}
    }
};

std::unordered_map<std::string, std::vector<std::pair<std::vector<Ast::Type>, Ast::Type>>> intrinsics = {
    {"+", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("float64")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("int64")}
    }},
    {"*", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("float64")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("int64")}
    }},
    {"/", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("float64")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("int64")}
    }},
    {"%", {
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("int64")}
    }},
    {"-", {
        {{Ast::Type("float64")}, Ast::Type("float64")},
        {{Ast::Type("int64")}, Ast::Type("int64")},
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("float64")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("int64")},
    }},
    {"<", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {"<=", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {">", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {">=", {
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {"==", {
        {{Ast::Type("bool"), Ast::Type("bool")}, Ast::Type("bool")},
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {"!=", {
        {{Ast::Type("bool"), Ast::Type("bool")}, Ast::Type("bool")},
        {{Ast::Type("float64"), Ast::Type("float64")}, Ast::Type("bool")},
        {{Ast::Type("int64"), Ast::Type("int64")}, Ast::Type("bool")}
    }},
    {"not", {
        {{Ast::Type("bool")}, Ast::Type("bool")}
    }},
    {"and", {
        {{Ast::Type("bool"), Ast::Type("bool")}, Ast::Type("bool")}
    }},
    {"or", {
        {{Ast::Type("bool"), Ast::Type("bool")}, Ast::Type("bool")}
    }},
    {"print", {
        {{Ast::Type("float64")}, Ast::Type("void")},
        {{Ast::Type("int64")}, Ast::Type("void")},
        {{Ast::Type("bool")}, Ast::Type("void")}
    }}
};