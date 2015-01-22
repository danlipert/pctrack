#include "defs.hpp"
#include <fstream>

std::vector<char> load_file(const std::string &path) {
    std::fstream fp;
    fp.open(path, fp.in | fp.binary);
    if (!fp.good()) {
        die("Could not read file: %s", path.c_str());
    }
    fp.seekg(0, fp.end);
    auto length = fp.tellg();
    fp.seekg(0, fp.beg);
    std::vector<char> result(length);
    fp.read(result.data(), length);
    if (!fp.good()) {
        die("Could not read file: %s", path.c_str());
    }
    return result;
}
