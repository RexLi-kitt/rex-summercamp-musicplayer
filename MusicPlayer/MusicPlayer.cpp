#include "MusicPlayer.h"
#include "./ui_MusicPlayer.h"
#include<QFileDialog>
#include <QDebug>
#include<QFileInfo>
#include<QDirIterator>
#include<QRandomGenerator>
#include <QMediaMetaData>
#include <QFile>
#include <QTextStream>
#include <QCoreApplication>
#include <QChar>
#include <QImage>
#include <QPixmap>
#include <QDir>
#include <QVariant>
#include <QMenu>
#include <QAction>

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
    ,ui(new Ui::MusicPlayer)
    ,m_listModel(new QStandardItemModel(this))
    ,m_player(new QMediaPlayer(this))
{
    ui->setupUi(this);
    ui->listView->setModel(m_listModel);

    connect(m_player, &QMediaPlayer::mediaStatusChanged,
            this, &MusicPlayer::onMediaStatusChanged);
    connect(m_player, &QMediaPlayer::metaDataAvailableChanged,
            this, &MusicPlayer::onMetaDataAvailable);
    connect(m_player, &QMediaPlayer::durationChanged,
            this, &MusicPlayer::onDurationChanged);
    connect(m_player, &QMediaPlayer::positionChanged,
            this, &MusicPlayer::onPositionChanged);
    connect(m_player, &QMediaPlayer::stateChanged,
            this, &MusicPlayer::onMediaPlayerStateChanged);

    loadPlayerList();
    m_player->setVolume(50);
    applyTheme(Heal);
}

MusicPlayer::~MusicPlayer()
{   savePlayerList();
    delete ui;
}


void MusicPlayer::on_opendirbttn_clicked()
{
    auto path = QFileDialog::getExistingDirectory(this, "选择文件夹");
    if (path.isEmpty()) {
        return;
    }

    m_listModel->clear();  // 清空旧列表

    QDirIterator it(path, {"*.mp3", "*.wav", "*.ogg","*.flac"}, QDir::Files);

    while (it.hasNext()) {
        it.next();                       // 先移动到下一项
        QFileInfo info = it.fileInfo();   // 再取当前文件信息

        // 列表显示：不带后缀的歌名
        QStandardItem *item = new QStandardItem(info.completeBaseName());
        item->setData(info.absoluteFilePath(), Qt::UserRole);  // 保存完整路径

        m_listModel->appendRow(item);
    }
}

void MusicPlayer::on_openfilebttn_clicked()
{
    auto paths = QFileDialog::getOpenFileNames(this,"选择文件","", "音频文件 (*.mp3 *.wav *.flac)");
    if(paths.isEmpty()){
        return;
    }
    for (const QString &path : paths) {
        QFileInfo info(path);
        QStandardItem *item = new QStandardItem(info.completeBaseName());
        item->setData(path, Qt::UserRole);
        m_listModel->appendRow(item);
    }
}

void MusicPlayer::on_playbttn_clicked()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
        ui->playbttn->setText("▶");
    } else {
        m_player->play();
        ui->playbttn->setText("■");
    }
}


void MusicPlayer::on_listView_doubleClicked(const QModelIndex &index)
{
    playAtIndex(index.row());
}

//记忆化功能

QString MusicPlayer::playerListFilePath() const{
     return QCoreApplication::applicationDirPath() + "/playlist.txt";
}

void MusicPlayer::savePlayerList(){
    QFile file(playerListFilePath());
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        return;
    }//无效文件
    QTextStream out(&file);
    for (int i = 0; i < m_listModel->rowCount(); ++i) {
        QString path = m_listModel->data(m_listModel->index(i, 0), Qt::UserRole).toString();
        out << path << "\n";   // 每行一个文件路径
    }
    file.close();
}

void MusicPlayer::loadPlayerList(){
    QFile file(playerListFilePath());
    if (!file.exists() || !file.open(QIODevice::ReadOnly | QIODevice::Text)) {
        return;   // 第一次运行，没有保存文件，直接返回
    }
    QTextStream in(&file);
    while(!in.atEnd()){
        QString path = in.readLine().trimmed();
        if(path.isEmpty()){
            continue;
        }

        QFileInfo info(path);
        if(!info.exists()){
            continue;
        }

        QStandardItem *item = new QStandardItem(info.completeBaseName());
        item->setData(path,Qt::UserRole);
        m_listModel->appendRow(item);
    }
    file.close();
}
//上一曲、下一曲
int MusicPlayer::currentPlayingIndex() const//寻找目前正在播放的歌曲的索引
{
    QString currentPath = m_player->currentMedia().request().url().toLocalFile();

    for (int i = 0; i < m_listModel->rowCount(); ++i) {
        QString path = m_listModel->data(m_listModel->index(i, 0), Qt::UserRole).toString();
        if (path == currentPath) {
            return i;
        }
    }
    return -1;  // 没找到
}

void MusicPlayer::playAtIndex(int index){
    if(index < 0 || index >= m_listModel->rowCount()){
        return;
    }

    QString path = m_listModel->data(m_listModel->index(index, 0), Qt::UserRole).toString();
    if(path.isEmpty()){
        return;
    }

    m_player->setMedia(QUrl::fromLocalFile(path));
    m_player->play();

    m_currentIndex = index;
    m_currentPath = path;

}

void MusicPlayer::on_prevbttn_clicked()
{
    if(m_listModel->rowCount() == 0){
        return;
    }

    int prev_Index = m_currentIndex - 1;
    if(prev_Index < 0){
        prev_Index = m_listModel->rowCount() - 1;//从第一首回到最后一首
    }

    playAtIndex(prev_Index);
}


void MusicPlayer::on_nextbttn_clicked()
{
    if(m_listModel->rowCount() == 0){
        return;
    }

    int next_Index = m_currentIndex + 1;
    if(next_Index > m_listModel->rowCount() - 1){
        next_Index = 0;
    }

    playAtIndex(next_Index);
}


//音量条
void MusicPlayer::on_horizontalSlider_valueChanged(int value)
{
    m_player->setVolume(value);
}

void MusicPlayer::onMediaStatusChanged(QMediaPlayer::MediaStatus status){
    if (status != QMediaPlayer::EndOfMedia) {
        return;
    }

    if (m_playMode == SingleLoop) {
        // 单曲循环：从头重播当前这首歌
        m_player->setPosition(0);
        m_player->play();
    }
    else if (m_playMode == Random) {
        // 随机播放：随机选一首
        int count = m_listModel->rowCount();
        if (count == 0) return;

        int randomIndex = QRandomGenerator::global()->bounded(count);
        playAtIndex(randomIndex);
    }
    else {
        // 列表循环：播放下一首，到末尾就回到第一首
        int count = m_listModel->rowCount();
        if (count == 0) return;

        int nextIndex = m_currentIndex + 1;
        if (nextIndex >= count) {
            nextIndex = 0;
        }
        playAtIndex(nextIndex);
    }
}

void MusicPlayer::on_loopbttn_clicked()
{
    // 切换到下一种模式（与 mainwindow 风格一致）
    if (m_playMode == ListLoop) {
        m_playMode = SingleLoop;
        ui->loopbttn->setText("🔂 单曲循环");
    }
    else if (m_playMode == SingleLoop) {
        m_playMode = Random;
        ui->loopbttn->setText("🔀 随机播放");
    }
    else {
        m_playMode = ListLoop;
        ui->loopbttn->setText("🔁 列表循环");
    }
}



void MusicPlayer::on_deletebttn_clicked()
{
    QModelIndex current = ui->listView->currentIndex();
    if (!current.isValid()) {
        return;   // 没有选中任何歌曲
    }

    int row = current.row();

    // 如果删除的是当前正在播放的歌，先停止
    if (row == m_currentIndex) {
        m_player->stop();
        m_currentIndex = -1;
    }
    else if (row < m_currentIndex) {
        m_currentIndex--;   // 删除位置在当前歌前面，当前索引要前移
    }

    m_listModel->removeRow(row);
}



void MusicPlayer::onMetaDataAvailable(bool available)
{
    if (!available) return;

    QString title = m_player->metaData(QMediaMetaData::Title).toString();
    QString artist = m_player->metaData(QMediaMetaData::Author).toString();
    QString album = m_player->metaData(QMediaMetaData::AlbumTitle).toString();

    // 如果元数据为空，用文件名
    if (title.isEmpty()) {
        title = QFileInfo(m_currentPath).completeBaseName();
    }
    if (artist.isEmpty()) {
        artist = "未知艺术家";
    }

    ui->label_3->setText(title + " - " + artist);
//找封面
 /*歌曲加载 → metaDataAvailableChanged(true)
    ↓
依次尝试 CoverArtImage / ThumbnailImage / "CoverArtImage"
    ↓
找到了 → 判断是 QImage 还是 QByteArray → 取出图片 → 显示
    ↓
没找到 → loadCoverFromFolder() 去文件夹找封面
*/
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
               ui->coverLabel->setPixmap(pixmap.scaled(
                   ui->coverLabel->size(),
                   Qt::KeepAspectRatio,
                   Qt::SmoothTransformation));
               return;
           }
       }
    // 没找到内嵌封面，去文件夹找
    loadCoverFromFolder();
}

void MusicPlayer::loadCoverFromFolder()
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
                ui->coverLabel->setPixmap(pixmap.scaled(
                    ui->coverLabel->size(),
                    Qt::KeepAspectRatio,
                    Qt::SmoothTransformation));
                return;
            }
        }
    }

}


void MusicPlayer::on_progressSlider_sliderMoved(int position)
{
    m_player->setPosition(position);
}
//显示歌曲播放时长和总时长
void MusicPlayer::onDurationChanged(qint64 duration){
    ui->label->setText("总时长 " + formatTime(duration));
    ui->progressSlider->setMaximum(static_cast<int>(duration));
}

void MusicPlayer::onPositionChanged(qint64 position)
{
    ui->label_2->setText("播放时长 " + formatTime(position));

    // 更新进度条，但不触发 sliderMoved 信号
    ui->progressSlider->blockSignals(true);
    ui->progressSlider->setValue(static_cast<int>(position));
    ui->progressSlider->blockSignals(false);
}

QString MusicPlayer::formatTime(qint64 ms) const
{
    int seconds = ms / 1000;
    int minutes = seconds / 60;
    seconds = seconds % 60;

    return QString("%1:%2")
        .arg(minutes, 2, 10, QChar('0'))
        .arg(seconds, 2, 10, QChar('0'));
}

void MusicPlayer::on_mutebttn_clicked()
{
    if(m_player->isMuted()){
        m_player->setMuted(false);
        ui->mutebttn->setText("静音");
    }
    else {
            m_player->setMuted(true);
            ui->mutebttn->setText("取消静音");
        }
}

void MusicPlayer::onMediaPlayerStateChanged(QMediaPlayer::State state)
{
    if (state == QMediaPlayer::PlayingState) {
        ui->playbttn->setText("■");
    } else if (state == QMediaPlayer::PausedState) {
        ui->playbttn->setText("▶");
    } else { // StoppedState
        ui->playbttn->setText("▶");
    }
}

// ========== 主题相关（从 mainwindow 移植）==========

void MusicPlayer::on_themebttn_clicked()
{
    showThemeMenu();
}

void MusicPlayer::showThemeMenu()
{
    QMenu menu(this);
    QAction *healAction = menu.addAction("🌿 治愈系（淡绿）");
    QAction *darkAction = menu.addAction("🌙 暗黑系（深色）");

    QAction *chosen = menu.exec(ui->themebttn->mapToGlobal(QPoint(0, ui->themebttn->height())));
    if (chosen == healAction) {
        applyTheme(Heal);
    } else if (chosen == darkAction) {
        applyTheme(Dark);
    }
}

void MusicPlayer::applyTheme(Theme theme)
{
    m_currentTheme = theme;
    setStyleSheet(themeStylesheet(theme));
}

QString MusicPlayer::themeStylesheet(Theme theme) const
{
    if (theme == Heal) {
        return R"(
            /* 主窗口背景：纯色，不突兀（QMainWindow 改为 QWidget 适配本类） */
            QWidget {
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
            QWidget {
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
