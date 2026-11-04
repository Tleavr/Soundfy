#include "mainwindow.h"

#include <QAbstractItemView>
#include <QCloseEvent>
#include <QCoreApplication>
#include <QDir>
#include <QFileDialog>
#include <QFont>
#include <QFontMetrics>
#include <QFrame>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QLabel>
#include <QPainter>
#include <QPixmap>
#include <QPolygonF>
#include <QPushButton>
#include <QRectF>
#include <QSlider>
#include <QStringList>
#include <QTableWidget>
#include <QVBoxLayout>

static QString fmtMs(qint64 ms)
{
    qint64 s = ms / 1000;
    return QString("%1:%2").arg(s / 60, 2, 10, QChar('0'))
        .arg(s % 60, 2, 10, QChar('0'));
}

// ---------- white icons drawn in code (no system icons) ----------
enum class IconKind { Prev, Play, Pause, Next, Stop };

static QIcon makeIcon(IconKind kind)
{
    const int s = 52;                       // drawn large, shown at 34px (sharp on HiDPI)
    const double cx = s / 2.0, cy = s / 2.0;
    const double h = s * 0.46;              // icon height
    const double w = s * 0.42;              // triangle width

    QPixmap pm(s, s);
    pm.fill(Qt::transparent);

    QPainter p(&pm);
    p.setRenderHint(QPainter::Antialiasing);
    p.setPen(Qt::NoPen);
    p.setBrush(QColor("#FFFFFF"));

    auto triRight = [&](double x) {         // ▶ with its left edge at x
        QPolygonF tri;
        tri << QPointF(x, cy - h / 2) << QPointF(x, cy + h / 2) << QPointF(x + w, cy);
        p.drawPolygon(tri);
        };
    auto triLeft = [&](double x) {          // ◀ with its right edge at x
        QPolygonF tri;
        tri << QPointF(x, cy) << QPointF(x - w, cy - h / 2) << QPointF(x - w, cy + h / 2);
        p.drawPolygon(tri);
        };
    const double bar = s * 0.08;

    switch (kind) {
    case IconKind::Play:
        triRight(cx - w / 2 + s * 0.04);
        break;
    case IconKind::Pause:
        p.drawRoundedRect(QRectF(cx - s * 0.17, cy - h / 2, s * 0.12, h), 2, 2);
        p.drawRoundedRect(QRectF(cx + s * 0.05, cy - h / 2, s * 0.12, h), 2, 2);
        break;
    case IconKind::Next:                    // ▶|
        triRight(cx - w / 2 - s * 0.06);
        p.drawRect(QRectF(cx + w / 2 - s * 0.06, cy - h / 2, bar, h));
        break;
    case IconKind::Prev:                    // |◀
        p.drawRect(QRectF(cx - w / 2 + s * 0.06 - bar, cy - h / 2, bar, h));
        triLeft(cx + w / 2 + s * 0.06);
        break;
    case IconKind::Stop:                    // ■
        p.drawRect(QRectF(cx - h * 0.4, cy - h * 0.4, h * 0.8, h * 0.8));
        break;
    }
    p.end();
    return QIcon(pm);
}

// Sets the play/pause icon from the state we KNOW we just caused,
// not from the player (Qt updates its state asynchronously).
static void setPlayIcon(QPushButton* btn, bool playing)
{
    btn->setIcon(makeIcon(playing ? IconKind::Pause : IconKind::Play));
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent)
{
    setWindowTitle("Song Playlist Queue Based");
    resize(900, 600);                       // starting size only, window is resizable
    setMinimumSize(720, 520);
    savePath = QDir(QCoreApplication::applicationDirPath()).filePath("playlist.txt");

    buildUi();

    connect(addBtn, &QPushButton::clicked, this, &MainWindow::onAdd);
    connect(clearBtn, &QPushButton::clicked, this, &MainWindow::onClear);
    connect(playBtn, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(prevBtn, &QPushButton::clicked, this, &MainWindow::onPrev);   // stop button
    connect(nextBtn, &QPushButton::clicked, this, &MainWindow::onNext);

    connect(&player, &AudioPlayer::finished, this, &MainWindow::onSongFinished);
    connect(&player, &AudioPlayer::positionChanged, this, &MainWindow::onPosition);
    connect(&player, &AudioPlayer::durationChanged, this, &MainWindow::onDuration);
    connect(&player, &AudioPlayer::metaChanged, this, &MainWindow::onMetaChanged);

    connect(progress, &QSlider::sliderPressed, this, [this] { dragging = true; });
    connect(progress, &QSlider::sliderMoved, this, [this](int v) { timeLabel->setText(fmtMs(v)); });
    connect(progress, &QSlider::sliderReleased, this, [this] {
        dragging = false;
        player.seekMs(progress->value());
        });

    connect(volume, &QSlider::valueChanged, this, &MainWindow::onVolume);
    onVolume(volume->value());

    playlist.loadFromFile(savePath.toStdString());
    refresh();
}

void MainWindow::buildUi()
{
    setStyleSheet(R"(
        QWidget#root { background: #2A2D34; }
        QFrame#card  { background: #32353D; border: 4px solid #9CA3C7; border-radius: 14px; }
        QLabel { color: #F2F2F2; background: transparent; }
        QLabel#stat { font-size: 18px; font-weight: bold; }
        QLabel#cover { background: #000000; border: 3px solid #9CA3C7; }
        QPushButton#text { background: transparent; color: #F2F2F2; border: none;
                           font-size: 20px; font-weight: bold; padding: 4px 8px; }
        QPushButton#text:hover { color: #9CA3C7; }
        QPushButton#ctrl { background: transparent; border: 4px solid #F2F2F2; border-radius: 28px;
                           min-width: 56px; max-width: 56px; min-height: 56px; max-height: 56px; }
        QPushButton#ctrl:hover { border-color: #9CA3C7; }
        QTableWidget { background: transparent; color: #F2F2F2; border: none;
                       gridline-color: transparent; font-size: 16px; }
        QTableWidget::item { padding: 6px; }
        QTableWidget::item:selected { background: #444A5C; color: #FFFFFF; }
        QHeaderView::section { background: transparent; color: #F2F2F2; border: none;
                               border-bottom: 4px solid #9CA3C7; font-size: 18px;
                               font-weight: bold; padding: 6px; }
        QSlider::groove:horizontal { height: 4px; background: #9CA3C7; border-radius: 2px; }
        QSlider::sub-page:horizontal { background: #F2F2F2; border-radius: 2px; }
        QSlider::handle:horizontal { background: #F2F2F2; width: 11px; height: 11px;
                                     margin: -4px 0; border-radius: 5px; }
    )");

    auto* root = new QWidget(this);
    root->setObjectName("root");
    setCentralWidget(root);

    auto* outer = new QVBoxLayout(root);
    outer->setContentsMargins(24, 20, 24, 20);
    outer->setSpacing(12);

    // ---------- top bar ----------
    clearBtn = new QPushButton("Clear Playlist  \u2715");
    clearBtn->setObjectName("text");
    addBtn = new QPushButton("Add song  +");
    addBtn->setObjectName("text");

    auto* top = new QHBoxLayout;
    top->addWidget(clearBtn);
    top->addStretch();
    top->addWidget(addBtn);

    // ---------- playlist card ----------
    auto* listCard = new QFrame;
    listCard->setObjectName("card");
    auto* listLay = new QVBoxLayout(listCard);
    listLay->setContentsMargins(14, 10, 14, 10);

    table = new QTableWidget(0, 3);
    table->setHorizontalHeaderLabels({ "Song Name", "Artist", "Duration" });
    table->horizontalHeader()->setDefaultAlignment(Qt::AlignCenter);
    table->verticalHeader()->setVisible(false);
    table->setEditTriggers(QAbstractItemView::NoEditTriggers);
    table->setSelectionBehavior(QAbstractItemView::SelectRows);
    table->setSelectionMode(QAbstractItemView::SingleSelection);
    table->setShowGrid(false);
    table->setFocusPolicy(Qt::NoFocus);
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    listLay->addWidget(table);

    // ---------- totals under the card ----------
    totalSongsLabel = new QLabel;
    totalSongsLabel->setObjectName("stat");
    totalDurLabel = new QLabel;
    totalDurLabel->setObjectName("stat");

    auto* totals = new QHBoxLayout;
    totals->addWidget(totalSongsLabel);
    totals->addStretch();
    totals->addWidget(totalDurLabel);

    // ---------- player card ----------
    const int kCover = 96;                  // cover size (px)
    const int kSide = 190;                 // width of the left and right boxes (px)

    // The right column width = the real width of the "00:00" text,
    // so the volume bar, the time and "Next:" all match it exactly.
    QFont timeFont;
    timeFont.setPixelSize(36);
    timeFont.setBold(true);
    const int kInfo = QFontMetrics(timeFont).horizontalAdvance("00:00") + 8;

    auto* playerCard = new QFrame;
    playerCard->setObjectName("card");
    playerCard->setMinimumHeight(130);
    auto* pl = new QHBoxLayout(playerCard);
    pl->setContentsMargins(16, 12, 16, 12);
    pl->setSpacing(18);

    // ----- LEFT box: cover -----
    coverLabel = new QLabel;
    coverLabel->setObjectName("cover");
    coverLabel->setAlignment(Qt::AlignCenter);
    coverLabel->setFixedSize(kCover, kCover);

    auto* leftBox = new QWidget;
    leftBox->setFixedWidth(kSide);
    auto* leftLay = new QHBoxLayout(leftBox);
    leftLay->setContentsMargins(0, 0, 0, 0);
    leftLay->addWidget(coverLabel, 0, Qt::AlignLeft | Qt::AlignVCenter);

    // ----- MIDDLE: song name above progress, controls below -----
    nowTitle = new QLabel("Nothing playing");
    nowTitle->setAlignment(Qt::AlignCenter);
    nowTitle->setStyleSheet("font-size: 16px; font-weight: bold;");

    progress = new QSlider(Qt::Horizontal);
    progress->setRange(0, 0);

    prevBtn = new QPushButton;              // acts as STOP: stops and rewinds to 00:00
    prevBtn->setObjectName("ctrl");
    prevBtn->setIcon(makeIcon(IconKind::Stop));
    prevBtn->setIconSize(QSize(34, 34));
    prevBtn->setToolTip("STOP PLAYBACK");

    playBtn = new QPushButton;
    playBtn->setObjectName("ctrl");
    playBtn->setIcon(makeIcon(IconKind::Play));
    playBtn->setIconSize(QSize(34, 34));

    nextBtn = new QPushButton;
    nextBtn->setObjectName("ctrl");
    nextBtn->setIcon(makeIcon(IconKind::Next));
    nextBtn->setIconSize(QSize(34, 34));

    auto* ctrls = new QHBoxLayout;
    ctrls->setSpacing(18);
    ctrls->addStretch();
    ctrls->addWidget(prevBtn);
    ctrls->addWidget(playBtn);
    ctrls->addWidget(nextBtn);
    ctrls->addStretch();

    auto* mid = new QVBoxLayout;
    mid->setSpacing(6);
    mid->addStretch();
    mid->addWidget(nowTitle);
    mid->addWidget(progress);
    mid->addLayout(ctrls);
    mid->addStretch();

    // ----- RIGHT box: volume bar, live time, "Next:", next song name -----
    volume = new QSlider(Qt::Horizontal);   // bar only, no percentage text
    volume->setRange(0, 100);
    volume->setValue(70);
    volume->setFixedWidth(kInfo);

    // hidden helper label: kept so onVolume() can update it safely, never shown
    volLabel = new QLabel("70%");
    volLabel->hide();

    timeLabel = new QLabel("00:00");
    timeLabel->setFont(timeFont);
    timeLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    timeLabel->setFixedWidth(kInfo);

    auto* nextHead = new QLabel("Next:");   // fixed caption
    nextHead->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    nextHead->setStyleSheet("font-size: 13px;");
    nextHead->setFixedWidth(kInfo);

    nextLabel = new QLabel("-");            // next song name, elided in refresh()
    nextLabel->setAlignment(Qt::AlignRight | Qt::AlignVCenter);
    nextLabel->setStyleSheet("font-size: 13px;");
    nextLabel->setFixedWidth(kInfo);

    // artistLabel is kept in code but not shown, to match the design
    artistLabel = new QLabel;

    auto* rightBox = new QWidget;
    rightBox->setFixedWidth(kSide);
    auto* info = new QVBoxLayout(rightBox);
    info->setContentsMargins(0, 0, 0, 0);
    info->setSpacing(2);
    info->addWidget(volume, 0, Qt::AlignRight);
    info->addWidget(timeLabel, 0, Qt::AlignRight);
    info->addWidget(nextHead, 0, Qt::AlignRight);
    info->addWidget(nextLabel, 0, Qt::AlignRight);
    info->addStretch();

    pl->addWidget(leftBox, 0, Qt::AlignVCenter);
    pl->addLayout(mid, 1);                  // takes the middle, so it stays centred
    pl->addWidget(rightBox, 0, Qt::AlignVCenter);

    // ---------- assemble ----------
    outer->addLayout(top);
    outer->addWidget(listCard, 1);          // the list takes all the extra height
    outer->addLayout(totals);
    outer->addWidget(playerCard);
}

// ---------------- cover art ----------------

void MainWindow::setCover(const QImage& img)
{
    if (img.isNull()) {                     // no picture: empty box, no text
        coverLabel->clear();
        return;
    }
    QPixmap pm = QPixmap::fromImage(img).scaled(88, 88,
        Qt::KeepAspectRatioByExpanding, Qt::SmoothTransformation);
    pm = pm.copy((pm.width() - 88) / 2, (pm.height() - 88) / 2, 88, 88);   // center crop
    coverLabel->setPixmap(pm);
}

void MainWindow::onMetaChanged()
{
    QMediaMetaData md = player.metaData();
    QImage img = md.value(QMediaMetaData::CoverArtImage).value<QImage>();
    if (img.isNull())
        img = md.value(QMediaMetaData::ThumbnailImage).value<QImage>();
    setCover(img);
}

// ---------------- actions ----------------

void MainWindow::onAdd()
{
    // multiple selection: hold Ctrl or Shift in the dialog
    QStringList paths = QFileDialog::getOpenFileNames(this, "Choose music files", QString(),
        "Audio Files (*.mp3 *.wav *.ogg);;All Files (*)");
    if (paths.isEmpty()) return;

    for (const QString& path : paths) {
        int dur = AudioPlayer::probeDurationSeconds(path);
        playlist.addSong(MakeSongFromPath(path.toStdString(), dur));   // enqueue, O(1)
    }
    refresh();
}

void MainWindow::onClear()
{
    playlist.clearPlaylist();
    player.stop();
    setPlayIcon(playBtn, false);
    refresh();
}

void MainWindow::onPlayPause()
{
    if (!player.isLoaded()) {               // nothing playing: start the next song
        if (!playlist.isEmpty()) playNext();
        return;
    }
    bool wasPlaying = player.isPlaying();
    player.togglePlayPause();
    setPlayIcon(playBtn, !wasPlaying);      // icon follows what we just did
    refresh();
}

// The left button is STOP: pause, rewind the current song to 00:00.
void MainWindow::onPrev()
{
    if (!player.isLoaded()) return;
    player.restart();                       // position back to 0
    if (player.isPlaying())
        player.togglePlayPause();           // stop (pause) at the start
    setPlayIcon(playBtn, false);
    refresh();
}

void MainWindow::onNext()
{
    playNext();
}

void MainWindow::onSongFinished()
{
    playNext();                             // auto-advance
}

void MainWindow::playNext()
{
    if (playlist.nextSong()) {              // dequeue, O(1)
        setCover(QImage());                 // clear the old cover before the new song loads
        player.load(QString::fromStdString(playlist.getCurrentSong().filePath));
        setPlayIcon(playBtn, true);         // load() starts playing right away
    }
    else {
        player.stop();
        setPlayIcon(playBtn, false);
    }
    refresh();
}

// ---------------- live updates ----------------

void MainWindow::onPosition(qint64 ms)
{
    if (!dragging) progress->setValue(static_cast<int>(ms));
    timeLabel->setText(fmtMs(ms));          // real elapsed time
}

void MainWindow::onDuration(qint64 ms)
{
    progress->setRange(0, static_cast<int>(ms));
}

void MainWindow::onVolume(int v)
{
    player.setVolume(v);
    volLabel->setText(QString("%1%").arg(v));   // hidden, kept for consistency
}

void MainWindow::closeEvent(QCloseEvent* e)
{
    playlist.saveToFile(savePath.toStdString());
    player.stop();
    QMainWindow::closeEvent(e);
}

// ---------------- refresh the whole UI from the Playlist ----------------

void MainWindow::refresh()
{
    // table: playing song first (row 0), then the queue in order
    table->setRowCount(0);
    const auto songs = playlist.getAllSongs();
    for (const Song& s : songs) {
        int row = table->rowCount();
        table->insertRow(row);

        auto* nameItem = new QTableWidgetItem(QString::fromStdString(s.title));
        auto* artistItem = new QTableWidgetItem(QString::fromStdString(s.artist));
        auto* durItem = new QTableWidgetItem(QString::fromStdString(s.getFormattedDuration()));

        nameItem->setTextAlignment(Qt::AlignCenter);
        artistItem->setTextAlignment(Qt::AlignCenter);
        durItem->setTextAlignment(Qt::AlignCenter);

        table->setItem(row, 0, nameItem);
        table->setItem(row, 1, artistItem);
        table->setItem(row, 2, durItem);
    }
    if (playlist.hasCurrentSong() && table->rowCount() > 0)
        table->selectRow(0);                // highlight the playing song

    // totals: count, and total duration as HH:MM:SS
    totalSongsLabel->setText("Total songs : " + QString::number(qulonglong(playlist.getSongCount())));
    totalDurLabel->setText("Total Duration : " + QString::fromStdString(playlist.getFormattedTotalDuration()));

    // now playing: title (+ artist, hidden in the layout)
    if (playlist.hasCurrentSong()) {
        Song c = playlist.getCurrentSong();
        nowTitle->setText(QString::fromStdString(c.title));
        artistLabel->setText(QString::fromStdString(c.artist));
    }
    else {
        nowTitle->setText("Nothing playing");
        artistLabel->setText(QString());
    }

    // next song: shortened with "..." so it never goes wider than the right column
    QString nextName = playlist.hasNextSong()
        ? QString::fromStdString(playlist.peekNextSong().title)
        : QString("-");
    QFontMetrics fm(nextLabel->font());
    nextLabel->setText(fm.elidedText(nextName, Qt::ElideRight, nextLabel->width()));
    nextLabel->setToolTip(nextName);        // full name on hover

    if (!player.isLoaded()) {
        timeLabel->setText("00:00");
        progress->setRange(0, 0);
        setCover(QImage());
    }
}