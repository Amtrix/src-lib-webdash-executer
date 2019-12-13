#include "webdash-core.hpp"

#include "myworldcpp/utility.hpp"
#include "myworldcpp/logging.hpp"

#include <nlohmann/json.hpp>
#include <queue>
#include <filesystem>
using namespace std;
using json = nlohmann::json;

WebDashCore::WebDashCore() {
    _CalculateMyWorldRootDirectory();
    _InitializeLoggingFiles();

    Log(WebDash::LogType::INFO, "Determined WebDash root path: " + _myworld_root_path);
}

void WebDashCore::Create() {
    if (_config.has_value()) return;

    _config = WebDashCore();
}

WebDashCore WebDashCore::Get() {
    Create();

    return _config.value();
}

vector<pair<string, string>> WebDashCore::GetAllDefinitions(bool surpress_logging) {
    vector<pair<string, string>> ret;

    const string path = _myworld_root_path + "/definitions.json";

    ifstream configStream;
    try {
        configStream.open(path.c_str(), ifstream::in);
    } catch (...) {
        if (!surpress_logging) Log(WebDash::LogType::ERR, "Issues opening to definitions file.");
        return ret;
    }

    json _defs;
    try {
        configStream >> _defs;
    } catch (...) {
        if (!surpress_logging) Log(WebDash::LogType::ERR, "Was unable to parse the config '" + path + "' file. Format error?");
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

bool WebDashCore::_CalculateMyWorldRootDirectory() {
    char current_path[2555];
    if (!GetCurrentDir(current_path, sizeof(current_path))) {
        return false;
    }
    current_path[sizeof(current_path) - 1] = '\0';

    fs::path fs_path(current_path);

    while (true) {
        _myworld_root_path = fs_path;

        auto defs = GetAllDefinitions(true);
        for (auto def : defs) {
            if (def.first == "$#.myworld.rootDir" && def.second == "this")
                return true;
        }

        if (fs_path == fs_path.root_path())
            return false;
        
        fs_path = fs_path.parent_path();
    }
    return true;
}

void WebDashCore::_InitializeLoggingFiles() {
    Log(WebDash::LogType::ERR, "");
    Log(WebDash::LogType::INFO, "");
    Log(WebDash::LogType::WARN, "");
}

string WebDashCore::GetMyWorldRootDirectory() {
    return _myworld_root_path;
}

std::filesystem::path WebDashCore::GetPersistenteStoragePath() {
    filesystem::path ret = GetMyWorldRootDirectory();
    ret += string("/app-persistent/data/") + _WEBDASH_PROJECT_NAME_;

    Log(WebDash::LogType::INFO, "Create recursive: " + ret.string());

    std::filesystem::create_directories(ret);
    
    return ret;
}

void WebDashCore::WriteToMyStorage(const string filename, std::function<void(WriterType)> fnc) {
    bool finished = false;

    // Get full path to destination. Directory is created.
    const string persistent_file = GetPersistenteStoragePath().string() + ("/" + filename);

    ofstream out;
    out.open(persistent_file, std::ofstream::out | std::ofstream::app);

    WriterType writer = [&](WriteType type, const string data) {
        if (type == WriteType::End) {
            finished = true;
            return;
        } else if (type == WriteType::Clear) {
            out.close();
            out.open(persistent_file, std::ofstream::out);
            return;
        } else if (type == WriteType::Append) {
            out << data;
        }
    };
    
    while (!finished) {
        fnc(writer);
    }

    if (out.is_open()) {
        out.close();
    }
}

void WebDashCore::LoadFromMyStorage(const string filename, ReadType type, std::function<void(istream&)> fnc) {
    try {
        string finpath = GetPersistenteStoragePath().string() + ("/" + filename);
        
        ifstream infilestream;
        infilestream.open(finpath.c_str(), ifstream::in);
        fnc(infilestream);
    } catch (...) {
        Log(WebDash::LogType::ERR,
            "Issues opening " + filename + ". Not saved yet? Fallback to default.");

        stringstream instream;
        switch (type) {
            case ReadType::JSON:
                instream.str("{}");
                break;
            default:
                instream.str("");
                break;
        }

        fnc(instream);
        return;
    }
}

void WebDashCore::Log(WebDash::LogType type, const std::string msg, const bool append_if_possible) {
    // Get current time to add timestamp to the log entry.
    std::string curr_time = "";
    {
        auto now_c = std::chrono::system_clock::now();
        std::time_t now_t = std::chrono::system_clock::to_time_t(now_c);
        
        std::stringstream ss;
        ss << std::put_time(std::localtime(&now_t), "%F %T");
        curr_time = ss.str();
    }

    const string fpath = GetAndCreateLogDirectory() + "/logging." + WebDash::kTypeToString.at(type) + ".txt";
    const bool fexists = std::filesystem::exists(fpath.c_str()); // does the file already exist?
    
    // Program was newly started and our goal is not to append
    // OR file does not exist, then initialize empty file.
    if ((!_is_logtype_initialized[type] && !append_if_possible) || (!fexists)) {
        std::ofstream out(fpath.c_str());
        out << curr_time << ": initialized this file." << std::endl;
        out.close();
        _is_logtype_initialized[type] = true;
    }

    // Write data.
    std::ofstream out(fpath.c_str(), std::ofstream::out | std::ofstream::app);
    out << curr_time << ": " << msg << std::endl;
}

string WebDashCore::GetAndCreateLogDirectory() {
    const string myworld_path = GetMyWorldRootDirectory();
    const string finpath = myworld_path + "/app-temporary/logging/" + _WEBDASH_PROJECT_NAME_;
    std::filesystem::create_directories(finpath);
    return finpath;
}

std::optional<WebDashCore> WebDashCore::_config = nullopt;