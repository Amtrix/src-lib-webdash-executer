#include "webdash-config.hpp"
#include "webdash-types.hpp"

#include <optional>
using namespace myworld::logging;

WebDashConfig::WebDashConfig(string path) {
    _path = path;

    ifstream configStream;
    try {
        Log(Type::INFO, "UXPath: " + _path);
        configStream.open(_path.c_str(), ifstream::in);
    } catch (...) {
        Log(Type::ERR, "Issues opening to config file. Something wrong with path?");
        return;
    }
    
    try {
        configStream >> _config;
    } catch (...) {
        Log(Type::ERR, "Was unable to parse the config '" + path + "' file. Format error?");
        return;
    }

    json cmds;
    try {
        cmds = _config["commands"];
    } catch (...) {
        Log(Type::ERR, "No 'commands' given.");
    }

    Log(Type::INFO, "Commands loaded. Available count: " + to_string(cmds.size()));
    
    int cmd_dx = 0;
    for (auto cmd : cmds) {
        Log(Type::INFO, to_string(cmd_dx) + "th command: " + cmd.dump());
        
        try {
            const string cmdid = path + "#" + cmd["name"].get<std::string>();
            tasks.emplace_back(path, cmdid, cmd);
        } catch (...) {
            Log(Type::INFO, "Failed getting name from " + to_string(cmd_dx) + "th command.");
        }

        cmd_dx++;
    }

    _is_initialized = true;
}

string WebDashConfig::GetPath() const {
    return _path;
}

void WebDashConfig::Serialize(WriterType writer) {
    // use writer to write to file.
    writer(myworld::storage::WriteType::Append, _path);
}

vector<string> WebDashConfig::GetTaskList() {
    vector<string> ret;

    for (auto task : tasks) {
        ret.push_back(task.GetName());
    }

    return ret;
}

std::optional<WebDashConfigTask> WebDashConfig::GetTask(const string cmdname) {
    for (auto task : tasks)
        if (task.GetName() == cmdname)
            return task;

    return nullopt;
}

std::vector<webdash::RunReturn> WebDashConfig::Run(const string cmdName, webdash::RunConfig runconfig) {
    std::vector<webdash::RunReturn> ret;

    // Enables tasks to resolve task-wide tasks.
    // Meaning, one can specify ":<task_name>" as an action.
    //                          "$.thisDir()/path-relative-to-dir-of-current-config/webdash.config.json:blabla"
    //                          "./path-relative-to-myworld/x/y/z/webdash.config.json:blabla"
    runconfig.TaskRetriever = [&](string cmdid) -> optional<WebDashConfigTask> {
        cout << "Resolving dependency: " << cmdid << endl;

        if (cmdid[0] == ':') {
            return GetTask(cmdid.substr(1));
        } else {
            if (cmdid.find(":") == string::npos)
                return nullopt;

            std::string configpath = cmdid.substr(0, cmdid.find(":"));
            std::string real_cmd_name = cmdid.substr(configpath.length() + 1);

            std::filesystem::path path(configpath);
            WebDashConfig other(((path.is_absolute() == false) ? myworld::utility::GetRepositoryRoot() + "/" : "") + configpath);

            if (!other.IsInitialized())
                return nullopt;

            return other.GetTask(real_cmd_name);
        }

        return nullopt;
    };

    for (WebDashConfigTask& task : tasks) {
        if (task.IsValid() == false)
            continue;
            
        if (task.GetName() == cmdName) {
            ret.push_back(task.Run(runconfig));
        } else if (cmdName == "") {
            ret.push_back(task.Run(runconfig));
        }
    }

    return ret;
}