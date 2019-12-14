#pragma once

#include <string>
#include <optional>
#include <vector>
#include <functional>
#include <filesystem>
#include <map>
using namespace std;

// Must be specified by consuming libraries.
extern const string _WEBDASH_PROJECT_NAME_;

namespace WebDash {
    enum class StoreWriteType {
        Append,
        Clear,
        End
    };

    enum class StoreReadType {
        JSON,
        Text
    };
    
    enum class LogType {
        INFO = 1,
        ERR = 2,
        WARN = 3,
        NOTIFY = 4
    };

    inline const std::map<LogType, string> kTypeToString {
        { WebDash::LogType::INFO,   "info" },
        { WebDash::LogType::ERR,    "error" },
        { WebDash::LogType::WARN,   "warn" },
        { WebDash::LogType::NOTIFY, "notify"}
    };    
}

using WriterType = std::function<void(WebDash::StoreWriteType, string)>;

class WebDashCore {
    public:
        // Returns the singleton of type WebDashCore.
        static WebDashCore Get();

        // Creates the singleton.
        static void Create(std::optional<string> cwd = nullopt);

        // Returns all definitions from within GetMyWorldRootDirectory()/definitions.json
        // Format: ($#.A.B.C.D.E, value)
        vector<pair<string, string>> GetAllDefinitions(bool surpress_logging = false);

        // Returns the webdash root directory.
        string GetMyWorldRootDirectory();

        // Returns the persistent storage path the including app can use.
        std::filesystem::path GetPersistenteStoragePath();

        // Provide a function to the caller to write to file <filename>.
        void WriteToMyStorage(const string filename, std::function<void(WriterType)> fnc);

        // Provide a function to the caller to read from file <filename>.
        void LoadFromMyStorage(const string filename, WebDash::StoreReadType type, std::function<void(istream&)> fnc);

        // Logs into GetAndCreateLogDirectory()/app-temporary/logging/_WEBDASH_PROJECT_NAME_;
        void Log(WebDash::LogType type, const std::string msg, const bool append_if_possible = false);

        // Return path of logging directroy and create if not exists:
        // GetAndCreateLogDirectory()/app-temporary/logging/_WEBDASH_PROJECT_NAME_
        string GetAndCreateLogDirectory();

        // Set working directory.
        void SetCwd(std::optional<string> cwd);
    
    private:
        WebDashCore();

        bool _CalculateMyWorldRootDirectory();

        void _InitializeLoggingFiles();

        // Is log type output initialized
        std::map<WebDash::LogType, bool> _is_logtype_initialized;

        static std::optional<WebDashCore> _config;

        string _myworld_root_path;

        std::optional<string> _preset_cwd;
};

inline WebDashCore MyWorld() {
    return WebDashCore::Get();
}

namespace myworld::dashboard {
    inline void notify(const std::string msg) {
        MyWorld().Log(WebDash::LogType::NOTIFY, msg, true);
    }
}