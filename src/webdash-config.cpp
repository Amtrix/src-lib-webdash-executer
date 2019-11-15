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

    runconfig.CmdResolveAndRun = [&](string cmdid, webdash::RunConfig config) {
        webdash::RunReturn ret;

        for (WebDashConfigTask& task : tasks)
            if (task.GetName() == cmdid)
                return task.Run(config);
        
        Log(Type::ERR, "No " + cmdid + " within " + GetPath());

        return ret;
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