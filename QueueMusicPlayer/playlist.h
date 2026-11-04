#pragma once
#include <algorithm>
#include <cstdio>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <queue>
#include <sstream>
#include <string>
#include <vector>
#include "song.h"

class Playlist {
    std::queue<Song> songQueue;        // الطابور: المقدمة = الأغنية التالية
    Song nowPlaying;
    bool hasNowPlaying = false;
    int  totalSeconds = 0;            // مجموع مدد كل الأغاني (الحالية + الطابور)

    static Song noSong() { return Song("none", "none", "", 0); }

public:
    // 1. Add Song - O(1)
    void addSong(const Song& s) { songQueue.push(s); totalSeconds += s.durationInSeconds; }

    // 2. Play Next - O(1): الأغنية الحالية تطلع من المجموع، والتالية تدخل
    bool nextSong() {
        if (hasNowPlaying) totalSeconds -= nowPlaying.durationInSeconds;
        hasNowPlaying = !songQueue.empty();
        if (hasNowPlaying) { nowPlaying = songQueue.front(); songQueue.pop(); }
        return hasNowPlaying;
    }

    // 3. View Next - O(1)
    Song peekNextSong() const { return songQueue.empty() ? noSong() : songQueue.front(); }
    bool hasNextSong() const { return !songQueue.empty(); }
    Song getCurrentSong() const { return hasNowPlaying ? nowPlaying : noSong(); }
    bool hasCurrentSong() const { return hasNowPlaying; }

    // 4. Display - O(n): الأغنية الحالية أولاً ثم الطابور بالترتيب
    std::vector<Song> getAllSongs() const {
        std::vector<Song> list;
        if (hasNowPlaying) list.push_back(nowPlaying);
        for (auto copy = songQueue; !copy.empty(); copy.pop()) list.push_back(copy.front());
        return list;
    }

    // 5. Clear - O(n)
    void clearPlaylist() { songQueue = std::queue<Song>(); hasNowPlaying = false; totalSeconds = 0; }

    // 6. Statistics - O(1)
    size_t getSongCount() const { return songQueue.size() + hasNowPlaying; }
    int getTotalDurationSeconds() const { return totalSeconds; }
    bool isEmpty() const { return getSongCount() == 0; }
    std::string getFormattedTotalDuration() const {
        char b[32];
        std::snprintf(b, sizeof b, "%02d:%02d:%02d",
            totalSeconds / 3600, totalSeconds % 3600 / 60, totalSeconds % 60);
        return b;
    }

    // Save / Load - سطر لكل أغنية: path TAB title TAB artist TAB duration
    bool saveToFile(const std::string& path) const {
        std::ofstream out(path, std::ios::trunc);
        if (!out) return false;
        for (const Song& s : getAllSongs())
            out << s.filePath << '\t' << s.title << '\t' << s.artist << '\t' << s.durationInSeconds << '\n';
        return true;
    }

    void loadFromFile(const std::string& path) {
        std::ifstream in(path);
        if (!in) return;                              // أول تشغيل ما فيه ملف
        clearPlaylist();
        for (std::string line; std::getline(in, line); ) {
            std::string f[4];
            std::stringstream ss(line);
            for (auto& field : f) std::getline(ss, field, '\t');
            if (f[3].empty() || !std::filesystem::exists(f[0])) continue;   // سطر تالف أو ملف محذوف
            addSong(Song(f[1], f[2], f[0], std::max(0, std::atoi(f[3].c_str()))));
        }
    }
};