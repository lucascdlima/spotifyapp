#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->searchBt->setEnabled(false);
    ui->createPlaylistBt->setEnabled(true);
    ui->addTrackBt->setEnabled(false);

    const QStringList headers({tr("name"),tr("id"),tr("uri"),tr("href")});

    playlistModel = new TreeModel(headers, "");
    playlistModel->setupModelData("playlistsdataoffline2.json");

    playlistsView = new QTreeView();
    playlistsView->setModel(playlistModel);
    tracksView = new QTreeView();
    tracksView->setModel(playlistModel);

    searchResultView = new QTreeView();
    ui->resultVLayout->addWidget(searchResultView);

    ui->artistRBt->setChecked(false);
    ui->musicRBt->setChecked(true);

    ui->playGboxVLayout->addWidget(playlistsView);
    ui->tracksVLayout->addWidget(tracksView);

    SetPlayListView();
    SetTracksView();

    //Create SpotfyAPI object to handle connections, queries, replies
    spotify = new SpotifyAPI("userkeys.xml");

    //Create connections of SIGNALS (actions) in SpotifyAPI to SLOTS (actions) in interface
    connect(spotify,&SpotifyAPI::UpdateOutputTextSignal,[=](){ this->UpdateOutputTextSlot(spotify);} );
    connect(spotify,&SpotifyAPI::ConnectedSignal,[=](){ this->ConnectGrantedSlot();} );
    connect(spotify,&SpotifyAPI::ArtistTracksFoundSignal,[=](){ this->ArtistTracksFoundSlot();} );
    connect(spotify,&SpotifyAPI::TracksFoundSignal,this, &MainWindow::TracksFoundSlot);


    //Create connections of user interface actions to internal computations
    connect(ui->connectBt, SIGNAL (clicked()), this, SLOT (ConnectSpotifyClicked()));
    connect(ui->searchBt, SIGNAL (clicked()), this, SLOT (SearchClickedSlot()));
    connect(ui->createPlaylistBt, SIGNAL (clicked()), this, SLOT (CreatePlaylistSlot()));
    connect(ui->removeTrackBt, SIGNAL(clicked()), this, SLOT(RemoveTrack()));
    connect(playlistsView, SIGNAL(pressed(const QModelIndex &)), this, SLOT(PlaylistSelected(const QModelIndex &)));
    connect(ui->addTrackBt, SIGNAL(clicked()), this, SLOT(AddTrack()));

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
        if(!playlistModel->insertRow(playlistModel->rowCount()))
        {
            qDebug()<<"Playslist not created: problem during child/row insertion"<<endl;
            return;
        }
        int i = playlistModel->rowCount()-1;
        const auto newPlaylistIndex = playlistModel->index(i,0);

        if(!playlistModel->hasIndex(i,0))
        {
            qDebug()<<"Playslist not created: Index invalid"<<endl;
            return;
        }
        playlistModel->setData(newPlaylistIndex,playlist_name);

        //Request to spotfy server criation of playlist with tracks
        //spotify->CreatePlaylistWeb(playlist_name.toStdString(),false,"Playlist Created with C++ app using Spotify API");
    }
    else
    {
        QMessageBox::warning(this, tr(""),tr("Invalid playlist name"));
        return;
    }
}

void  MainWindow::SetTracksView()
{
   int playlist_data_count =  tracksView->model()->columnCount();

    for(int i = 1;  i < playlist_data_count ; i++)
            tracksView->setColumnHidden(i,true);
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

void MainWindow::RemoveTrack()
{
    const auto model = tracksView->model();
    if(model->parent(tracksView->currentIndex()) == QModelIndex())
    {
        qDebug()<<"Playlist chosen: Not allowed to remove"<<endl;
        return;
    }
    const auto track_index = tracksView->currentIndex();

    if(model->hasIndex(track_index.row(),0,track_index.parent()))
    {
        model->removeRow(track_index.row(),track_index.parent());
    }
}

void MainWindow::SearchClickedSlot()
{
    ui->addTrackBt->setEnabled(false);
    if(ui->artistRBt->isChecked())
    {

    }
    else if(ui->musicRBt->isChecked())
    {
        //spotify->SearchArtist("tim maia");
        QString track_name = ui->searchTxEdit->toPlainText();
        spotify->SearchTrack(track_name);

    }

}

void MainWindow::TracksFoundSlot(QJsonObject data)
{
    const QStringList headers({tr("name"),tr("id"),tr("uri"),tr("href")});

    TreeModel * tracksSearchModel = new TreeModel(headers, "");
    tracksSearchModel->setupModelFromJson(data,MODEL_TYPE_TRACK);

    QItemSelectionModel *m = searchResultView->selectionModel();
    searchResultView->setModel(tracksSearchModel);
    for(int j=0; j<tracksSearchModel->columnCount(); j++)
    {
        const auto dataIndex = tracksSearchModel->index(0,j);
        if(!(tracksSearchModel->headData(dataIndex) == QString("name") || tracksSearchModel->headData(dataIndex) == QString("id")))
            searchResultView->setColumnHidden(j,true);
    }

    ui->addTrackBt->setEnabled(true);

    delete m;

}

void MainWindow::AddTrack()
{
    if(!searchResultView->model()->hasChildren(QModelIndex()))
    {
        qDebug()<<"There are no tracks in the search results"<<endl;
        return;
    }
    TreeModel * trackModel = (TreeModel*)(searchResultView->model());
    if(trackModel->getModelType()!=MODEL_TYPE_TRACK)
    {
        qDebug()<<"Search results have items different from track"<<endl;
        return;
    }

    if(!searchResultView->currentIndex().isValid())
    {
        qDebug()<<"None track selected for adding to playlist"<<endl;
        return;
    }

    const auto track_index = searchResultView->currentIndex();
    if(trackModel->index(track_index.row(),0) != track_index)
    {
        qDebug()<<"Choose a track add to playlist"<<endl;
        return;
    }

    if(!playlistsView->currentIndex().isValid())
    {
        qDebug()<<"Playlist not selected to add track"<<endl;
        return;
    }

    const auto playlist_index = playlistsView->currentIndex();

    int N_tracks = playlistModel->rowCount(playlist_index);
    if(!playlistModel->insertRow(N_tracks,playlist_index))
    {
        qDebug()<<"It was not possible to add track to playlist Tree Model "<<endl;
        return;
    }

    playlistsView->setRowHidden(playlistModel->rowCount(playlist_index)-1,playlist_index,true);

    int N_data = playlistModel->columnCount(playlistModel->index(N_tracks,0,playlist_index));
    for(int i = 0; i< N_data; i++)
    {
        const auto indexData = playlistModel->index(N_tracks,i,playlist_index);
        playlistModel->setData(indexData,trackModel->data(trackModel->index(track_index.row(),i),Qt::EditRole));
    }

}

void MainWindow::closeEvent(QCloseEvent *event)
{
    playlistModel->saveModelDataOffline("playlistsdataoffline2.json");
    QMainWindow::closeEvent(event);
}

