#pragma once
#include <string>
#include <cstdio>

struct Song {
    std::string title;
    std::string artist;
    std::string filePath;
    int durationInSeconds;

    Song(const std::string& t = "",
        const std::string& a = "",
        const std::string& p = "",
        int d = 0)
        : title(t), artist(a), filePath(p), durationInSeconds(d) {
    }

    // H:MM:SS
    std::string getFormattedDuration() const {
        int hours = durationInSeconds / 3600;
        int minutes = (durationInSeconds % 3600) / 60;
        int seconds = durationInSeconds % 60;
        char buffer[24];
        std::snprintf(buffer, sizeof(buffer), "%d:%02d:%02d", hours, minutes, seconds);
        return std::string(buffer);
    }
};

// "C:\music\Artist - Title.mp3"  ->  title = "Title", artist = "Artist"
inline Song MakeSongFromPath(const std::string& path, int duration)
{
    size_t slash = path.find_last_of("/\\");
    std::string name = (slash == std::string::npos) ? path : path.substr(slash + 1);
    size_t dot = name.find_last_of('.');
    if (dot != std::string::npos) name = name.substr(0, dot);

    std::string artist = "Unknown";
    size_t dash = name.find(" - ");
    if (dash != std::string::npos) {
        artist = name.substr(0, dash);
        name = name.substr(dash + 3);
    }
    return Song(name, artist, path, duration);
}