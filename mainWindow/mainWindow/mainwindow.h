#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QBoxLayout>
#include <QPushButton>
#include <QSlider>
#include <QLabel>
#include <QListView>
#include <QStringListModel>
#include <QMediaPlayer>
#include <QMediaPlaylist>
#include <QTimer>
#include "volumepulseoverlay.h"

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // 主题枚举
    enum Theme { Heal, Dark };
    // 播放模式枚举
    enum PlayMode { LoopList, LoopOne, RandomPlay };

    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void resizeEvent(QResizeEvent *event) override;

private slots:
    void showThemeMenu();          // 弹出主题选择菜单
    void applyTheme(Theme theme);  // 应用主题
    void onAddFile();              // 添加文件
    void onAddFolder();            // 添加文件夹
    void onRemoveSelected();       // 移除选中

    // 播放控制槽
    void onPlayPause();            // 播放/暂停
    void onPrev();                 // 上一首
    void onNext();                 // 下一首
    void onMuteToggle();           // 静音切换
    void onPlayModeToggle();       // 播放模式切换
    void onVolumeChange(int value); // 音量滑块变化
    void onProgressChange(int value); // 进度滑块拖动

    // 播放器回调槽
    void onMediaPlayerPositionChanged(qint64 position);
    void onMediaPlayerDurationChanged(qint64 duration);
    void onMediaPlayerStateChanged(QMediaPlayer::State state);
    void onMediaPlayerMediaChanged(const QMediaContent &media);
    void onPlaylistCurrentIndexChanged(int index);

    // 定时器更新进度
    void updateProgress();

    // 读取当前媒体的元数据并刷新歌名/歌手/专辑/封面
    void refreshMetadataDisplay();

private:
    QString themeStylesheet(Theme theme); // 返回对应主题的样式表

    QPushButton *btnPrev;
    QPushButton *btnPlay;
    QPushButton *btnNext;
    QPushButton *btnMute;
    QPushButton *btnAddFile;
    QPushButton *btnAddFolder;
    QPushButton *btnRemove;
    QPushButton *btnPlayMode;
    QPushButton *btnTheme;         // 主题选择按钮

    QSlider *sliderVolume;
    QSlider *sliderProgress;

    QLabel *labelCurrentTime;
    QLabel *labelTotalTime;
    QLabel *labelTitle;
    QLabel *labelArtist;
    QLabel *labelAlbum;
    QLabel *labelCover;

    QListView *listViewPlaylist;
    QStringListModel *playlistModel;

    VolumePulseOverlay *pulseOverlay;

    Theme m_currentTheme;          // 当前主题

    // 多媒体成员
    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;
    QTimer *m_progressTimer;

    PlayMode m_currentPlayMode;    // 当前播放模式
    bool m_isMuted;                // 静音状态
    int m_volumeBeforeMute;        // 静音前的音量
    bool m_isSliderDragging;       // 进度条是否正在拖动
};

#endif
