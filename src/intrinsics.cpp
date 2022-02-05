#include "intrinsics.hpp"

std::unordered_map<std::string, std::pair<std::vector<Type>, Type>> interfaces = {
    {"+", 
        {{Type("$t"), Type("$t")}, Type("$t")}
    },
    {"*", 
        {{Type("$t"), Type("$t")}, Type("$t")}
    },
    {"/", 
        {{Type("$t"), Type("$t")}, Type("$t")}
    },
    {"-",
        {{Type("$t"), Type("$t")}, Type("$t")}
    },
    {"<",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {"<=",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {">",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {">=",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {"==",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {"!=",
        {{Type("$t"), Type("$t")}, Type("bool")}
    },
    {"print",
        {{Type("$t")}, Type("void")}
    }
};

std::unordered_map<std::string, std::vector<std::pair<std::vector<Type>, Type>>> intrinsics = {
    {"+", {
        {{Type("$t"), Type("$t")}, Type("$t")},
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"*", {
        {{Type("$t"), Type("$t")}, Type("$t")},
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"/", {
        {{Type("$t"), Type("$t")}, Type("$t")},
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")}
    }},
    {"-", {
        {{Type("$t"), Type("$t")}, Type("$t")},
        {{Type("float64")}, Type("float64")},
        {{Type("int64")}, Type("int64")},
        {{Type("float64"), Type("float64")}, Type("float64")},
        {{Type("int64"), Type("int64")}, Type("int64")},
    }},
    {"<", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"<=", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {">", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {">=", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"==", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("bool"), Type("bool")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"!=", {
        {{Type("$t"), Type("$t")}, Type("bool")},
        {{Type("bool"), Type("bool")}, Type("bool")},
        {{Type("float64"), Type("float64")}, Type("bool")},
        {{Type("int64"), Type("int64")}, Type("bool")}
    }},
    {"not", {
        {{Type("bool")}, Type("bool")}
    }},
    {"and", {
        {{Type("bool"), Type("bool")}, Type("bool")}
    }},
    {"or", {
        {{Type("bool"), Type("bool")}, Type("bool")}
    }},
    {"print", {
        {{Type("float64")}, Type("void")},
        {{Type("int64")}, Type("void")},
        {{Type("bool")}, Type("void")}
    }}
};