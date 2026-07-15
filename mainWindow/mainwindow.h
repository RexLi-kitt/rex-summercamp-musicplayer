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
#include <QModelIndex>
#include <QScrollArea>
#include <QMap>

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

    // 以下两个槽从 MusicPlayer 移植（保留原函数名）
    void onMetaDataAvailable(bool available);              // 元数据可用：标题/艺术家/专辑/封面
    void on_listView_doubleClicked(const QModelIndex &index); // 双击列表项播放

    // ---------- 歌词功能（前后端）----------
    // 【后端】解析歌词文件（.lrc），存储到 m_lyricLines
    void loadLyricFile(const QString &audioFilePath);
    // 【后端】根据当前播放位置查找应高亮的歌词行索引
    int  findCurrentLyricIndex(qint64 position);
    // 【前端】刷新歌词显示，高亮当前行
    void updateLyricDisplay();

private:
    QString themeStylesheet(Theme theme); // 返回对应主题的样式表

    // 记忆化播放列表（从 MusicPlayer 移植，保留原函数名）
    QString playerListFilePath() const;
    void savePlayerList();
    void loadPlayerList();

    // 从文件夹加载封面（从 MusicPlayer 移植，保留原函数名）
    void loadCoverFromFolder();

    // ---------- 歌词相关成员 ----------
    QScrollArea *lyricScrollArea;         // 滚动区域，包裹歌词容器
    QWidget     *lyricContainer;          // 歌词容器，内含逐行 QLabel
    QVBoxLayout *lyricLayout;            // 歌词容器的垂直布局
    QList<QLabel*> m_lyricLineLabels;    // 逐行歌词标签（与 m_lyricTimeStamps 一一对应）
    QMap<qint64, QString> m_lyricLines;  // 时间(ms) → 歌词文本
    QList<qint64> m_lyricTimeStamps;     // 排序后的时间戳列表（用于快速查找）
    int m_currentLyricIndex;             // 当前高亮歌词行索引

    // ---------- 歌词辅助 ----------
    void clearLyricLines();              // 清空所有歌词行标签
    void showLyricMessage(const QString &msg); // 显示单条状态信息（如"无歌词文件"）

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

    Theme m_currentTheme;          // 当前主题

    // 多媒体成员
    QMediaPlayer *m_player;
    QMediaPlaylist *m_playlist;
    QTimer *m_progressTimer;

    PlayMode m_currentPlayMode;    // 当前播放模式
    bool m_isSliderDragging;       // 进度条是否正在拖动

    QString m_currentPath;         // 当前播放文件路径（供封面加载使用，从 MusicPlayer 移植）
};

#endif
