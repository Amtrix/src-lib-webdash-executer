#include <iostream>
using namespace std;

enum class LogCode : unsigned int {
    E_OK = 0xE0000000,
    E_UNKNOWN,
    E_INIT_LOG_FILE,

    N_UNKNOWN = 0xA0000000
};