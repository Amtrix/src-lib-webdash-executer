#include <string>
#include <optional>
#include <vector>
using namespace std;

class WebDashGlobalConfig {
    public:
        static WebDashGlobalConfig GetConfig();

        vector<pair<string, string>> GetAllDefinitions();
    
    private:
        static void Create();

        static std::optional<WebDashGlobalConfig> _config;
};