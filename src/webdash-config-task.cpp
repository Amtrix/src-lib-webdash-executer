#include "webdash-config-task.hpp"

#include <myworldcpp/paths.hpp>

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <errno.h>
#include <sstream>

WebDashConfigTask::WebDashConfigTask(string config_path, string taskid, json task_config) {
    myworld::logging::Log(myworld::logging::Type::INFO, "Loading Task: " + taskid);

    this->_taskid = taskid;

    try {
        const string name = task_config["name"].get<std::string>();
        this->_name = name;
    } catch (...) {
        myworld::logging::Log(myworld::logging::Type::ERR, "T| " + taskid + ": no name.");
        _is_valid = false;
    }

    {
        bool has_action = false;
        try {
            const string action = task_config["action"].get<std::string>();
            this->_actions.push_back(action);
            has_action = true;
        } catch (...) {
        }

        try {
            json actions = task_config["actions"];

            for (auto action : actions) 
                this->_actions.push_back(action.get<std::string>());
            has_action = true;
        } catch (...) {
        }

        if (!has_action) {
            _is_valid = false;
            myworld::logging::Log(myworld::logging::Type::ERR, "T| " + taskid + ": no actions.");
        }
    }

    try {
        json dependencies = task_config["dependencies"];

        for (auto dependency : dependencies) 
            this->_dependencies.push_back(dependency.get<std::string>());
    } catch (...) {
        myworld::logging::Log(myworld::logging::Type::WARN, "T| " + taskid + ": no dependencies.");
    }

    
    try {
        const string frequency = task_config["frequency"].get<std::string>();
        this->_frequency = frequency;
    } catch (...) {
        myworld::logging::Log(myworld::logging::Type::WARN, "T| " + taskid + ": no frequency.");
    }

    try {
        const string wdir = task_config["wdir"].get<std::string>();
        this->_wdir = wdir;
    } catch (...) {
        myworld::logging::Log(myworld::logging::Type::WARN, "T| " + taskid + ": no working directory (wdir) given.");
    }

    try {
        const bool val = task_config["notify-dashboard"].get<bool>();
        this->_notify_dashboard = val;
    } catch (...) {
        myworld::logging::Log(myworld::logging::Type::WARN, "T| " + taskid + ": dashboard notification not specified.");
    }

    size_t pos;
    string pattern = "$.thisDir()";
    while ((pos = _name.find(pattern)) != string::npos) {
        _name.replace(pos, pattern.size(), myworld::paths::GetDirectoryOnlyOf(config_path));
    }

    for (auto& action : _actions) {
        while ((pos = action.find(pattern)) != string::npos) {
            action.replace(pos, pattern.size(), myworld::paths::GetDirectoryOnlyOf(config_path));
        }
    }

    if (_wdir.has_value()) {
        string wdir = _wdir.value();
        while ((pos = wdir.find(pattern)) != string::npos) {
            wdir.replace(pos, pattern.size(), myworld::paths::GetDirectoryOnlyOf(config_path));
        }
        _wdir = wdir;
    }
}

bool is_number(const std::string& s) {
    char* end = 0;
    double val = strtod(s.c_str(), &end);
    return end != s.c_str() && *end == '\0' && val != HUGE_VAL;
}

// wsl.exe -- source ~/.profile && webdash install
webdash::RunReturn WebDashConfigTask::Run(webdash::RunConfig config, std::string action) {
    webdash::RunReturn retval;
    _times_called++;

    {
        auto diff = std::chrono::high_resolution_clock::now() - _last_exec_time;
        auto diff_h = duration_cast<hours>(diff).count();
        auto diff_ms = duration_cast<milliseconds>(diff).count();

        bool enough_time_passed = true;
        if (_frequency.has_value()) {
            string freqv = _frequency.value();
            if (diff_h < 24 && freqv == "daily") {
                enough_time_passed = false;
            } else if (diff_ms && is_number(freqv) && (diff_ms < stod(freqv))) {
                enough_time_passed = false;
            }
        }

        if (enough_time_passed == false) {
            if (_print_skip_has_happened == false) {
                myworld::logging::Log(myworld::logging::Type::INFO, "Skipping: " + this->_taskid);
                myworld::logging::Log(myworld::logging::Type::INFO, "Was executed " + to_string(diff_ms) + " milliseconds ago.");
                myworld::logging::Log(myworld::logging::Type::INFO, "....ommitting further similar reports until next execution passed.");
                _print_skip_has_happened = true;
            }
            return retval;
        }
        
        _print_skip_has_happened = false;
        _last_exec_time = std::chrono::high_resolution_clock::now();
    }

    myworld::logging::Log(myworld::logging::Type::INFO, "Executing: " + this->_taskid);
    myworld::logging::Log(myworld::logging::Type::INFO, "    => " + action);

    cout << "Forking... " << endl;

    int filedes[2];
    // We create a pipe to be shared with two processes.
    if (pipe(filedes) == -1) {
        perror("pipe");
        exit(1);
    }

    // Close pipe automatically after child done calling exec.
    if (fcntl(filedes[0], F_SETFD, FD_CLOEXEC) == -1) {
        perror("fcntl");
        exit(1);
    }

    pid_t pid = fork();
    if (pid == 0) {
        if (_wdir.has_value()) {
            if (chdir(_wdir.value().c_str()) != 0) {
                myworld::logging::Log(myworld::logging::Type::ERR, "Working directory not set to: " + _wdir.value());
            }
            myworld::logging::Log(myworld::logging::Type::INFO, "Working directory set to: " + _wdir.value());
        }

        std::istringstream iss(action.c_str());
        std::vector<std::string> execParts(std::istream_iterator<std::string>{iss},
                                           std::istream_iterator<std::string>());

        const char **paramList = new const char*[execParts.size() + 1];
        for (unsigned int i = 0; i < execParts.size(); ++i)
            paramList[i] = execParts[i].c_str();
        paramList[execParts.size()] = NULL;

        // child
        cout << "Call `" << paramList[0];
        for (unsigned int i = 1; i < execParts.size(); ++i) {
            cout << " " << execParts[i];
        }
        cout << "`" << endl;
        cout << "-----------------" << endl;

        {
            // If we are to redirect output to a string, create a copy of filedes[1] to STDOUT_FILENO (standard output).
            if (config.redirect_output_to_str) {
                while ((dup2(filedes[1], STDOUT_FILENO) == -1) && (errno == EINTR)) {}
                while ((dup2(filedes[1], STDERR_FILENO) == -1) && (errno == EINTR)) {}
            }

            // Close now children copies. We don't need them. We only used them to share our standard output with filedes[1] (which parent still utilizes).
            close(filedes[1]);
            close(filedes[0]);
        }

        if (execvp(paramList[0], (char**)paramList) < 0) {
            perror ("execvp");
        }

        exit(1);
    } else if (pid > 0) {
        close(filedes[1]);

        // parent
        if (config.redirect_output_to_str) {
            retval.output = "";
            while (1) {
                char buffer[43];
                int len = read(filedes[0], buffer, 42);
                if ( len < 0 ) {
                    if (errno == EINTR) {
                        continue;
                    } else {
                        perror("read");
                        exit(1);
                    }
                } else if ( len == 0 ) {
                    break;
                } else {
                    std::string data(buffer, len);
                    retval.output += data;
                }
            }
        }
    }

    int status;
    pid_t wpid = waitpid(pid, &status, 0); // wait for child to finish before exiting
    
    retval.return_code = wpid == pid && WIFEXITED(status) ? WEXITSTATUS(status) : -1;
    return retval;
}


webdash::RunReturn WebDashConfigTask::Run(webdash::RunConfig config) {
    webdash::RunReturn ret;

    if (_notify_dashboard) {
        myworld::dashboard::notify(_taskid);
    }

    for (int i = 0; i < (int)_dependencies.size(); ++i) {
        config.CmdResolveAndRun(_dependencies[i], config);
    }

    for (int i = 0; i < (int)_actions.size(); ++i) {
        string action = _actions[i];

        if (action[0] == ':') {
            auto ret_sub = config.CmdResolveAndRun(action, config);
            ret.output += ret_sub.output;
            ret.return_code |= ret_sub.return_code;
        } else {
            auto ret_sub = Run(config, action);
            ret.output += ret_sub.output;
            ret.return_code |= ret_sub.return_code;
        }
    }

    return ret;
}