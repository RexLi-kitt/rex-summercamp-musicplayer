#include "mainwindow.h"
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QMenu>
#include <QAction>
#include <QDir>
#include <QItemSelectionModel>
#include <QFileInfo>
// 以下为从 MusicPlayer 移植功能所需的头文件
#include <QMediaMetaData>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QChar>
#include <QImage>
#include <QPixmap>
#include <QVariant>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent), m_currentTheme(Heal)
{
    // 1. 创建一个中央容器
    QWidget *centralWidget = new QWidget(this);
    setCentralWidget(centralWidget);

    // 2. 创建主垂直布局
    QVBoxLayout *mainLayout = new QVBoxLayout(centralWidget);

    // ========== 第一行：播放控制 ==========
    QHBoxLayout *controlLayout = new QHBoxLayout();
    btnPrev = new QPushButton("⏮", this);
    btnPlay = new QPushButton("▶", this);
    btnNext = new QPushButton("⏭", this);
    btnMute = new QPushButton("🔇", this);
    sliderVolume = new QSlider(Qt::Horizontal, this);
    sliderVolume->setRange(0, 100);

    controlLayout->addWidget(btnPrev);
    controlLayout->addWidget(btnPlay);
    controlLayout->addWidget(btnNext);
    controlLayout->addStretch(1);
    controlLayout->addWidget(btnMute);
    controlLayout->addWidget(sliderVolume);

    // ========== 第二行：歌曲信息 + 封面 ==========
    QHBoxLayout *infoLayout = new QHBoxLayout();
    labelCover = new QLabel(this);
    labelCover->setFixedSize(80, 80);
    labelCover->setStyleSheet("border: 1px solid gray;");

    QVBoxLayout *infoTextLayout = new QVBoxLayout();
    labelTitle = new QLabel("歌名：未知", this);
    labelArtist = new QLabel("歌手：未知", this);
    labelAlbum = new QLabel("专辑：未知", this);
    infoTextLayout->addWidget(labelTitle);
    infoTextLayout->addWidget(labelArtist);
    infoTextLayout->addWidget(labelAlbum);

    infoLayout->addWidget(labelCover);
    infoLayout->addLayout(infoTextLayout);
    infoLayout->addStretch(1);

    // ========== 第三行：操作按钮 ==========
    QHBoxLayout *actionLayout = new QHBoxLayout();
    btnAddFile = new QPushButton("添加文件", this);
    btnAddFolder = new QPushButton("添加文件夹", this);
    btnRemove = new QPushButton("移除选中", this);
    btnTheme = new QPushButton("🎨 主题", this);      // 新增主题按钮
    btnPlayMode = new QPushButton("🔁 列表循环", this);

    actionLayout->addWidget(btnAddFile);
    actionLayout->addWidget(btnAddFolder);
    actionLayout->addWidget(btnRemove);
    actionLayout->addStretch(1);
    actionLayout->addWidget(btnTheme);                 // 放在播放模式前
    actionLayout->addWidget(btnPlayMode);

    // ========== 第四行：播放列表 ==========
    listViewPlaylist = new QListView(this);
    playlistModel = new QStringListModel(this);
    listViewPlaylist->setModel(playlistModel);

    // ========== 第五行：进度条 ==========
    QHBoxLayout *progressLayout = new QHBoxLayout();
    labelCurrentTime = new QLabel("00:00", this);
    sliderProgress = new QSlider(Qt::Horizontal, this);
    sliderProgress->setRange(0, 0);   // 初始为0，待加载歌曲后由 onMediaPlayerDurationChanged 更新
    labelTotalTime = new QLabel("00:00", this);

    progressLayout->addWidget(labelCurrentTime);
    progressLayout->addWidget(sliderProgress);
    progressLayout->addWidget(labelTotalTime);

    // ========== 把所有行添加到主布局 ==========
    mainLayout->addLayout(controlLayout);
    mainLayout->addLayout(infoLayout);
    mainLayout->addLayout(actionLayout);
    mainLayout->addWidget(listViewPlaylist);
    mainLayout->addLayout(progressLayout);

    // ========== 多媒体初始化 ==========
    m_player = new QMediaPlayer(this);
    m_playlist = new QMediaPlaylist(this);
    m_player->setPlaylist(m_playlist);

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(200);

    m_currentPlayMode = LoopList;
    m_isSliderDragging = false;

    // 音量默认值
    sliderVolume->setValue(70);
    m_player->setVolume(70);

    // ========== 连接信号 ==========

    // 音量滑块到播放器
    connect(sliderVolume, &QSlider::valueChanged, this, &MainWindow::onVolumeChange);

    // 进度条拖动
    connect(sliderProgress, &QSlider::sliderPressed, this, [this]() {
        m_isSliderDragging = true;
    });
    connect(sliderProgress, &QSlider::sliderReleased, this, [this]() {
        m_isSliderDragging = false;
        onProgressChange(sliderProgress->value());
    });

    // 播放控制按钮
    connect(btnPlay, &QPushButton::clicked, this, &MainWindow::onPlayPause);
    connect(btnPrev, &QPushButton::clicked, this, &MainWindow::onPrev);
    connect(btnNext, &QPushButton::clicked, this, &MainWindow::onNext);
    connect(btnMute, &QPushButton::clicked, this, &MainWindow::onMuteToggle);
    connect(btnPlayMode, &QPushButton::clicked, this, &MainWindow::onPlayModeToggle);

    // 功能按钮
    connect(btnAddFile, &QPushButton::clicked, this, &MainWindow::onAddFile);
    connect(btnAddFolder, &QPushButton::clicked, this, &MainWindow::onAddFolder);
    connect(btnRemove, &QPushButton::clicked, this, &MainWindow::onRemoveSelected);

    // 主题按钮
    connect(btnTheme, &QPushButton::clicked, this, &MainWindow::showThemeMenu);

    // 播放器信号
    connect(m_player, &QMediaPlayer::positionChanged, this, &MainWindow::onMediaPlayerPositionChanged);
    connect(m_player, &QMediaPlayer::durationChanged, this, &MainWindow::onMediaPlayerDurationChanged);
    connect(m_player, &QMediaPlayer::stateChanged, this, &MainWindow::onMediaPlayerStateChanged);
    connect(m_player, &QMediaPlayer::mediaChanged, this, &MainWindow::onMediaPlayerMediaChanged);

    // 元数据可用信号（从 MusicPlayer 移植：用于读取标题/艺术家/专辑/封面）
    connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, &MainWindow::onMetaDataAvailable);

    // 播放列表信号
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::onPlaylistCurrentIndexChanged);

    // 双击列表项播放（从 MusicPlayer 移植，保留原槽函数名）
    connect(listViewPlaylist, &QListView::doubleClicked, this, &MainWindow::on_listView_doubleClicked);

    // 进度更新定时器
    connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::updateProgress);

    // 加载上次保存的播放列表（从 MusicPlayer 移植）
    loadPlayerList();

    // 应用默认主题（治愈系）
    applyTheme(Heal);

    setWindowTitle("音乐播放器");
    resize(600, 500);
}

MainWindow::~MainWindow()
{
    // （从 MusicPlayer 移植）
    savePlayerList();
}

void MainWindow::showThemeMenu()
{
    QMenu menu(this);
    QAction *healAction = menu.addAction("🌿 治愈系（淡绿）");
    QAction *darkAction = menu.addAction("🌙 暗黑系（深色）");

    QAction *chosen = menu.exec(btnTheme->mapToGlobal(QPoint(0, btnTheme->height())));
    if (chosen == healAction) {
        applyTheme(Heal);
    } else if (chosen == darkAction) {
        applyTheme(Dark);
    }
}

void MainWindow::applyTheme(Theme theme)
{
    m_currentTheme = theme;
    setStyleSheet(themeStylesheet(theme));
}

QString MainWindow::themeStylesheet(Theme theme)
{
    if (theme == Heal) {
        return R"(
            /* 主窗口背景：纯色，不突兀 */
            QMainWindow {
                background-color: #F5F5F0;
            }
                        /* 按钮渐变 - 强制覆盖默认蓝色，所有状态颜色统一 */
                        QPushButton {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #C8E6C9, stop:1 #E8F5E9);
                            border: 1px solid #A5D6A7;
                            border-radius: 4px;
                            padding: 4px 8px;
                            color: #2E7D32;
                        }
                        QPushButton:hover {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #A5D6A7, stop:1 #C8E6C9);
                            color: #1B5E20;
                        }
                        QPushButton:pressed {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #81C784, stop:1 #A5D6A7);
                            color: #1B5E20;
                        }
                        QPushButton:focus {
                            color: #2E7D32;
                        }
                        QPushButton:!focus {
                            color: #2E7D32;
                        }
            /* 滑块渐变 */
            QSlider::groove:horizontal {
                height: 6px;
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #C8E6C9, stop:1 #E8F5E9);
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: #66BB6A;
                width: 14px;
                height: 14px;
                margin: -4px 0;
                border-radius: 7px;
            }
            /* 标签 */
            QLabel {
                color: #1B5E20;
            }
            /* 列表视图 */
            QListView {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #F1F8E9, stop:1 #E8F5E9);
                border: 1px solid #A5D6A7;
                border-radius: 4px;
                color: #1B5E20;
            }
        )";
    } else { // Dark
        return R"(
            QMainWindow {
                background-color: #2D2D2D;
            }
                        QPushButton {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #424242, stop:1 #616161);
                            border: 1px solid #757575;
                            border-radius: 4px;
                            padding: 4px 8px;
                            color: #E0E0E0;
                        }
                        QPushButton:hover {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #616161, stop:1 #757575);
                            color: #FFFFFF;
                        }
                        QPushButton:pressed {
                            background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                                stop:0 #757575, stop:1 #9E9E9E);
                            color: #FFFFFF;
                        }
                        QPushButton:focus {
                            color: #E0E0E0;
                        }
                        QPushButton:!focus {
                            color: #E0E0E0;
                        }
            QSlider::groove:horizontal {
                height: 6px;
                background: qlineargradient(x1:0, y1:0, x2:1, y2:0,
                    stop:0 #424242, stop:1 #616161);
                border-radius: 3px;
            }
            QSlider::handle:horizontal {
                background: #BDBDBD;
                width: 14px;
                height: 14px;
                margin: -4px 0;
                border-radius: 7px;
            }
            QLabel {
                color: #E0E0E0;
            }
            QListView {
                background: qlineargradient(x1:0, y1:0, x2:1, y2:1,
                    stop:0 #383838, stop:1 #424242);
                border: 1px solid #757575;
                border-radius: 4px;
                color: #E0E0E0;
            }
                )";
    }
}

void MainWindow::onAddFile()
{
    QStringList files = QFileDialog::getOpenFileNames(this, "选择音频文件", QString(),
        "音频文件 (*.mp3 *.wav *.flac *.ogg *.wma *.aac);;所有文件 (*)");

    if (files.isEmpty())
        return;

        QStringList currentList = playlistModel->stringList();
    currentList.append(files);
    playlistModel->setStringList(currentList);

    // 同时添加到 QMediaPlaylist
    for (const QString &file : files) {
        m_playlist->addMedia(QUrl::fromLocalFile(file));
    }
}

void MainWindow::onAddFolder()
{
    QString dir = QFileDialog::getExistingDirectory(this, "选择文件夹", QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);

    if (dir.isEmpty())
        return;

    QDir directory(dir);
    QStringList filters;
    filters << "*.mp3" << "*.wav" << "*.flac" << "*.ogg" << "*.wma" << "*.aac";
    QStringList files = directory.entryList(filters, QDir::Files);

    if (files.isEmpty()) {
        QMessageBox::information(this, "提示", "所选文件夹中没有找到支持的音频文件。");
        return;
    }

    // 将文件名转为完整路径
    QStringList fullPaths;
    for (const QString &file : files) {
        fullPaths.append(dir + "/" + file);
    }

        QStringList currentList = playlistModel->stringList();
    currentList.append(fullPaths);
    playlistModel->setStringList(currentList);

    // 同时添加到 QMediaPlaylist
    for (const QString &path : fullPaths) {
        m_playlist->addMedia(QUrl::fromLocalFile(path));
    }
}

void MainWindow::onRemoveSelected()
{
    QModelIndexList selected = listViewPlaylist->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) {
        QMessageBox::warning(this, "警告", "请先选中要移除的歌曲。");
        return;
    }

    QStringList currentList = playlistModel->stringList();

    // 从后往前删除，避免索引错乱
    std::sort(selected.begin(), selected.end(), [](const QModelIndex &a, const QModelIndex &b) {
        return a.row() > b.row();
    });

        for (const QModelIndex &index : selected) {
        if (index.row() >= 0 && index.row() < currentList.size()) {
            currentList.removeAt(index.row());
            m_playlist->removeMedia(index.row());
        }
    }

    playlistModel->setStringList(currentList);
}

// ========== 工具函数：时间格式化 ==========
static QString formatTime(qint64 ms)
{
    int seconds = static_cast<int>(ms / 1000);
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

// ========== 播放控制函数（移植自 MusicPlayer 对应槽函数）==========
// 说明：保留 QMediaPlaylist 架构 + m_progressTimer + 空列表判断，
//       核心逻辑从 MusicPlayer 迁移。

void MainWindow::onPlayPause()
{
    if (m_playlist->mediaCount() == 0) {
        QMessageBox::information(this, "提示", "播放列表为空，请先添加歌曲。");
        return;
    }

    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
        m_progressTimer->stop();
        btnPlay->setText("▶");
    } else {
        m_player->play();
        m_progressTimer->start();
        btnPlay->setText("■");
    }
}

void MainWindow::onPrev()
{
    if (m_playlist->mediaCount() == 0) return;
    m_playlist->previous();
}

void MainWindow::onNext()
{
    if (m_playlist->mediaCount() == 0) return;
    m_playlist->next();
}

// 移植自 MusicPlayer::on_mutebttn_clicked，改用 isMuted/setMuted 方式
void MainWindow::onMuteToggle()
{
    if (m_player->isMuted()) {
        m_player->setMuted(false);
        btnMute->setText("🔊");
    } else {
        m_player->setMuted(true);
        btnMute->setText("🔇");
    }
}

// 移植自 MusicPlayer::on_loopbttn_clicked，保留 QMediaPlaylist::setPlaybackMode
void MainWindow::onPlayModeToggle()
{
    switch (m_currentPlayMode) {
    case LoopList:
        m_playlist->setPlaybackMode(QMediaPlaylist::Loop);
        m_currentPlayMode = LoopOne;
        btnPlayMode->setText("🔂 单曲循环");
        break;
    case LoopOne:
        m_playlist->setPlaybackMode(QMediaPlaylist::Random);
        m_currentPlayMode = RandomPlay;
        btnPlayMode->setText("🔀 随机播放");
        break;
    case RandomPlay:
        m_playlist->setPlaybackMode(QMediaPlaylist::Sequential);
        m_currentPlayMode = LoopList;
        btnPlayMode->setText("🔁 列表循环");
        break;
    }
}

// 移植自 MusicPlayer::on_horizontalSlider_valueChanged，保留静音/音量状态同步
void MainWindow::onVolumeChange(int value)
{
    m_player->setVolume(value);
    if (value > 0) {
        m_player->setMuted(false);
        btnMute->setText("🔊");
    } else {
        m_player->setMuted(true);
        btnMute->setText("🔇");
    }
}

// 移植自 MusicPlayer::on_progressSlider_sliderMoved，进度条已改为毫秒直设
void MainWindow::onProgressChange(int value)
{
    m_player->setPosition(value);
}

// ========== 播放器回调 ==========

void MainWindow::onMediaPlayerPositionChanged(qint64 position)
{
    Q_UNUSED(position);
}

// 移植自 MusicPlayer::onDurationChanged，新增进度条最大值为时长（毫秒）
void MainWindow::onMediaPlayerDurationChanged(qint64 duration)
{
    labelTotalTime->setText(formatTime(duration));
    sliderProgress->setMaximum(static_cast<int>(duration));
}

// 移植自 MusicPlayer::onMediaPlayerStateChanged，保留定时器管理 + 进度条归零
void MainWindow::onMediaPlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState) {
        btnPlay->setText("■");
        if (!m_progressTimer->isActive())
            m_progressTimer->start();
    } else if (state == QMediaPlayer::PausedState) {
        btnPlay->setText("▶");
        m_progressTimer->stop();
    } else {
        btnPlay->setText("▶");
        m_progressTimer->stop();
        sliderProgress->setValue(0);
        labelCurrentTime->setText("00:00");
    }
}

void MainWindow::onMediaPlayerMediaChanged(const QMediaContent &media)
{
    QString filePath = media.canonicalUrl().toLocalFile();
    m_currentPath = filePath;   // 记录当前路径，供 onMetaDataAvailable/loadCoverFromFolder 使用

    if (filePath.isEmpty()) {
        labelTitle->setText("歌名：未知");
        labelArtist->setText("歌手：未知");
        labelAlbum->setText("专辑：未知");
        return;
    }

    // 先用文件名做初始显示，待元数据可用时由 onMetaDataAvailable 覆盖
    QFileInfo fileInfo(filePath);
    labelTitle->setText("歌名：" + fileInfo.completeBaseName());
    labelArtist->setText("歌手：未知");
    labelAlbum->setText("专辑：未知");
}

void MainWindow::onPlaylistCurrentIndexChanged(int index)
{
    if (index >= 0 && index < playlistModel->stringList().size()) {
        QModelIndex modelIndex = playlistModel->index(index);
        listViewPlaylist->setCurrentIndex(modelIndex);
    }
}

// ========== 定时器更新进度 ==========

// 移植自 MusicPlayer::onPositionChanged，进度条改用毫秒直设（不再用 0-1000 比例）
void MainWindow::updateProgress()
{
    if (m_isSliderDragging)
        return;

    qint64 position = m_player->position();
    sliderProgress->setValue(static_cast<int>(position));
    labelCurrentTime->setText(formatTime(position));
}

// ========== 以下为从 MusicPlayer 移植的功能（保留原函数名）==========

// 元数据可用：读取标题/艺术家/专辑/封面（从 MusicPlayer::onMetaDataAvailable 移植）
void MainWindow::onMetaDataAvailable(bool available)
{
    if (!available) return;

    QString title  = m_player->metaData(QMediaMetaData::Title).toString();
    QString artist = m_player->metaData(QMediaMetaData::Author).toString();
    QString album  = m_player->metaData(QMediaMetaData::AlbumTitle).toString();

    // 如果元数据为空，用文件名/占位文本
    if (title.isEmpty()) {
        title = QFileInfo(m_currentPath).completeBaseName();
    }
    if (artist.isEmpty()) {
        artist = "未知";
    }
    if (album.isEmpty()) {
        album = "未知";
    }

    labelTitle->setText("歌名：" + title);
    labelArtist->setText("歌手：" + artist);
    labelAlbum->setText("专辑：" + album);

    // 找封面：依次尝试 CoverArtImage / ThumbnailImage / "CoverArtImage"
    QVariant coverVariant;
    coverVariant = m_player->metaData(QMediaMetaData::CoverArtImage);
    if (!coverVariant.isValid()) {
        coverVariant = m_player->metaData(QMediaMetaData::ThumbnailImage);
    }
    if (!coverVariant.isValid()) {
        coverVariant = m_player->metaData("CoverArtImage");
    }

    if (coverVariant.isValid()) {
        // QVariant 可能是 QImage，也可能是 QByteArray
        QImage coverImage;
        if (coverVariant.canConvert<QImage>()) {
            coverImage = coverVariant.value<QImage>();
        } else if (coverVariant.canConvert<QByteArray>()) {
            coverImage.loadFromData(coverVariant.value<QByteArray>());
        }

        if (!coverImage.isNull()) {
            QPixmap pixmap = QPixmap::fromImage(coverImage);
            labelCover->setPixmap(pixmap.scaled(
                labelCover->size(),
                Qt::KeepAspectRatio,
                Qt::SmoothTransformation));
            return;
        }
    }
    // 没找到内嵌封面，去文件夹找
    loadCoverFromFolder();
}

// 从文件夹加载封面（从 MusicPlayer::loadCoverFromFolder 原样移植，coverLabel→labelCover）
void MainWindow::loadCoverFromFolder()
{
    if (m_currentPath.isEmpty()) return;

    QFileInfo fileInfo(m_currentPath);
    QDir dir = fileInfo.absoluteDir();

    // 常见的封面文件名
    QStringList names = {"folder.jpg", "cover.jpg", "front.jpg", "folder.png", "cover.png"};

    for (const QString &name : names) {
        QString coverPath = dir.absoluteFilePath(name);
        if (QFile::exists(coverPath)) {
            QPixmap pixmap(coverPath);
            if (!pixmap.isNull()) {
                labelCover->setPixmap(pixmap.scaled(
                    labelCover->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
                return;
            }
        }
    }
}

// 双击列表项播放（从 MusicPlayer::on_listView_doubleClicked 移植，适配 QMediaPlaylist）
void MainWindow::on_listView_doubleClicked(const QModelIndex &index)
{
    if (!index.isValid()) return;
    int row = index.row();
    if (row < 0 || row >= m_playlist->mediaCount()) return;

    m_playlist->setCurrentIndex(row);
    m_player->play();
}

// 记忆化播放列表：保存（适配 QStringListModel，原实现用 QStandardItemModel）
void MainWindow::savePlayerList()
{
    QFile file(playerListFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;   // 无效文件
    }
    QTextStream out(&file);
    QStringList list = playlistModel->stringList();
    for (const QString &path : list) {
        out << path << "\n";   // 每行一个文件路径
    }
    file.close();
}

// 记忆化播放列表：加载（适配 QStringListModel，并同步到 QMediaPlaylist）
void MainWindow::loadPlayerList()
{
    QFile file(playerListFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;   // 第一次运行，没有保存文件，直接返回
    }
    QTextStream in(&file);
    QStringList list;
    while (!in.atEnd()) {
        QString path = in.readLine().trimmed();
        if (path.isEmpty()) {
            continue;
        }
        QFileInfo info(path);
        if (!info.exists()) {
            continue;
        }
        list.append(path);
    }
    file.close();

    if (list.isEmpty()) {
        return;
    }

    playlistModel->setStringList(list);
    // 同步到 QMediaPlaylist
    for (const QString &path : list) {
        m_playlist->addMedia(QUrl::fromLocalFile(path));
    }
}

// 记忆化播放列表：保存路径（从 MusicPlayer 原样移植）
QString MainWindow::playerListFilePath() const
{
    return QCoreApplication::applicationDirPath() + "/playlist.txt";
}

