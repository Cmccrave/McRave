#pragma once
#include <string>

struct Time {
    int minutes;
    int seconds;
    int frames;
    Time(int f)
    {
        frames  = f;
        minutes = int(double(f) / 1428.6);
        seconds = int(double(f) / 23.81) % 60;
    }
    Time(int m, int s)
    {
        minutes = m, seconds = s;
        frames = int(23.81 * ((double(m) * 60.0) + double(s)));
    }
    Time()
    {
        minutes = 999;
        seconds = 0;
    }
    std::string toString() { return std::to_string(minutes) + ":" + (seconds < 10 ? "0" + std::to_string(seconds) : std::to_string(seconds)); }

    bool isUnknown() { return minutes >= 999; }

    bool operator<(const Time t2) { return (minutes < t2.minutes) || (minutes == t2.minutes && seconds < t2.seconds); }

    bool operator<=(const Time t2) { return (minutes < t2.minutes) || (minutes == t2.minutes && seconds <= t2.seconds); }

    const bool operator>(const Time t2) const { return (minutes > t2.minutes) || (minutes == t2.minutes && seconds > t2.seconds); }

    bool operator>=(const Time t2) { return (minutes > t2.minutes) || (minutes == t2.minutes && seconds >= t2.seconds); }

    bool operator!=(const Time t2) { return minutes != t2.minutes || seconds != t2.seconds; }

    bool operator==(const Time t2) { return minutes == t2.minutes && seconds == t2.seconds; }

    Time operator-(const Time t2)
    {
        auto op = (minutes * 60 + seconds) - (t2.minutes * 60 + t2.seconds);
        return Time(op / 60, op % 60);
    }

    Time operator+(const Time t2)
    {
        auto op = (minutes * 60 + seconds) + (t2.minutes * 60 + t2.seconds);
        return Time(op / 60, op % 60);
    }
};