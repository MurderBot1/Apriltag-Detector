#include "RestartPolicy.hpp"
#include <string>
#include <algorithm>

const char* RestartPolicy::toString(Type type) {
    switch (type) {
        case NONE: return "none";
        case ALWAYS: return "always";
        case ON_FAILURE: return "on_failure";
        default: return "unknown";
    }
}

RestartPolicy::Type RestartPolicy::fromString(const std::string& str) {
    std::string lower = str;
    std::transform(lower.begin(), lower.end(), lower.begin(), 
        [](unsigned char c) { return std::tolower(c); });
    
    if (lower == "none") return NONE;
    if (lower == "always") return ALWAYS;
    if (lower == "on_failure" || lower == "on_failure") return ON_FAILURE;
    
    return ON_FAILURE; // Default
}
