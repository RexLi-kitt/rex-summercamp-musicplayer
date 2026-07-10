#ifndef MUSICPLAYER_H
#define MUSICPLAYER_H

#include <QStandardItemModel>
#include <QWidget>
#include <QMediaPlayer>

QT_BEGIN_NAMESPACE
namespace Ui { class MusicPlayer; }
QT_END_NAMESPACE

class MusicPlayer : public QWidget
{
    Q_OBJECT

public:
    MusicPlayer(QWidget *parent = nullptr);
    ~MusicPlayer();

private slots:
    void on_opendirbttn_clicked();
    void on_playbttn_clicked();
    void on_listView_doubleClicked(const QModelIndex &index);

private:
    Ui::MusicPlayer *ui;
    QStandardItemModel *m_listModel;
    QMediaPlayer *m_player;
};

#endif // MUSICPLAYER_H
