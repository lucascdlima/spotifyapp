#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <QMainWindow>
#include <QtUiTools>
#include "spotifyapi.h"
#include "treemodel.h"

QT_BEGIN_NAMESPACE
namespace Ui { class MainWindow; }
QT_END_NAMESPACE

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

    void  SetTracksView();
    void SetPlayListView();

private slots:

    //Slot methods called after user interface operation occurs
    void ConnectSpotifyClicked();
    void GetArtistTracksSlot();
    void CreatePlaylistSlot();
    void GetPlaylistsSlot();

    //Slot methods called after a SpotifyAPI object operation return
    void UpdateOutputTextSlot(SpotifyAPI *spotfy_sender);
    void ConnectGrantedSlot();
    void ArtistTracksFoundSlot();

    void PlaylistSelected(const QModelIndex & index);

private:
    Ui::MainWindow *ui;

    QTreeView *playlistsView;
    QTreeView *tracksView;

    TreeModel *playlistModel;

    //Spotfy handle to perform server requests and queries
    SpotifyAPI *spotify;
};
#endif // MAINWINDOW_H
