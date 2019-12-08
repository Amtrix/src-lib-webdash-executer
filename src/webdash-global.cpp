#include "webdash-global.hpp"

#include "myworldcpp/utility.hpp"
#include "myworldcpp/logging.hpp"

#include <nlohmann/json.hpp>
#include <queue>
using namespace std;
using namespace myworld::logging;
using json = nlohmann::json;

void WebDashGlobalConfig::Create() {
    if (_config.has_value()) return;

    _config = WebDashGlobalConfig();
}

WebDashGlobalConfig WebDashGlobalConfig::GetConfig() {
    Create();

    return _config.value();
}

vector<pair<string, string>> WebDashGlobalConfig::GetAllDefinitions() {
    vector<pair<string, string>> ret;

    const string path = myworld::utility::GetRepositoryRoot() + "/definitions.json";

    ifstream configStream;
    try {
        Log(Type::INFO, "UXPath: " + path);
        configStream.open(path.c_str(), ifstream::in);
    } catch (...) {
        Log(Type::ERR, "Issues opening to definitions file.");
        return ret;
    }

    json _defs;
    try {
        configStream >> _defs;
    } catch (...) {
        Log(Type::ERR, "Was unable to parse the config '" + path + "' file. Format error?");
        return ret;
    }

    queue<pair<string, json>> Q;
    Q.push(make_pair("", _defs));

    while (!Q.empty()) {
        pair<string, json> u = Q.front();
        Q.pop();

        for (auto& [key, value] : u.second.items()) {
            if (value.is_object()) {
                Q.push(make_pair(u.first + "." + key, value));
            } else {
                ret.push_back(make_pair(
                    "$#" + u.first + "." + key,
                    value.get<std::string>()
                ));
            }
        }
    }

    return ret;
}

std::optional<WebDashGlobalConfig> WebDashGlobalConfig::_config = nullopt;