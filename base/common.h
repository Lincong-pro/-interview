#include <thread>
#include <sstream>

inline std::string idString(std::thread::id ID) {
    std::stringstream ss;
    ss << ID;
    return ss.str();
}