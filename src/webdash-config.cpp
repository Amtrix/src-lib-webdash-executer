#include "webdash-config.hpp"
#include "webdash-types.hpp"
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
        Log(Type::ERR, "Was unable to parse the config file. Format error?");
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

std::vector<webdash::RunReturn> WebDashConfig::Run(const string cmdName, webdash::RunConfig runconfig) {
    std::vector<webdash::RunReturn> ret;

    // Enables tasks to resolve task-wide tasks.
    // Meaning, one can specify "#<task_name>" as an action.
    runconfig.CmdResolveAndRun = [&](string cmdid, webdash::RunConfig config) {
        webdash::RunReturn ret;

        if (cmdid[0] == ':') {
            for (WebDashConfigTask& task : tasks)
                if (task.GetName() == cmdid)
                    return task.Run(config);
            
            Log(Type::ERR, "No " + cmdid + " within " + GetPath());

            return ret;
        } else {
            std::string configpath = cmdid.substr(0, cmdid.find(":")); // token is "scott
            std::string real_cmd_name = cmdid.substr(configpath.length() + 1);

            cout << "Resolving dependency: " << myworld::utility::GetRepositoryRoot() + cmdid << endl;
            WebDashConfig other(myworld::utility::GetRepositoryRoot() + configpath);

            webdash::RunConfig other_config;
            other_config.redirect_output_to_str = config.redirect_output_to_str;

            Log(Type::INFO, "Cross-config: " + GetPath() + " => " + configpath);
            return other.Run(real_cmd_name, other_config)[0];
        }
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