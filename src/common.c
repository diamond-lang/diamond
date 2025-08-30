#include "common.h"

Platform currentPlatform() {
#ifdef _WIN32
    return Windows;
#elif __APPLE__
    return MacOs;
#elif __linux__
    return Linux;
#endif
}