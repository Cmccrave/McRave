#pragma once
#include <BWAPI.h>

#include <ostream>

namespace BWEB::Logger {

    inline std::ofstream &getLogStream()
    {
        static std::ofstream instance("bwapi-data/write/bweb_logger.txt");
        return instance;
    }

    template <typename... Args> //
    void writeToLogger(const char *file, int line, Args &&... args)
    {
        std::ofstream &writeFile = getLogStream();
        std::ostringstream ss;

        auto bracketWrap = [](const auto &x) -> std::string {
            std::ostringstream temp;
            temp << "[" << x << "]";
            return temp.str();
        };

        auto seconds = int(double(BWAPI::Broodwar->getFrameCount()) / 23.81) % 60;
        auto minutes = int(double(BWAPI::Broodwar->getFrameCount()) / 23.81) / 60;
        auto time    = std::to_string(minutes) + ":" + (seconds < 10 ? "0" + std::to_string(seconds) : std::to_string(seconds));

        ss << time << bracketWrap(BWAPI::Broodwar->getFrameCount());
        ss << bracketWrap(std::string(file) + ":" + std::to_string(line)) << " ";
        (ss << ... << args);

        writeFile << ss.str() << "\n";
    }

    constexpr const char *getFileName(const char *path)
    {
        const char *file = path;
        while (*path) {
            if (*path == '/' || *path == '\\')
                file = path + 1;
            ++path;
        }
        return file;
    }
} // namespace BWEB::Logger

#define BWEB_LOG(...)                                                                                                                                                                                  \
    do {                                                                                                                                                                                               \
        Logger::writeToLogger(Logger::getFileName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                                   \
    } while (0)

#define BWEB_LOG_ONCE(...)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        static bool _logged = false;                                                                                                                                                                   \
        if (!_logged) {                                                                                                                                                                                \
            Logger::writeToLogger(Logger::getFileName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                               \
            _logged = true;                                                                                                                                                                            \
        }                                                                                                                                                                                              \
    } while (0)

#define BWEB_LOG_FAST(...)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        static auto lastLogFrame = BWAPI::Broodwar->getFrameCount() - 24;                                                                                                                              \
        if (BWAPI::Broodwar->getFrameCount() - lastLogFrame >= 24) {                                                                                                                                   \
            Logger::writeToLogger(Logger::getFileName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                               \
            lastLogFrame = BWAPI::Broodwar->getFrameCount();                                                                                                                                           \
        }                                                                                                                                                                                              \
    } while (0)

#define BWEB_LOG_SLOW(...)                                                                                                                                                                             \
    do {                                                                                                                                                                                               \
        static auto lastLogFrame = BWAPI::Broodwar->getFrameCount() - 120;                                                                                                                             \
        if (BWAPI::Broodwar->getFrameCount() - lastLogFrame >= 120) {                                                                                                                                  \
            Logger::writeToLogger(Logger::getFileName(__FILE__), __LINE__, __VA_ARGS__);                                                                                                               \
            lastLogFrame = BWAPI::Broodwar->getFrameCount();                                                                                                                                           \
        }                                                                                                                                                                                              \
    } while (0)