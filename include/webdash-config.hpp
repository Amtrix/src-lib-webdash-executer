#pragma once

#include "webdash-config-task.hpp"
#include "webdash-core.hpp"

#include <nlohmann/json.hpp>

using namespace std;
using json = nlohmann::json;

class WebDashConfig {
    public:
        WebDashConfig(string path);

        /**
         * Runs a single task with name {cmdName} or all if none provided or "" is provided.
         * **/
        std::vector<webdash::RunReturn> Run(const string cmdName = "", webdash::RunConfig runconfig = {});

        string GetPath() const;

        void Serialize(WriterType writer);

        bool IsInitialized() const { return _is_initialized; };
        
        vector<string> GetTaskList();

        std::optional<WebDashConfigTask> GetTask(const string cmdname);
    private:
        json _config;

        vector<WebDashConfigTask> tasks;

        string _path;

        bool _is_initialized;
};