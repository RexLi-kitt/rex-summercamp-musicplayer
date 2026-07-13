#include "mainwindow.h"
#include <QListWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QResizeEvent>
#include <QMenu>
#include <QAction>
#include <QColor>
#include <QDir>
#include <QItemSelectionModel>
#include <QFileInfo>
#include <QSet>
#include <QMediaMetaData>

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
    btnRemove->setToolTip("提示：按住 Ctrl 点击可多选，按住 Shift 点击可连续选择，或直接框选");
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
    listViewPlaylist->setSelectionMode(QAbstractItemView::ExtendedSelection);

    // ========== 第五行：进度条 ==========
    QHBoxLayout *progressLayout = new QHBoxLayout();
    labelCurrentTime = new QLabel("00:00", this);
    sliderProgress = new QSlider(Qt::Horizontal, this);
    sliderProgress->setRange(0, 1000);
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

        // 创建覆盖在进度条上的脉动层
    pulseOverlay = new VolumePulseOverlay(sliderProgress->parentWidget());
    pulseOverlay->setGeometry(sliderProgress->geometry());
    pulseOverlay->raise();

    // ========== 多媒体初始化 ==========
    m_player = new QMediaPlayer(this);
    m_playlist = new QMediaPlaylist(this);
    m_player->setPlaylist(m_playlist);

    m_progressTimer = new QTimer(this);
    m_progressTimer->setInterval(200);

    m_currentPlayMode = LoopList;
    m_isMuted = false;
    m_volumeBeforeMute = 70;
    m_isSliderDragging = false;

    // 音量默认值
    sliderVolume->setValue(70);
    m_player->setVolume(70);

    // ========== 连接信号 ==========

    // 音量滑块到覆盖层和播放器
    connect(sliderVolume, &QSlider::valueChanged, pulseOverlay, &VolumePulseOverlay::setVolume);
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
    // 元数据变化时主动刷新显示
    connect(m_player, QOverload<>::of(&QMediaPlayer::metaDataChanged), this, &MainWindow::refreshMetadataDisplay);
    // metaDataAvailableChanged 在某些后端中更可靠，辅助触发刷新
    connect(m_player, &QMediaPlayer::metaDataAvailableChanged, this, [this](bool available) {
        if (available) refreshMetadataDisplay();
    });

    // 播放列表信号
    connect(m_playlist, &QMediaPlaylist::currentIndexChanged, this, &MainWindow::onPlaylistCurrentIndexChanged);

    // 进度更新定时器
    connect(m_progressTimer, &QTimer::timeout, this, &MainWindow::updateProgress);

    // 应用默认主题（治愈系）
    applyTheme(Heal);

    setWindowTitle("音乐播放器");
    resize(600, 500);
}

MainWindow::~MainWindow() {}

void MainWindow::resizeEvent(QResizeEvent *event)
{
    QMainWindow::resizeEvent(event);
    if (pulseOverlay && sliderProgress)
        pulseOverlay->setGeometry(sliderProgress->geometry());
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
    QString style = themeStylesheet(theme);
    // 对主窗口及其子控件应用样式表
    setStyleSheet(style);

    // 更新脉动覆盖层的颜色以匹配主题
    QColor pulseColor;
    if (theme == Heal) {
        pulseColor = QColor(100, 200, 120);   // 淡绿色
    } else {
        pulseColor = QColor(200, 60, 80);     // 暗红色
    }
    pulseOverlay->setBaseColor(pulseColor);
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
                            color: #2e7d58;
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
    QStringList duplicates;
    QStringList newFiles;

    // 检查重复：现有列表 + 本次选择内部
    QSet<QString> seenInBatch;
    for (const QString &file : files) {
        if (currentList.contains(file) || seenInBatch.contains(file)) {
            duplicates.append(file);
        } else {
            newFiles.append(file);
            seenInBatch.insert(file);
        }
    }

    // 提示已存在的文件
    if (!duplicates.isEmpty()) {
        QString dupList = duplicates.join("\n");
        QMessageBox::information(this, "重复文件",
            QString("以下文件已存在于播放列表中，已跳过：\n%1").arg(dupList));
    }

    if (newFiles.isEmpty())
        return;

    currentList.append(newFiles);
    playlistModel->setStringList(currentList);

    // 同时添加到 QMediaPlaylist
    for (const QString &file : newFiles) {
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
    QStringList duplicates;
    QStringList newPaths;

    // 检查重复：现有列表 + 本次选择内部
    QSet<QString> seenInBatch;
    for (const QString &path : fullPaths) {
        if (currentList.contains(path) || seenInBatch.contains(path)) {
            duplicates.append(path);
        } else {
            newPaths.append(path);
            seenInBatch.insert(path);
        }
    }

    // 提示已存在的文件
    if (!duplicates.isEmpty()) {
        QString dupList = duplicates.join("\n");
        QMessageBox::information(this, "重复文件",
            QString("以下文件已存在于播放列表中，已跳过：\n%1").arg(dupList));
    }

    if (newPaths.isEmpty())
        return;

    currentList.append(newPaths);
    playlistModel->setStringList(currentList);

    // 同时添加到 QMediaPlaylist
    for (const QString &path : newPaths) {
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

    // 记录当前正在播放的索引（移除前）
    int currentIndex = m_playlist->currentIndex();

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

    // 如果播放列表已空，停止播放并清除信息显示
    if (currentList.isEmpty()) {
        m_player->stop();
        labelTitle->setText("歌名：未知");
        labelArtist->setText("歌手：未知");
        labelAlbum->setText("专辑：未知");
        labelCover->clear();
        labelCurrentTime->setText("00:00");
        labelTotalTime->setText("00:00");
        sliderProgress->setValue(0);
        btnPlay->setText("▶");
    }
}

// ========== 工具函数：时间格式化 ==========
static QString formatTime(qint64 ms)
{
    int seconds = static_cast<int>(ms / 1000);
    int minutes = seconds / 60;
    seconds %= 60;
    return QString("%1:%2").arg(minutes, 2, 10, QChar('0')).arg(seconds, 2, 10, QChar('0'));
}

// ========== 播放控制函数 ==========

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

void MainWindow::onMuteToggle()
{
    m_isMuted = !m_isMuted;
    if (m_isMuted) {
        m_volumeBeforeMute = m_player->volume();
        m_player->setVolume(0);
        sliderVolume->setValue(0);
        btnMute->setText("🔇");
    } else {
        m_player->setVolume(m_volumeBeforeMute);
        sliderVolume->setValue(m_volumeBeforeMute);
        btnMute->setText("🔊");
    }
}

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

void MainWindow::onVolumeChange(int value)
{
    m_player->setVolume(value);
    if (value > 0) {
        m_isMuted = false;
        btnMute->setText("🔊");
        m_volumeBeforeMute = value;
    } else {
        m_isMuted = true;
        btnMute->setText("🔇");
    }
}

void MainWindow::onProgressChange(int value)
{
    if (m_player->duration() > 0) {
        qint64 position = static_cast<qint64>(value) * m_player->duration() / 1000;
        m_player->setPosition(position);
    }
}

// ========== 播放器回调 ==========

void MainWindow::onMediaPlayerPositionChanged(qint64 position)
{
    Q_UNUSED(position);
}

void MainWindow::onMediaPlayerDurationChanged(qint64 duration)
{
    labelTotalTime->setText(formatTime(duration));
}

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
    // QMediaContent::canonicalUrl() is deprecated; use request().url()
    QString filePath = media.request().url().toLocalFile();
    if (filePath.isEmpty()) {
        labelTitle->setText("歌名：未知");
        labelArtist->setText("歌手：未知");
        labelAlbum->setText("专辑：未知");
        labelCover->clear();
        return;
    }

    QFileInfo fileInfo(filePath);
    // 先用文件名作为初始标题，元数据加载后由 metaDataChanged 信号更新
    labelTitle->setText("歌名：" + fileInfo.completeBaseName());
    labelArtist->setText("");
    labelAlbum->setText("");
    labelCover->clear();
}

// ========== 播放列表回调 ==========

void MainWindow::refreshMetadataDisplay()
{
    QString title = m_player->metaData(QStringLiteral("Title")).toString();
    if (!title.isEmpty()) {
        labelTitle->setText(QStringLiteral("歌名：") + title);
    }
    QString artist = m_player->metaData(QStringLiteral("Author")).toString();
    if (artist.isEmpty())
        artist = m_player->metaData(QStringLiteral("ContributingArtist")).toString();
    if (!artist.isEmpty()) {
        labelArtist->setText(QStringLiteral("歌手：") + artist);
    }
    QString album = m_player->metaData(QStringLiteral("AlbumTitle")).toString();
    if (!album.isEmpty()) {
        labelAlbum->setText(QStringLiteral("专辑：") + album);
    }
    QVariant coverVariant = m_player->metaData(QStringLiteral("ThumbnailImage"));
    if (coverVariant.isValid()) {
        QImage coverImage = coverVariant.value<QImage>();
        if (!coverImage.isNull()) {
            labelCover->setPixmap(QPixmap::fromImage(coverImage).scaled(
                80, 80, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
    }
}

void MainWindow::onPlaylistCurrentIndexChanged(int index)
{
    if (index >= 0 && index < playlistModel->stringList().size()) {
        QModelIndex modelIndex = playlistModel->index(index);
        listViewPlaylist->setCurrentIndex(modelIndex);
    }
}

// ========== 定时器更新进度 ==========

void MainWindow::updateProgress()
{
    if (m_isSliderDragging)
        return;

    if (m_player->duration() > 0) {
        qint64 position = m_player->position();
        qint64 duration = m_player->duration();

        int sliderValue = static_cast<int>(position * 1000 / duration);
        sliderProgress->setValue(sliderValue);
        labelCurrentTime->setText(formatTime(position));
    }
}

