#pragma once
#include <string>

namespace micro_sd_text_reader {

inline std::string extract_filename_from_path(const std::string& path) {
    size_t pos = path.find_last_of("/\\");
    if (pos != std::string::npos) {
        return path.substr(pos + 1);
    }
    return path;
}

} // namespace micro_sd_text_reader 