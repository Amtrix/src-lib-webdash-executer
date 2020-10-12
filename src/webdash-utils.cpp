#include <iostream>
#include <string>
#include <vector>
using namespace std;

string SubstituteKeywords(string src, string keyword, string replace_with) {
    size_t pos;

    while ((pos = src.find(keyword)) != string::npos) {
        src.replace(pos, keyword.size(), replace_with);
    }

    return src;
}

string ApplySubstitutions(string src, vector<pair<string, string>> substitutions) {
    
    for (auto& [key, value] : substitutions) {
        src = SubstituteKeywords(src, key, value);
    }

    return src;
}

string GetDirectoryOf(string full_fulename) {
    size_t pos = full_fulename.find_last_of("\\/");
    return (std::string::npos == pos) ? "" : full_fulename.substr(0, pos);
}