#include "audio.h"
#include <QAudioOutput>
#include <QEventLoop>
#include <QMediaPlayer>
#include <QTimer>
#include <QUrl>

AudioPlayer::AudioPlayer(QObject* parent)
    : QObject(parent),
    player(new QMediaPlayer(this)),
    output(new QAudioOutput(this))
{
    player->setAudioOutput(output);
    output->setVolume(0.7f);

    connect(player, &QMediaPlayer::mediaStatusChanged, this,
        [this](QMediaPlayer::MediaStatus s) {
            if (s == QMediaPlayer::EndOfMedia) emit finished();
        });
    connect(player, &QMediaPlayer::positionChanged, this, &AudioPlayer::positionChanged);
    connect(player, &QMediaPlayer::durationChanged, this, &AudioPlayer::durationChanged);
    connect(player, &QMediaPlayer::metaDataChanged, this, &AudioPlayer::metaChanged);
}

void AudioPlayer::load(const QString& filePath)
{
    stop();
    player->setSource(QUrl::fromLocalFile(filePath));
    player->play();
    loaded = true;
}

void AudioPlayer::stop()
{
    player->stop();
    player->setSource(QUrl());
    loaded = false;
}

void AudioPlayer::togglePlayPause()
{
    if (!loaded) return;
    if (isPlaying()) player->pause();
    else             player->play();
}

void AudioPlayer::restart()
{
    if (loaded) player->setPosition(0);
}

void AudioPlayer::seekMs(qint64 ms)
{
    if (loaded) player->setPosition(ms);
}

void AudioPlayer::setVolume(int percent)
{
    output->setVolume(percent / 100.0f);
}

bool AudioPlayer::isLoaded() const { return loaded; }

bool AudioPlayer::isPlaying() const
{
    return player->playbackState() == QMediaPlayer::PlayingState;
}

qint64 AudioPlayer::positionMs() const { return player->position(); }
qint64 AudioPlayer::durationMs() const { return player->duration(); }

QMediaMetaData AudioPlayer::metaData() const
{
    return player->metaData();
}

// Qt loads media asynchronously, so we wait (briefly) for the duration to be known.
int AudioPlayer::probeDurationSeconds(const QString& filePath)
{
    QMediaPlayer probe;
    QEventLoop loop;
    qint64 dur = 0;

    QObject::connect(&probe, &QMediaPlayer::durationChanged, &loop,
        [&](qint64 d) { dur = d; if (d > 0) loop.quit(); });
    QObject::connect(&probe, &QMediaPlayer::mediaStatusChanged, &loop,
        [&](QMediaPlayer::MediaStatus s) { if (s == QMediaPlayer::InvalidMedia) loop.quit(); });
    QTimer::singleShot(3000, &loop, &QEventLoop::quit);

    probe.setSource(QUrl::fromLocalFile(filePath));
    loop.exec();

    return static_cast<int>((dur + 500) / 1000);
}