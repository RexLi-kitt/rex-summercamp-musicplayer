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

    enum PlayMode{
        ListLoop,
        SingleLoop,
        Random
    };

private slots:
    void on_opendirbttn_clicked();
    void on_playbttn_clicked();
    void on_listView_doubleClicked(const QModelIndex &index);
    void on_random_play_bttn_clicked();
    void on_prevbttn_clicked();

    void on_nextbttn_clicked();

    void on_horizontalSlider_valueChanged(int value);

    void on_openfilebttn_clicked();

    void onMediaStatusChanged(QMediaPlayer::MediaStatus status);

    void on_loopbttn_clicked();

    void on_deletebttn_clicked();

    void onMetaDataAvailable(bool available);

    void onDurationChanged(qint64 duration);

    void onPositionChanged(qint64 position);

    void on_progressSlider_sliderMoved(int position);






    void on_mutebttn_clicked();

private:
    Ui::MusicPlayer *ui;
    QStandardItemModel *m_listModel;  //m_listModel = 存放数据的"仓库"
    QMediaPlayer *m_player;
    int m_currentIndex = -1;
    QString m_currentPath;
    PlayMode m_playMode = ListLoop;
    QString formatTime(qint64 ms) const;

    void loadCoverFromFolder();

private:
    void savePlayerList();
    void loadPlayerList();
    QString playerListFilePath() const;
    int currentPlayingIndex() const;
    void playAtIndex(int index);
};

#endif // MUSICPLAYER_H
