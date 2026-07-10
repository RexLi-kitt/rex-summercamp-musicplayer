#include "MusicPlayer.h"
#include "./ui_MusicPlayer.h"
#include<QFileDialog>
#include <QDebug>
#include<QDirIterator>

MusicPlayer::MusicPlayer(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::MusicPlayer)
    ,m_listModel(new QStandardItemModel(this))
    ,m_player(new QMediaPlayer(this))
{
    ui->setupUi(this);
    ui->listView->setModel(m_listModel);


}

MusicPlayer::~MusicPlayer()
{
    delete ui;
}


void MusicPlayer::on_opendirbttn_clicked()
{
    auto path = QFileDialog::getExistingDirectory(this, "选择文件夹", "C:/CloudMusic");
    if (path.isEmpty()) {
        return;
    }

    m_listModel->clear();  // 清空旧列表

    QDirIterator it(path, {"*.mp3", "*.wav", "*.ogg"}, QDir::Files);

    while (it.hasNext()) {
        it.next();                       // 先移动到下一项
        QFileInfo info = it.fileInfo();   // 再取当前文件信息

        // 列表显示：不带后缀的歌名
        QStandardItem *item = new QStandardItem(info.completeBaseName());
        item->setData(info.absoluteFilePath(), Qt::UserRole);  // 保存完整路径

        m_listModel->appendRow(item);
    }
}

void MusicPlayer::on_playbttn_clicked()
{
    if (m_player->state() == QMediaPlayer::PlayingState) {
        m_player->pause();
    } else {
        m_player->play();
    }
}


void MusicPlayer::on_listView_doubleClicked(const QModelIndex &index)
{
    QString filePath = m_listModel->data(index, Qt::UserRole).toString();
    if (filePath.isEmpty()) {
        return;
    }
    m_player->setMedia(QUrl::fromLocalFile(filePath));
    m_player->play();
}
