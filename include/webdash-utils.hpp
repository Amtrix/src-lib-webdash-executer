#include <string>
#include <vector>
using namespace std;

string SubstituteKeywords(string src, string keyword, string replace_with);

string ApplySubstitutions(string src, vector<pair<string, string>> substitutions);

string GetDirectoryOf(string full_fulename);