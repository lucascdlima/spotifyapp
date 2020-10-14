#include "mainwindow.h"
#include "ui_mainwindow.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
{
    ui->setupUi(this);

    ui->searchBt->setEnabled(false);
    ui->addTrackBt->setEnabled(false);
    ui->playBt->setEnabled(false);
    ui->createPlaylistBt->setEnabled(true);

    const QStringList headers({tr("name"),tr("id"),tr("uri"),tr("href"),tr("artist")});

    playlistModel = new TreeModel(headers);
    playlistModel->loadModelData("playlistsdata.json");

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
    connect(spotify,&SpotifyAPI::UpdateOutputTextSignal,this, &MainWindow::UpdateOutputTextSlot);
    connect(spotify,&SpotifyAPI::ConnectedSignal,[=](){ this->ConnectGrantedSlot();} );
    connect(spotify,&SpotifyAPI::ArtistTracksFoundSignal,[=](){ this->ArtistTracksFoundSlot();} );
    connect(spotify,&SpotifyAPI::TracksFoundSignal,this, &MainWindow::TracksFoundSlot);


    //Create connections between user interface actions and internal computations
    connect(ui->connectBt, SIGNAL (clicked()), this, SLOT (ConnectSpotifyClicked()));
    connect(ui->searchBt, SIGNAL (clicked()), this, SLOT (SearchClickedSlot()));
    connect(ui->createPlaylistBt, SIGNAL (clicked()), this, SLOT (CreatePlaylistSlot()));
    connect(ui->removeTrackBt, SIGNAL(clicked()), this, SLOT(RemoveTrack()));
    connect(playlistsView, SIGNAL(pressed(const QModelIndex &)), this, SLOT(PlaylistSelected(const QModelIndex &)));
    connect(ui->addTrackBt, SIGNAL(clicked()), this, SLOT(AddTrack()));
    connect(ui->playBt, SIGNAL(clicked()), this, SLOT(PlayTracks()));

}

MainWindow::~MainWindow()
{
    delete spotify;
    delete ui;
}

/**
*SLOT method called after connect button is clicked.
*It calls spotify API to request connection for Spotify server.
*/
void MainWindow::ConnectSpotifyClicked()
{
    ui->logPTxEdit->appendPlainText("Application requesting connection to spotfy");
    spotify->ConnectToServer();
}

void MainWindow::ConnectGrantedSlot()
{
    ui->searchBt->setEnabled(true);
    ui->playBt->setEnabled(true);
}

void MainWindow::ArtistTracksFoundSlot()
{
    ui->createPlaylistBt->setEnabled(true);
}

/**
*SLOT method called after search artist button clicked.
*It calls spotify API for requesting artist information and correspondent tracks.
*/
void MainWindow::GetArtistTracksSlot()
{

    spotify->GetCurrentPlaylists();
    //Get artist name from interface
    QString artist_name = ui->searchTxEdit->toPlainText();

    if(!artist_name.isEmpty())
    {
        spotify->SearchArtist(artist_name);
    }
    else
        QMessageBox::warning(this, tr(""),tr("Invalid artist name"));

}

/**
*SLOT method called by SpotfyAPI in order to show a message text in the interface.
*@param text message text to be displayed on interface.
*@param clear param to clear edit box
*/
void MainWindow::UpdateOutputTextSlot(QString text, bool clear)
{
     if(!text.isEmpty())
     {
         if(clear)
             ui->logPTxEdit->setPlainText(text);
         else
             ui->logPTxEdit->appendPlainText(text);
     }

}

/**
*SLOT method called after create playlist button clicked.
*It creates a empty playlist in the TreeModel with name data set by the user on
*text edit.
*/
void MainWindow::CreatePlaylistSlot()
{
    //Get playlist name from interface
    QString playlist_name = ui->playlistNameTxEdit->toPlainText();

    if(playlist_name.isEmpty()){
        QMessageBox::warning(this, tr(""),tr("Invalid playlist name"));
        return;
    }

    if(!playlistModel->insertRow(playlistModel->rowCount())){
       ui->logPTxEdit->appendPlainText("Playslist not created: problem during child/row insertion");
       return;
    }

    //Get index number of row inserted
    int i = playlistModel->rowCount()-1;

    //QModelIndex of first element in the row and set name
    const auto newPlaylistIndex = playlistModel->index(i,0);
    playlistModel->setData(newPlaylistIndex,playlist_name);

    //Set head data of all elements indexes in the row created
    QStringList childrenHeader = {"name","id","href","uri"};
    for (int k=0;k<childrenHeader.size();k++)
    {
        if(!playlistModel->hasIndex(i,k)){
            ui->logPTxEdit->appendPlainText("Playslist not created: Index invalid");
            return;
        }
        const auto dataIndex = playlistModel->index(i,k);
        playlistModel->setHeadData(dataIndex,childrenHeader[k]);
    }

    //Added code to create playlist online also.
    spotify->CreatePlaylistWeb(playlist_name,false,"Playlist created by application in C++");
}

/**
*Method to define which rows and colums of the TreeModel data will be displayed in the interface.
*The visualization of data is based on QTreeView class. In this view only the playlist selected and
*respective tracks will be displayed.
*Obs: The TreeModel have a artist column in the tracks children to display the main artist and also
*artist TreeItem childs of each track to store all the artists of a track.
*/
void  MainWindow::SetTracksView()
{
    int playlist_data_count =  tracksView->model()->columnCount();

    for(int i = 0;  i < playlist_data_count ; i++)
    {
        //Only display name and artist columns
        QString header = tracksView->model()->headerData(i,Qt::Orientation::Horizontal).toString();
        bool hide = !(header=="name" || header == "artist");
        tracksView->setColumnHidden(i,hide);
    }

    for(int i=0; i< tracksView->model()->rowCount();i++)
    {
        const auto playlistIndex = tracksView->model()->index(i,0);
        for(int j = 0; j < tracksView->model()->rowCount(playlistIndex); j++)
        {
            const auto trackIndex = tracksView->model()->index(j,0,playlistIndex);

            //Hide artist rows (childs)
            for(int k = 0; k<tracksView->model()->rowCount(trackIndex); k++)
                tracksView->setRowHidden(k,trackIndex,true);
        }
    }

}

/**
*Method to define which rows and colums of the TreeModel data will be displayed in the Playlist view
*part of interface.
*The visualization of data is based on QTreeView class. In this view only the playlists names will be displayed.
*/
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

/**
*Method SLOT called when a playlist item is selected in the playlist view.
*It will display in the tracks View only the playlist selected and respective tracks rows and artist columns data.
*The visualization of data is based on QTreeView class.
*@param index QModelIndex object of the selected item in the playlist view
*/
void MainWindow::PlaylistSelected(const QModelIndex & index)
{
    int playlists_count = tracksView->model()->rowCount();
    if(index.isValid())
    {
        //Hide all playlists rows in the tracks view except the playlist selected (index)
        for(int i=0;i<playlists_count; i++)
        {
            bool hide_row = !(tracksView->model()->index(i,0)==index);
            tracksView->setRowHidden(i,tracksView->model()->parent(QModelIndex()),hide_row);
        }
    }
}

/**
*Method SLOT called when the remove track button is clicked.
*It removes the selected track from the ModelTree.
*/
void MainWindow::RemoveTrack()
{
    const auto model = tracksView->model();

    //Check if a track or playlist was selected in tracks view
    if(model->parent(tracksView->currentIndex()) == QModelIndex())
    {
        ui->logPTxEdit->appendPlainText("Playlist chosen: Not allowed to remove");
        return;
    }
    const auto track_index = tracksView->currentIndex();

    if(model->hasIndex(track_index.row(),0,track_index.parent()))
        model->removeRow(track_index.row(),track_index.parent());

}

/**
*Method SLOT called when the search button is clicked.
*It call SpotifyAPI method SearchTrack() to acess apotify server.
*/
void MainWindow::SearchClickedSlot()
{
    ui->addTrackBt->setEnabled(false);
    if(ui->artistRBt->isChecked())
    {
        //To be implemented
    }
    else if(ui->musicRBt->isChecked())
    {
        QString track_name = ui->searchTxEdit->toPlainText();

        //Execute a query to spotify server with the given track name
        spotify->SearchTrack(track_name);
    }

}

/**
*Method SLOT called after the query executed in SpotifyAPI SearchTrack() method returns.
*It gets the JsonObject data and creates a track model based on TreeModel class and display
*tracks founded in the search result tree view.
*@param data object data received from spotify server reply.
*/
void MainWindow::TracksFoundSlot(QJsonObject data)
{
    const QStringList headers({tr("name"),tr("id"),tr("uri"),tr("href"),tr("artist")});

    //Creates the model to store search results from QJson object
    TreeModel * tracksSearchModel = new TreeModel(headers);
    tracksSearchModel->loadModelData(data,MODEL_TYPE_TRACK);

    QItemSelectionModel *m = searchResultView->selectionModel();

    //Sets a new model to be displayed in the search result view
    searchResultView->setModel(tracksSearchModel);
    for(int j=0; j<tracksSearchModel->columnCount(); j++)
    {
        const auto dataIndex = tracksSearchModel->index(0,j);
        if(!(tracksSearchModel->headData(dataIndex) == QString("name") || tracksSearchModel->headData(dataIndex) == QString("id")))
            searchResultView->setColumnHidden(j,true);
    }

    ui->addTrackBt->setEnabled(true);

    //Delete selection model from older search results
    delete m;

}

/**
*Method SLOT called after add track button is clicked on the interface.
*It gets the index of a track selected in the search results view and copy the data to a track child row
*that is inserted on the playlist TreeItem correspondent to the playlist index selected in the playlist tree view.
*The criation of child row and insertion of data is performed in the playlist TreeModel.
*/
void MainWindow::AddTrack()
{
    if(!searchResultView->model()->hasChildren(QModelIndex()))
    {
        ui->logPTxEdit->appendPlainText("There are no tracks in the search results");
        return;
    }

    TreeModel * trackSearchModel = (TreeModel*)(searchResultView->model());

    //Check if a track is selected
    if(!searchResultView->currentIndex().isValid())
        return;


    const auto selTrackIndex = searchResultView->currentIndex();
    if(trackSearchModel->index(selTrackIndex.row(),0) != selTrackIndex)
    {
        qDebug()<<"Choose a track add to playlist"<<endl;
        return;
    }

    //Check if a playlist is selected
    if(!playlistsView->currentIndex().isValid())
        return;


    const auto playlist_index = playlistsView->currentIndex();

    int N_tracks = playlistModel->rowCount(playlist_index);

    //Insert a new child row (a track) to the current playlist
    if(!playlistModel->insertRow(N_tracks,playlist_index))
    {
        ui->logPTxEdit->appendPlainText("It was not possible to add track to playlist Tree Model ");
        return;
    }

    const auto newTrackindex = playlistModel->index(playlistModel->rowCount(playlist_index)-1,0,playlist_index);

    playlistsView->setRowHidden(playlistModel->rowCount(playlist_index)-1,playlist_index,true);

    int N_data = playlistModel->columnCount(playlistModel->index(N_tracks,0,playlist_index));

    //Copy data from track item selected in the search result view to the new track created in playlist model
    for(int i = 0; i< N_data; i++)
    {
        const auto indexData = playlistModel->index(N_tracks,i,playlist_index);
        playlistModel->setData(indexData,trackSearchModel->data(trackSearchModel->index(selTrackIndex.row(),i),Qt::EditRole));
        playlistModel->setHeadData(indexData,trackSearchModel->headData(trackSearchModel->index(selTrackIndex.row(),i)));
    }

    int N_artists = trackSearchModel->rowCount(selTrackIndex);
    N_data = trackSearchModel->columnCount(trackSearchModel->index(0,0,selTrackIndex));
    for (int j=0; j<N_artists; j++)
    {
        //Insert a new child row (a artist) to each track created in the playlist modol
        if(!playlistModel->insertRow(playlistModel->rowCount(newTrackindex),newTrackindex))
        {
            playlistModel->removeRows(playlistModel->rowCount()-1,1);
            return;
        }

        //Copy artists data from track item selected in the search result view to the new track created in playlist model
        for (int k=0; k<N_data; k++)
        {
            const auto indexData = playlistModel->index(j,k,newTrackindex);
            playlistModel->setData(indexData,trackSearchModel->data(trackSearchModel->index(j,k,selTrackIndex),Qt::EditRole));
            playlistModel->setHeadData(indexData,trackSearchModel->headData(trackSearchModel->index(j,k,selTrackIndex)));
        }
        tracksView->setRowHidden(j,newTrackindex,true);

    }
}

/**
*Method SLOT called after play track button is clicked on the interface to playback a spotify track or playlist.
*It gets the index of a track or playlist selected in the tracks view and calls the SpotifyAPI::PlayTracks(QString)
*passing the address uri of the respective track or playlist.
*/
void MainWindow::PlayTracks()
{
    //Get item selected (track of playlist)
    auto item_index = tracksView->currentIndex();
    if(!item_index.isValid())
        return;

    QVariant data = playlistModel->findDataByHead("uri",item_index);
    if(data.isNull())
        return;

    spotify->PlayTracks(data.toString());
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    //Save user playlists data in Json format
    playlistModel->saveModelDataOffline("playlistsdata.json");
    QMainWindow::closeEvent(event);
}
