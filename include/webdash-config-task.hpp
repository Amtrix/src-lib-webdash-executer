#pragma once

#include <chrono>

#include <nlohmann/json.hpp>

#include "webdash-config-task.hpp"
#include "webdash-types.hpp"

class WebDashConfig;

using json = nlohmann::json;
using namespace std::chrono;

/**
 * 
 * Representative of a single webdash task that's from within a config file.
 *
 * */
class WebDashConfigTask {
    public:
        WebDashConfigTask(WebDashConfig*, string, json);

        bool ShouldExecuteTimewise(webdash::RunConfig config);

        webdash::RunReturn Run(webdash::RunConfig config, std::string action);

        webdash::RunReturn Run(webdash::RunConfig config = {});

        string GetName() { return _name; }

        bool IsValid() { return _is_valid; }
    private:

        string _taskid;
        std::optional<string> _frequency;
        vector<string> _actions;
        vector<string> _dependencies;
        string _name;
        std::optional<string> _wdir;

        // Per default, ::time_point is initialized to epoch.
        std::chrono::high_resolution_clock::time_point _last_exec_time;

        // Exactly that. Counts the number of times ::Run() was called.
        int _times_called = 0;

        // We want to print once if a task execution was skipped. We use this flag to
        // skip such further logging.
        bool _print_skip_has_happened = false;

        // The task might be invalid due to some parameters wrongly set in the JSON.
        // Not restricted to this example only.
        bool _is_valid = true;

        bool _notify_dashboard = false;

        string _when_to_execute;

        string _config_path;
};