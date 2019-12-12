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

namespace myworld {
    namespace storage {
        enum class WriteType {
            Append,
            Clear,
            End
        };

        enum class ReadType {
            JSON,
            Text
        };
    }
}

namespace myworld::logging {
    enum class Type {
        INFO = 1,
        ERR = 2,
        WARN = 3,
        NOTIFY = 4
    };

    inline const std::map<Type, string> kTypeToString {
        { Type::INFO,   "info" },
        { Type::ERR,    "error" },
        { Type::WARN,   "warn" },
        { Type::NOTIFY, "notify"}
    };    
}

using namespace myworld::storage;
using WriterType = std::function<void(WriteType, string)>;

class WebDashCore {
    public:
        static WebDashCore Get();

        vector<pair<string, string>> GetAllDefinitions(bool surpress_logging = false);

        string GetMyWorldRootDirectory();

        std::filesystem::path GetPersistenteStoragePath();

        void WriteToMyStorage(const string filename, std::function<void(WriterType)> fnc);

        void LoadFromMyStorage(const string filename, ReadType type, std::function<void(istream&)> fnc);

        void Log(myworld::logging::Type type, const std::string msg, const bool append_if_possible = false);

        string GetAndCreateLogDirectory();
    
    private:
        WebDashCore();

        static void Create();

        bool _CalculateMyWorldRootDirectory();

        void _InitializeLoggingFiles();

        // Is log type output initialized
        std::map<myworld::logging::Type, bool> _is_logtype_initialized;

        static std::optional<WebDashCore> _config;

        string _myworld_root_path;
};

inline WebDashCore MyWorld() {
    return WebDashCore::Get();
}

namespace myworld::dashboard {
    inline void notify(const std::string msg) {
        MyWorld().Log(myworld::logging::Type::NOTIFY, msg, true);
    }
}