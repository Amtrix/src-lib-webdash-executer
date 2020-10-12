#pragma once

#include "webdash-config-task.hpp"
#include "webdash-core.hpp"

#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class WebDashConfig {
    public:
        WebDashConfig(string path);

        // Runs a single task with name {cmdName} or all if none provided or "" is provided.
        std::vector<webdash::RunReturn> Run(const string cmdName = "", webdash::RunConfig runconfig = {});

        std::vector<std::pair<string,string>> GetAllDefinitions() const;

        void Reload();

        string GetPath() const;

        void Serialize(WriterType writer);

        bool IsLoaded() const { return _is_loaded; };
        
        vector<string> GetTaskList();

        std::optional<WebDashConfigTask> GetTask(const string cmdname);
    private:

        // Loads the config. Returns false iff failure detected.
        bool Load();

        json _config;

        vector<WebDashConfigTask> tasks;

        string _path;

        bool _is_loaded;
};