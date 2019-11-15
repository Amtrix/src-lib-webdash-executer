#pragma once

#include <string>
#include <vector>
using namespace std;

namespace webdash {
    struct RunReturn {
        int return_code;
        string output;
    };

    struct RunConfig {
        bool redirect_output_to_str;
        std::function<RunReturn(string, RunConfig)> CmdResolveAndRun;
    };
}
