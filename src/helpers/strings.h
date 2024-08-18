#ifndef RD_STRINGS_H
#define RD_STRINGS_H
#include <string>
#include <ctime>
#include <cstdlib>

namespace rd::strings {
    inline std::string get_uuid(int len = 6){
        // Generate a simple 6-character alphanumeric UUID
        std::string uuid;
        const std::string chars = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";
        std::srand(std::time(nullptr));
        for (int i = 0; i < len; ++i) {
            uuid += chars[rand() % chars.length()];
        }
        return uuid;
    }
}
#endif // RD_STRINGS_H
