#pragma once

#include <webdash-log-code.hpp>

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
        NOTIFY = 4,
        DEBUG = 5
    };

    inline const std::map<LogType, string> kTypeToString {
        { WebDash::LogType::INFO,   "info"  },
        { WebDash::LogType::ERR,    "error" },
        { WebDash::LogType::WARN,   "warn"  },
        { WebDash::LogType::NOTIFY, "notify"},
        { WebDash::LogType::DEBUG,  "debug"}
    };

    struct PullProject {
        string source;
        string destination;
        string webdash_task;
        bool do_register;
    };
}

using WriterType = std::function<void(WebDash::StoreWriteType, string)>;

class WebDashCore {
    private:
        struct PrivateCtorClass {};

    public:
        // Constructor with a dummy parameter. The parameter ensures that no one but WebDashCore can use it.
        // The reason this was implemented like this instead of moving constructor to private section is due to
        // it's requirement for the std::optional::emplace function.
        WebDashCore(PrivateCtorClass private_ctor);
        // Delete copy constructor.
        WebDashCore(const WebDashCore&) = delete;

        // Returns the singleton of type WebDashCore.
        static WebDashCore& Get();

        // Creates the singleton.
        static void Create(std::optional<string> cwd = nullopt);

        vector<pair<string, string>> GetCoreDefinitions() const;

        // Returns all definitions from within GetMyWorldRootDirectory()/definitions.json
        // Format of each element in result vector: {.first = $#.A.B.C.D.E, .second = value)
        //
        // Arguments:
        //     file_must_exist - Return empty vector if file does not exist in current root or is not JSON.
        //         This is useful to probe the current root directory.
        vector<pair<string, string>> GetCustomDefinitions(bool file_must_exist = true);

        // Returns the webdash root directory.
        string GetMyWorldRootDirectory();

        // Returns the persistent storage path the including app can use.
        std::filesystem::path GetPersistenteStoragePath();

        // Provide a function to the caller to write to file <filename>.
        void WriteToMyStorage(const string filename, std::function<void(WriterType)> fnc);

        // Provide a function to the caller to read from file <filename>.
        void LoadFromMyStorage(const string filename, WebDash::StoreReadType type, std::function<void(istream&)> fnc);

        // Logs into GetAndCreateLogDirectory()/app-temporary/logging/_WEBDASH_PROJECT_NAME_;
        void Log(WebDash::LogType type,
                 const std::string msg,
                 const LogCode logcode = LogCode::E_UNKNOWN,
                 const bool append_if_possible = false);

        void Notify(const std::string msg, const LogCode logcode = LogCode::N_UNKNOWN);

        // Return path of logging directory and create if not exists:
        // GetAndCreateLogDirectory()/app-temporary/logging/_WEBDASH_PROJECT_NAME_
        string GetAndCreateLogDirectory();

        // Set working directory.
        void SetCwd(std::optional<string> cwd);

        vector<string> GetPathAdditions();

        vector<pair<string, string>> GetEnvAdditions();

        vector<WebDash::PullProject> GetExternalProjects();
    
    private:
    
        bool _CalculateMyWorldRootDirectory();

        void _InitializeLoggingFiles();

        // Is log type output initialized
        std::map<WebDash::LogType, bool> _is_logtype_initialized;

        static std::optional<WebDashCore> _config;

        string _myworld_root_path;

        std::optional<string> _preset_cwd;

        static bool _creation_is_active;
};

inline WebDashCore& MyWorld() {
    return WebDashCore::Get();
}

//
// Handy routines meant to provide shortcuts to MyWorld() calls. These should
// reduce the direct usage of the WebDashCore() object across the codebase.
//

namespace myworld {
    inline void notify(const std::string msg, const LogCode logcode = LogCode::N_UNKNOWN) {
        MyWorld().Log(WebDash::LogType::NOTIFY, msg, logcode, true);
    }
}