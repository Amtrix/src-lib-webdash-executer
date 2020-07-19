#include "webdash-core.hpp"

#include <nlohmann/json.hpp>
#include <queue>
#include <filesystem>
#include <iostream>
#include <fstream>
#include <sstream>
using namespace std;
using json = nlohmann::json;

WebDashCore::WebDashCore() {
    if (!_CalculateMyWorldRootDirectory()) {
        cout << "Could not find WebDash root directory." << endl;
        return;
    }
    _InitializeLoggingFiles();

    Log(WebDash::LogType::DEBUG, "Determined WebDash root path: " + _myworld_root_path);
}

void WebDashCore::Create(std::optional<string> cwd) {
    if (_config.has_value()) {
        _config->Log(WebDash::LogType::WARN, "Create has been called before.");
        return;
    }

    _config = WebDashCore();
    _config->SetCwd(cwd);
}

WebDashCore WebDashCore::Get() {
    if (!_config.has_value()) {
        Create();

        if (_config.has_value())
            _config->Log(WebDash::LogType::WARN, "Default creation done for WebDashCore");
        else
            cout << "Default creation for WebDashCore has failed" << endl;
    }

    return _config.value();
}

string BasicJsonToString(json val) {
    if (val.is_string()) return val.get<std::string>();
    if (val.is_boolean()) return val.get<bool>() ? "true" : "false";
    if (val.is_number_integer()) return to_string(val.get<int>());
    if (val.is_number()) return to_string(val.get<double>());
    return "-";
}

vector<pair<string, string>> WebDashCore::GetAllDefinitions(bool surpress_logging) {
    vector<pair<string, string>> ret;

    const string path = _myworld_root_path + "/definitions.json";

    ifstream configStream;
    try {
        configStream.open(path.c_str(), ifstream::in);
    } catch (...) {
        if (!surpress_logging) Log(WebDash::LogType::ERR, "Issues opening the definitions file.");
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

        if (u.second.is_array()) {
            int dx = 0;
            for (auto elem : u.second) {
                const string nkey = u.first + ".[" + to_string(dx) + "]";
                if (elem.is_object() || elem.is_array())
                    Q.push(make_pair(nkey, elem));
                else
                    ret.push_back(make_pair("$#" + nkey, BasicJsonToString(elem)));
                dx++;
            }
            continue;
        }

        for (auto& [key, value] : u.second.items()) {
            if (value.is_object() || value.is_array()) {
                Q.push(make_pair(u.first + "." + key, value));
            } else {
                ret.push_back(make_pair(
                    "$#" + u.first + "." + key,
                    BasicJsonToString(value)
                ));
            }
        }
    }

    return ret;
}

inline std::optional<string> GetRepositoryRoot() {
    std::optional<string> myworld_path = nullopt;
    char* myworld_path_c = nullptr;
#ifdef _MSC_VER
    size_t myworld_path_len = 0;
    if (_dupenv_s(&myworld_path_c, &myworld_path_len, "MYWORLD") == 0 && myworld_path_c != nullptr)
#else
    myworld_path_c = getenv("MYWORLD");
    if (myworld_path_c != NULL)
#endif
    {
        myworld_path = myworld_path_c;
    }
    return myworld_path;
}

bool WebDashCore::_CalculateMyWorldRootDirectory() {
    // Get the working directory.
    filesystem::path fs_path = filesystem::current_path();
    if (_preset_cwd.has_value())
        fs_path = _preset_cwd.value();

    while (true) {
        _myworld_root_path = fs_path.string();

        auto defs = GetAllDefinitions(true);
        for (auto def : defs) {
            if (def.first == "$#.myworld.rootDir" && def.second == "this")
                return true;
        }

        if (fs_path == fs_path.root_path())
            break;
        
        fs_path = fs_path.parent_path();
    }

    // Still none found, check environment path
    auto myworld_env = GetRepositoryRoot();

    if (myworld_env.has_value()) {
        _myworld_root_path = myworld_env.value();
        return true;
    }

    return false;
}

void WebDashCore::_InitializeLoggingFiles() {
    Log(WebDash::LogType::ERR, "");
    Log(WebDash::LogType::INFO, "");
    Log(WebDash::LogType::WARN, "");
    Log(WebDash::LogType::DEBUG, "");
}

string WebDashCore::GetMyWorldRootDirectory() {
    return _myworld_root_path;
}

std::filesystem::path WebDashCore::GetPersistenteStoragePath() {
    filesystem::path ret = GetMyWorldRootDirectory();
    ret += string("/app-persistent/data/") + _WEBDASH_PROJECT_NAME_;

    Log(WebDash::LogType::DEBUG, "Create recursive: " + ret.string());

    std::filesystem::create_directories(ret);
    
    return ret;
}

void WebDashCore::WriteToMyStorage(const string filename, std::function<void(WriterType)> fnc) {
    bool finished = false;

    // Get full path to destination. Directory is created.
    const string persistent_file = GetPersistenteStoragePath().string() + ("/" + filename);

    ofstream out;
    out.open(persistent_file, std::ofstream::out | std::ofstream::app);

    WriterType writer = [&](WebDash::StoreWriteType type, const string data) {
        if (type == WebDash::StoreWriteType::End) {
            finished = true;
            return;
        } else if (type == WebDash::StoreWriteType::Clear) {
            out.close();
            out.open(persistent_file, std::ofstream::out);
            return;
        } else if (type == WebDash::StoreWriteType::Append) {
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

void WebDashCore::LoadFromMyStorage(const string filename, WebDash::StoreReadType type, std::function<void(istream&)> fnc) {
    string finpath = GetPersistenteStoragePath().string() + ("/" + filename);
    
    try {
        ifstream infilestream;
        infilestream.open(finpath.c_str(), ifstream::in);
        fnc(infilestream);
    } catch (...) {
        Log(WebDash::LogType::ERR, "Issues opening " + finpath + ". Not saved yet? Fallback to default.");

        stringstream instream;
        switch (type) {
            case WebDash::StoreReadType::JSON:
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
static string StringifyLogCode(LogCode err) {
    std::stringstream stream;
    stream << std::hex << (static_cast<int>(err));
    std::string result( stream.str() );
    return result;
}

void WebDashCore::Log(WebDash::LogType type, const std::string msg, const LogCode errcode, const bool append_if_possible) {
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
        out << StringifyLogCode(LogCode::N_INIT_LOG_FILE) << " " << curr_time << ": initialized this file." << std::endl;
        out.close();
        _is_logtype_initialized[type] = true;
    }

    // Write data.
    std::ofstream out(fpath.c_str(), std::ofstream::out | std::ofstream::app);
    out << StringifyLogCode(errcode) << " " << curr_time << ": " << msg << std::endl;
}

void WebDashCore::Notify(const std::string msg, const LogCode logcode) {
    Log(WebDash::LogType::NOTIFY, msg, logcode, true);
}

string WebDashCore::GetAndCreateLogDirectory() {
    const string myworld_path = GetMyWorldRootDirectory();
    const string finpath = myworld_path + "/app-temporary/logging/" + _WEBDASH_PROJECT_NAME_;
    std::filesystem::create_directories(finpath);
    return finpath;
}

void WebDashCore::SetCwd(std::optional<string> cwd) {
    _preset_cwd = cwd;
}

namespace {
    void ParseJsonConcats(vector<pair<string, string>> defs, std::function<void(vector<string>, string)> func) {
        for (std::pair<string, string> entry : defs) {
            istringstream iss(entry.first);

            std::vector<std::string> tokens;
            std::string token;
            while (std::getline(iss, token, '.')) {
                if (!token.empty())
                    tokens.push_back(token);
            }

            func(tokens, entry.second);
        }
    }

    string SubstituteKeywords(string src, string keyword, string replace_with) {
        size_t pos;

        while ((pos = src.find(keyword)) != string::npos) {
            src.replace(pos, keyword.size(), replace_with);
        }

        return src;
    }

    string NormalizeKeyw(string src) {
        vector<pair<string, string>> subs;
        subs.push_back(make_pair("$.rootDir()", WebDashCore::Get().GetMyWorldRootDirectory()));

        // Substitue keywords.
        for (auto& [key, value] : subs) {
            src = SubstituteKeywords(src, key, value);
        }

        return src;
    }
}

vector<string> WebDashCore::GetPathAdditions() {
    vector<string> ret;

    auto defs = GetAllDefinitions(true);
    ParseJsonConcats(defs, [&](vector<string> tokens, string val) {
        if (tokens.size() == 3 && tokens[1] == "path-add") {
            ret.push_back(NormalizeKeyw(val));
        }
    });

    return ret;
}

vector<pair<string, string>> WebDashCore::GetEnvAdditions() {
    vector<pair<string, string>> ret;

    auto defs = GetAllDefinitions(true);
    ParseJsonConcats(defs, [&](vector<string> tokens, string val) {
        if (tokens.size() >= 3 && tokens[1] == "env") {
            string key = "";
            for (size_t i = 2; i < tokens.size(); ++i) {
                key += tokens[i];
                if (i + 1 < tokens.size())
                    key += ".";
            }
            ret.push_back(make_pair(key, NormalizeKeyw(val)));
        }
    });

    return ret;
}

vector<WebDash::PullProject> WebDashCore::GetExternalProjects() {
    unordered_map<string, WebDash::PullProject> projects;
 
    auto defs = GetAllDefinitions(true);
    ParseJsonConcats(defs, [&](vector<string> tokens, string val) {
        // looking for $#.pull-projects.{source, destination, exec}

        if (tokens.size() == 4 && tokens[1] == "pull-projects") {
            const string pkey = tokens[2];
            const string property = tokens[3];

            if (property == "source")
                projects[pkey].source = NormalizeKeyw(val);
            if (property == "destination")
                projects[pkey].destination = NormalizeKeyw(val);
            if (property == "exec")
                projects[pkey].webdash_task = NormalizeKeyw(val);
            if (property == "register")
                projects[pkey].do_register = val == "true";
        }
    });

    vector<WebDash::PullProject> ret;
    for (std::pair<string, WebDash::PullProject> element : projects) {
        ret.push_back(element.second);
    }

    return ret;
}

std::optional<WebDashCore> WebDashCore::_config = nullopt;