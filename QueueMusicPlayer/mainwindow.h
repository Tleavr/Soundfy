#pragma once
#include <QMainWindow>
#include <QString>
#include <QImage>
#include "playlist.h"
#include "audio.h"

class QLabel;
class QSlider;
class QPushButton;
class QTableWidget;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);

protected:
    void closeEvent(QCloseEvent* e) override;

private slots:
    void onAdd();
    void onClear();
    void onPlayPause();
    void onPrev();
    void onNext();
    void onSongFinished();
    void onPosition(qint64 ms);
    void onDuration(qint64 ms);
    void onVolume(int v);
    void onMetaChanged();

private:
    void buildUi();
    void playNext();
    void refresh();
    void setCover(const QImage& img);

    Playlist    playlist;
    AudioPlayer player;
    QString     savePath;
    bool        dragging = false;

    // top bar
    QPushButton* clearBtn;
    QPushButton* addBtn;
    QSlider* volume;
    QLabel* volLabel;

    // playlist table
    QTableWidget* table;

    // stats under the table
    QLabel* totalSongsLabel;
    QLabel* totalDurLabel;

    // player bar
    QLabel* coverLabel;
    QSlider* progress;
    QPushButton* prevBtn;
    QPushButton* playBtn;
    QPushButton* nextBtn;
    QLabel* nowTitle;
    QLabel* artistLabel;
    QLabel* timeLabel;
    QLabel* nextLabel;
};