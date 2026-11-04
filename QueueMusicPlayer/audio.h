#pragma once
#include <QObject>
#include <QString>
#include <QMediaMetaData>

class QMediaPlayer;
class QAudioOutput;

class AudioPlayer : public QObject {
    Q_OBJECT
public:
    explicit AudioPlayer(QObject* parent = nullptr);

    void load(const QString& filePath);     // loads and starts playing
    void stop();                            // stops and frees the current song
    void togglePlayPause();
    void restart();                         // jump back to 00:00
    void seekMs(qint64 ms);
    void setVolume(int percent);            // 0..100

    bool isLoaded() const;
    bool isPlaying() const;
    qint64 positionMs() const;
    qint64 durationMs() const;
    QMediaMetaData metaData() const;        // tags of the current song (cover, etc.)

    // seconds, 0 if the file can't be read
    static int probeDurationSeconds(const QString& filePath);

signals:
    void finished();                        // the song reached its end
    void positionChanged(qint64 ms);
    void durationChanged(qint64 ms);
    void metaChanged();                     // metadata (cover art) became available

private:
    QMediaPlayer* player;
    QAudioOutput* output;
    bool loaded = false;
};