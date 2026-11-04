cmake_minimum_required(VERSION 3.16)
project(QueueMusicPlayer LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_AUTOMOC ON)

find_package(Qt6 REQUIRED COMPONENTS Widgets Multimedia)

qt_add_executable(QueueMusicPlayer WIN32
    QueueMusicPlayer/main.cpp
    QueueMusicPlayer/mainwindow.h QueueMusicPlayer/mainwindow.cpp
    QueueMusicPlayer/audio.h QueueMusicPlayer/audio.cpp
    QueueMusicPlayer/playlist.h QueueMusicPlayer/playlist.cpp
    QueueMusicPlayer/song.h
)

target_link_libraries(QueueMusicPlayer PRIVATE Qt6::Widgets Qt6::Multimedia)