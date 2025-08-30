#include "lld-c.h"

#include <lld/Common/Driver.h>
#include <llvm/Support/raw_ostream.h>
#include <stdbool.h>

#include <iostream>
#include <string>

#include "common.h"
#include "types.h"

LLD_HAS_DRIVER(macho)

extern "C" bool lld_link(CStringList args) {
    std::string output = "";
    std::string errors = "";
    llvm::raw_string_ostream outputStream(output);
    llvm::raw_string_ostream errorsStream(errors);
    std::vector<const char*> argsAsCStrings;
    for (uint32_t i = 0; i < list_size(args); i++) {
        printf("%s ", *list_get(args, i));
        argsAsCStrings.push_back(*list_get(args, i));
    }
    bool result;
    switch (currentPlatform()) {
    case Windows: todo();
    case Linux: todo();
    case MacOs: {
        result = lld::macho::link(
            argsAsCStrings,
            outputStream,
            errorsStream,
            false,
            false
        );
        break;
    }
    }
    if (result == false) {
        std::cout << errors;
        return false;
    }
    return true;
}