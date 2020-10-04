#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->searchBt->setEnabled(false);
    ui->createPlaylistBt->setEnabled(false);

    const QStringList headers({tr("name"),tr("id"),tr("uri"),tr("href")});

    playlistModel = new TreeModel(headers, "");

    playlistsView = new QTreeView();
    playlistsView->setModel(playlistModel);
    tracksView = new QTreeView();
    tracksView->setModel(playlistModel);

    ui->playGboxVLayout->addWidget(playlistsView);
    ui->tracksVLayout->addWidget(tracksView);

    SetPlayListView();
    SetTracksView();

    //Create SpotfyAPI object to handle connections, queries, replies
    spotify = new SpotifyAPI();

    //Create connections of SIGNALS (actions) in SpotifyAPI to SLOTS (actions) in interface
    connect(spotify,&SpotifyAPI::UpdateOutputTextSignal,[=](){ this->UpdateOutputTextSlot(spotify);} );
    connect(spotify,&SpotifyAPI::ConnectedSignal,[=](){ this->ConnectGrantedSlot();} );
    connect(spotify,&SpotifyAPI::ArtistTracksFoundSignal,[=](){ this->ArtistTracksFoundSlot();} );


    //Create connections of user interface actions to internal computations
    connect(ui->connectBt, SIGNAL (clicked()), this, SLOT (ConnectSpotifyClicked()));
    connect(ui->searchBt, SIGNAL (clicked()), this, SLOT (GetArtistTracksSlot()));
    connect(ui->createPlaylistBt, SIGNAL (clicked()), this, SLOT (CreatePlaylistSlot()));

    connect(playlistsView, SIGNAL(pressed(const QModelIndex &)), this, SLOT(PlaylistSelected(const QModelIndex &)));

}

MainWindow::~MainWindow()
{
    delete spotify;
    delete ui;
}

/**
SLOT function called after connect button is clicked.
It calls spotfy API to request connection for Spotify server.
*/
void MainWindow::ConnectSpotifyClicked()
{
    ui->logPTxEdit->appendPlainText("Application requesting connection to spotfy");
    spotify->ConnectToServer();
}

void MainWindow::ConnectGrantedSlot()
{
    ui->searchBt->setEnabled(true);
}

void MainWindow::ArtistTracksFoundSlot()
{
    ui->createPlaylistBt->setEnabled(true);
}

/**
SLOT function called after search artist button clicked.
It calls spotfy API for requesting artist information and correspondent tracks.
*/
void MainWindow::GetArtistTracksSlot()
{

    spotify->GetCurrentPlaylists();
    //Get artist name from interface
    QString artist_name = ui->searchTxEdit->toPlainText();

    if(!artist_name.isEmpty())
    {
        spotify->SearchArtist(artist_name.toStdString());
    }
    else
        QMessageBox::warning(this, tr(""),tr("Invalid artist name"));

}

/**
SLOT function called by SpotfyAPI in order to show a message text in
the interface.
@param spotfy_sender SpotifyAPI object for retrieving messages to be outputed
*/
void MainWindow::UpdateOutputTextSlot(SpotifyAPI *spotfy_sender)
{
    //Get last message to be displayed and remove from SpotifyAPI message list
    OutputMessage message = spotfy_sender->GetLastMessage();

    if(!message.text.isEmpty())
    {
        if(message.clear)
            ui->logPTxEdit->setPlainText(message.text);
        else
            ui->logPTxEdit->appendPlainText(message.text);
    }

}

void MainWindow::GetPlaylistsSlot()
{


}

/**
SLOT function called after create playlist button clicked.
It calls spotfy API for requesting server to create playlist and add tracks
found after search of artist call GetArtistTracksSlot() return.
*/
void MainWindow::CreatePlaylistSlot()
{
    //Get playlist name from interface
    QString playlist_name = ui->playlistNameTxEdit->toPlainText();

    if(!playlist_name.isEmpty())
    {
        //Request to spotfy server criation of playlist with tracks
        spotify->CreatePlaylistWeb(playlist_name.toStdString(),false,"Playlist Created with C++ app using Spotify API");
    }
    else
        QMessageBox::warning(this, tr(""),tr("Invalid playlist name"));
}

void  MainWindow::SetTracksView()
{
    int playlists_count = tracksView->model()->rowCount();
    int playlist_data_count =  tracksView->model()->columnCount();

    for(int i = 1;  i < playlist_data_count ; i++)
    {
        tracksView->setColumnHidden(i,true);
    }



}

void MainWindow::SetPlayListView()
{
    int playlists_count = playlistsView->model()->rowCount();
    int playlist_data_count =  playlistsView->model()->columnCount();

    for(int i = 1;  i < playlist_data_count ; i++)
        playlistsView->setColumnHidden(i,true);

    for(int i = 0;  i < playlists_count ; i++)
    {
        const auto index = playlistsView->model()->index(i,0);
        int tracks_count = playlistsView->model()->rowCount(index);

        for(int j = 0;  j < tracks_count ; j++)
            playlistsView->setRowHidden(j,index,true);
    }
}

void MainWindow::PlaylistSelected(const QModelIndex & index)
{

    int playlists_count = tracksView->model()->rowCount();
    if(index.isValid())
    {
        for(int i=0;i<playlists_count; i++)
        {
            bool hide_row = !(tracksView->model()->index(i,0)==index);
            tracksView->setRowHidden(i,tracksView->model()->parent(QModelIndex()),hide_row);

        }

    }


}


