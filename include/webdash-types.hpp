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
        bool run_only_with_frequency = false;
        bool redirect_output_to_str = false;
        std::function<RunReturn(string, RunConfig)> CmdResolveAndRun;
    };
}
