#include "playlist.h"
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <sstream>
#include <utility>

// ---------------- Core queue operations ----------------

// 1. Add Song: enqueue to the end of the playlist                      O(1)
void Playlist::addSong(const Song& song) {
    songQueue.push(song);
    totalSeconds += song.durationInSeconds;
}

// 2. Play Next: dequeue the song at the front and play it              O(1)
// The song that was playing leaves the playlist, so its duration is removed
// from the running total. The dequeued song stays counted (it is now nowPlaying).
bool Playlist::nextSong() {
    if (hasNowPlaying) totalSeconds -= nowPlaying.durationInSeconds;

    if (songQueue.empty()) {
        hasNowPlaying = false;      // nothing left to play
        return false;
    }
    nowPlaying = songQueue.front();
    songQueue.pop();
    hasNowPlaying = true;
    return true;
}

// 3. View Next Song: peek without removing                             O(1)
Song Playlist::peekNextSong() const {
    if (!songQueue.empty()) return songQueue.front();
    return Song("No Song", "Unknown", "", 0);
}

bool Playlist::hasNextSong() const {
    return !songQueue.empty();
}

Song Playlist::getCurrentSong() const {
    if (hasNowPlaying) return nowPlaying;
    return Song("No Song", "Unknown", "", 0);
}

bool Playlist::hasCurrentSong() const {
    return hasNowPlaying;
}

// ---------------- Display ----------------

// 4. Display Playlist: playing song first, then the queue in order      O(n)
// A std::queue can't be iterated, so we copy it and empty the COPY.
// The original queue is never touched.
std::vector<Song> Playlist::getAllSongs() const {
    std::vector<Song> list;
    list.reserve(getSongCount());
    if (hasNowPlaying) list.push_back(nowPlaying);
    std::queue<Song> temp = songQueue;
    while (!temp.empty()) {
        list.push_back(temp.front());
        temp.pop();
    }
    return list;
}

// 5. Clear Playlist                                                     O(n)
void Playlist::clearPlaylist() {
    std::queue<Song> empty;
    std::swap(songQueue, empty);    // releases all the songs at once
    hasNowPlaying = false;
    totalSeconds = 0;
}

// ---------------- 6. Statistics (all O(1)) ----------------

size_t Playlist::getSongCount() const {
    return songQueue.size() + (hasNowPlaying ? 1 : 0);
}

int Playlist::getTotalDurationSeconds() const {
    return totalSeconds;
}

// Total duration as HH:MM:SS
std::string Playlist::getFormattedTotalDuration() const {
    int t = getTotalDurationSeconds();
    char buffer[32];
    std::snprintf(buffer, sizeof(buffer), "%02d:%02d:%02d", t / 3600, (t % 3600) / 60, t % 60);
    return std::string(buffer);
}

bool Playlist::isEmpty() const {
    return songQueue.empty() && !hasNowPlaying;
}

// ---------------- Save / Load ----------------
// File format: each line = path <TAB> title <TAB> artist <TAB> duration
static std::string CleanField(std::string s) {
    for (char& ch : s) if (ch == '\t' || ch == '\n' || ch == '\r') ch = ' ';
    return s;
}

bool Playlist::saveToFile(const std::string& filePath) const {
    std::ofstream out(filePath, std::ios::trunc);
    if (!out) return false;
    for (const Song& s : getAllSongs()) {
        out << CleanField(s.filePath) << '\t' << CleanField(s.title) << '\t'
            << CleanField(s.artist) << '\t' << s.durationInSeconds << '\n';
    }
    return true;
}

void Playlist::loadFromFile(const std::string& filePath) {
    std::ifstream in(filePath);
    if (!in) return;                       // first run: no file yet, that's fine

    clearPlaylist();

    std::string line;
    while (std::getline(in, line)) {
        if (!line.empty() && line.back() == '\r') line.pop_back();
        std::vector<std::string> parts;
        std::stringstream ss(line);
        std::string part;
        while (std::getline(ss, part, '\t')) parts.push_back(part);
        if (parts.size() < 4) continue;    // broken line, ignore

        std::ifstream audioFile(parts[0], std::ios::binary);
        if (!audioFile) continue;          // the music file was moved or deleted

        int duration = std::atoi(parts[3].c_str());
        if (duration < 0) duration = 0;    // protect the total from corrupted data
        addSong(Song(parts[1], parts[2], parts[0], duration));   // keeps totalSeconds correct
    }
}