#ifndef PLAYLIST_H
#define PLAYLIST_H

#include <queue>
#include <string>
#include <vector>
#include "song.h"

// Queue (FIFO) based playlist.
//  - songQueue    : songs waiting to be played (front = next song)
//  - nowPlaying   : the song that was dequeued and is playing right now
//  - totalSeconds : running total of every song in the playlist
//                   (nowPlaying + queue), kept up to date on every change
//                   so the statistics are O(1) instead of O(n)
//
// n = number of songs waiting in the queue
class Playlist {
private:
    std::queue<Song> songQueue;
    Song nowPlaying;
    bool hasNowPlaying = false;
    int  totalSeconds = 0;

public:
    // 1. Add Song      : enqueue at the end                         O(1)
    void addSong(const Song& song);
    // 2. Play Next     : dequeue the front song and play it         O(1)
    //                    returns false (and nothing is playing) if the queue is empty
    bool nextSong();
    // 3. View Next     : peek at the front without removing it      O(1)
    Song peekNextSong() const;
    bool hasNextSong() const;

    Song getCurrentSong() const;                                  // O(1)
    bool hasCurrentSong() const;                                  // O(1)

    // 4. Display       : playing song first, then the queue         O(n)
    std::vector<Song> getAllSongs() const;

    // 5. Clear                                                      O(n)
    void clearPlaylist();

    // 6. Statistics                                                 O(1)
    size_t getSongCount() const;
    int getTotalDurationSeconds() const;
    std::string getFormattedTotalDuration() const;   // HH:MM:SS
    bool isEmpty() const;

    // Saving / loading (one song per line in a plain text file)     O(n)
    bool saveToFile(const std::string& filePath) const;
    void loadFromFile(const std::string& filePath);  // skips songs whose file no longer exists
};

#endif // PLAYLIST_H
