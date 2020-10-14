#include "spotifyapi.h"


SpotifyAPI::SpotifyAPI(const char* fileName)
{
    replyHandler = new QOAuthHttpServerReplyHandler(8080, this);
    isConnected = false;

    //Read file with user keys data
    if(ReadUserKeys(fileName))
    {
        //Setup connection data
        connectAuth.setReplyHandler(replyHandler);
        connectAuth.setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
        connectAuth.setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));


        connectAuth.setClientIdentifier(clientId);
        connectAuth.setClientIdentifierSharedKey(clientSecret);
        connectAuth.setScope("user-read-private user-top-read playlist-read-private playlist-modify-public playlist-modify-private user-modify-playback-state");

        //Creates connections to Slots functions called after connection and autorization access
        //to spotify server is established
        connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
                &QDesktopServices::openUrl);

        connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::statusChanged,
                this, &SpotifyAPI::AuthStatusChanged);

        connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::granted,
                this, &SpotifyAPI::AccessGranted);

    }

}

/**
Method to read a xml file and set user keys: id and secret
@param: fileName path/name description of then file.
*/
bool SpotifyAPI::ReadUserKeys(const char* fileName)
{
    QFile userFile(fileName);
    QXmlStreamReader xml;

    if(userFile.open(QFile::ReadOnly | QFile::Text))
    {
        xml.setDevice(&userFile);

        if (xml.readNextStartElement())
        {
            if (xml.name() == "user")
            {
                while(xml.readNextStartElement())
                {
                    if(xml.name() == "id")
                        clientId = xml.readElementText();
                    else if(xml.name() == "secret")
                        clientSecret = xml.readElementText();
                }
            }
            else
            {
                qDebug()<<"User keys not set: Problem with Xml file";
                userFile.close();
                return false;
            }

        }
        userFile.close();
        return !(clientId.isEmpty()||clientSecret.isEmpty());

    }
    else
    {
        qDebug() << "User keys not set: File not open"<<userFile.errorString();
        return false;
    }

}

void SpotifyAPI::ConnectToServer()
{
    connectAuth.grant();

}

bool SpotifyAPI::IsConnected()
{
    return isConnected;
}

bool SpotifyAPI::IsProcessingRequest()
{
    return processingRequest;
}

/**
SLOT Method called when authorization status changes.
It sends a message to user interface log editbox with the status.
@param: status object that handle status information.
*/
void SpotifyAPI::AuthStatusChanged(QAbstractOAuth::Status status)
{
    QString s;
    if (status == QAbstractOAuth::Status::Granted)
    {
        s = "granted";
    }

    if (status == QAbstractOAuth::Status::TemporaryCredentialsReceived) {
        s = "temp credentials";
    }

   //Sends message to interface log editbox
   QString text = "Authorization status = " + s;
   emit UpdateOutputTextSignal(text,false);
}

/**
SLOT Method called when signal grant() is called after spotfy server grant access
to client application. This method calls GetUserName(QNetworkReply*) method to retrieve user data.
*/
void SpotifyAPI::AccessGranted()
{
    isConnected=true;

    token = connectAuth.token();

    QUrl u ("https://api.spotify.com/v1/me");

    emit UpdateOutputTextSignal("Client connected to spotfy server",false);

    QString text = "Token = " + token;
    emit UpdateOutputTextSignal(text,false);

    auto reply = connectAuth.get(u);
    connect(reply,&QNetworkReply::finished,[=](){ this->GetUserName(reply);} );

}

/**
SLOT Method called after get user name request returns.
@param: network_reply reply object with spotify user data in Json format.
*/
void SpotifyAPI::GetUserName(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        qDebug()<<"Not able to get user data"<<endl;
        return;
    }
    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root = document.object();
    userName = root.value("id").toString();

    QString text = "Client username = " + userName;
    emit UpdateOutputTextSignal(text,false);

    emit ConnectedSignal();

    GetCurrentPlaylists();

    network_reply->deleteLater(); 
}


void SpotifyAPI::GetCurrentPlaylists()
{
    QUrl url ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    auto reply = connectAuth.get(url);

    connect(reply,&QNetworkReply::finished,[=](){ this->GetCurrentPlaylistsReply(reply);} );

}

/**
SLOT Method called after a request to get user current playlists returns.
This method saves the href of each playlist to perform new requests to get playlists full
data (tracks and artists).
@param: network_reply reply object with data in Json format.
*/
void SpotifyAPI::GetCurrentPlaylistsReply(QNetworkReply *network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to get list of current playlists"<<endl;
        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    const auto items_array = root_obj.value("items").toArray();

    //copy user playlists reply data in Json format
    userPlaylistsJson = root_obj;

    userPlaylistsArray.clear();
    userPlaylistsFullJson.clear();

    userPlaylistsArray.resize(ulong(items_array.size()));
    userPlaylistsFullJson.resize(ulong(items_array.size()));

    for(int i=0; i<items_array.size(); i++)
    {
        const auto play_obj = items_array[i].toObject();
        const auto tracks_obj = play_obj.value("tracks").toObject();

        userPlaylistsArray[i].SetHref(tracks_obj.value("href").toString().toStdString());
    }

    GetPlaylistsTracks();

}

/**
Method to request individual playlists data including tracks and artists to spotify server.
The data received is saved in Json objects.
*/
void SpotifyAPI::GetPlaylistsTracks()
{
    for (int i=0; i<userPlaylistsArray.size(); i++)
    {
        SpotifyPlaylist playlist_current = userPlaylistsArray[i];

        QUrl u (playlist_current.GetHref().c_str());

        //Request playlist full data
        auto reply = connectAuth.get(u);

        connect(reply,&QNetworkReply::finished,[=](){ this->GetPlaylistsTracksReply(reply, i);} );

        while(!reply->isFinished())
        {
            QCoreApplication::processEvents();
        }
    }

    SavePlaylistsJsonFromWeb("playlistsonline.json");

}

void SpotifyAPI::GetPlaylistsTracksReply(QNetworkReply *network_reply, int indice)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to  tracks data"<<endl;
        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();
    if(indice < userPlaylistsFullJson.size())
        userPlaylistsFullJson[indice] = root_obj;

    network_reply->deleteLater();
}

bool SpotifyAPI::copyJsonData(QStringList jsonHeaders, QJsonObject &destData, QJsonObject sourceData)
{
    int n = jsonHeaders.count();
    for(int i=0; i < n;i++)
    {
        if(!sourceData.contains(jsonHeaders[i]))
        {
            qDebug()<<"header not found in source json object"<<endl;
            return 0;
        }
        destData.insert(jsonHeaders[i],sourceData.value(jsonHeaders[i]).toString());
    }
    return 1;

}

/**
Method to get the JsonObjects received from a client request to server of current playlists data and
save data in file (.json format).
@param fileName path and name of file where data will be saved.
*/
bool SpotifyAPI::SavePlaylistsJsonFromWeb(QString fileName)
{
    QJsonDocument playlistsJsonDoc;
    QJsonObject root_obj;
    root_obj.insert("info", QJsonValue::fromVariant("Document of Spotify playlists in JSOn format"));
    QJsonArray playlistTargetArray;

    const auto playlistsReplyArray = userPlaylistsJson.value("items").toArray();
    QStringList headerList = {"name","id","href","uri"};

    //Get each playlist item basic data
    for(int i=0; i < playlistsReplyArray.size();i++)
    {
        QJsonObject playlistTarget;
        copyJsonData(headerList,playlistTarget,playlistsReplyArray[i].toObject());

        QJsonArray TracksTargetArray;
        const auto tracksReplyArray = userPlaylistsFullJson[i].value("items").toArray();

        //Get each track data of the parent playlist item
        for (int j=0; j<tracksReplyArray.size(); j++)
        {
            const auto trackReply = tracksReplyArray[j].toObject().value("track").toObject();
            QJsonObject trackTarget;

            if(!copyJsonData(headerList,trackTarget,trackReply))
                return 0;

            QJsonArray artistTargetArray;
            const auto artistReplyArray = trackReply.value("artists").toArray();

            //Get each artist data of the parent track item
            for(int k=0; k<artistReplyArray.size(); k++)
            {
                const auto artistReplyItem = artistReplyArray[k].toObject();
                QJsonObject artistTarget;

                if(!copyJsonData(headerList,artistTarget,artistReplyItem))
                    return 0;

                artistTargetArray.append(artistTarget);
            }
            trackTarget.insert("artists",artistTargetArray);
            TracksTargetArray.append(trackTarget);
        }
        playlistTarget.insert("tracks",TracksTargetArray);
        playlistTargetArray.append(playlistTarget);
    }

    root_obj.insert("playlists",playlistTargetArray);

    playlistsJsonDoc.setObject(root_obj);

    QFile saveFile(fileName);

    if (!saveFile.open(QIODevice::WriteOnly)) {
        qWarning("Couldn't open save file.");
        return 0;
    }

    saveFile.write(playlistsJsonDoc.toJson());
    return 1;
}


/**
Method to for searching a track on spotify server by name. After the request is executed
the reply data is processed in SearchTrackReply(QNetworkReply) method.
@param trackName name of track to search.
*/
void SpotifyAPI::SearchTrack(QString trackName)
{
    QString url_search = "https://api.spotify.com/v1/search?";

    QString query = "q=";

    trackName.replace(" ", "%20");
    query+=trackName;

    query +="&type=track&limit=5";

    QString  result = url_search + query;

    cout<<"URL search = "<<result.toStdString()<<endl;
    QUrl query_url(result);

    QNetworkReply * reply = connectAuth.get(query_url);

    connect(reply,&QNetworkReply::finished,[=](){ this->SearchTrackReply(reply);} );
}

void SpotifyAPI::SearchTrackReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to get tracks information"<<endl;
        return;
    }

    const auto data = network_reply->readAll();
    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    if(!root_obj.contains("tracks"))
    {
        qDebug()<<"Tracks not found"<<endl;
        return;
    }

    //Create a target root json object so store all data tracks received
    QJsonObject targeRoot;

    QJsonArray targetTracksArray;
    const auto tracksReplyFull = root_obj.value("tracks").toObject();
    if(!tracksReplyFull.contains("items"))
    {
        qDebug()<<"Tracks not found"<<endl;
        return;
    }

    QStringList headers = {"name","id","href","uri"};

    const auto tracksReplyArray = tracksReplyFull.value("items").toArray();

    //Copy tracks data from reply to target json objects
    for(int i=0; i<tracksReplyArray.size();i++)
    {
        const auto trackReplyItem = tracksReplyArray[i].toObject();
        QJsonObject targetTrackItem;

        if(!copyJsonData(headers,targetTrackItem,trackReplyItem))
            return;

        if(!trackReplyItem.contains("artists"))
        {
            qDebug()<<"Tracks search error: Incomplete artist data received"<<endl;
            return;
        }

        const auto artistsReplyArray =  trackReplyItem.value("artists").toArray();
        QJsonArray targetArtistsArray;

        // For each Track item received copy artists data from reply to target json objects
        for(int j=0;j<artistsReplyArray.size();j++)
        {
            QJsonObject targetArtistItem;
            const auto artistReplyItem = artistsReplyArray[j].toObject();

            if(!copyJsonData(headers,targetArtistItem,artistReplyItem))
                return;

            targetArtistsArray.append(targetArtistItem);
        }

        targetTrackItem.insert("artists",targetArtistsArray);
        targetTracksArray.append(targetTrackItem);
    }
    targeRoot.insert("tracks",targetTracksArray);

    //Send data to interface
    emit TracksFoundSignal(targeRoot);

   QString text = "Tracks found : " + QString::number(tracksReplyArray.count());

    emit UpdateOutputTextSignal(text,false);

    network_reply->deleteLater();

}

/**
Method for requesting to playback a track or playlist based on uri identifier.
Request performed through PUT method.
@param uri uir address of a playlist or track.
*/
void SpotifyAPI::PlayTracks(QString uri)
{
    QString url_play = "https://api.spotify.com/v1/me/player/play";

    QString query = "q=";

    QJsonObject json_obj;
    QJsonDocument json_doc(json_obj);

    //Create a Json object of the playlist or track uri to be played to send in the PUT request
    json_obj.insert("context_uri",uri);

    auto network_manager = connectAuth.networkAccessManager();

    QNetworkRequest request(url_play);

    QByteArray code;

    //Setting request headers
    QString auth_header("Bearer ");
    code.append(auth_header + connectAuth.token());
    request.setRawHeader("Authorization", code);

    auto reply = network_manager->put(request,json_doc.toBinaryData());
    connect(reply,&QNetworkReply::finished,[=](){ this->PlayTracksReply(reply);} );
}

void SpotifyAPI::PlayTracksReply(QNetworkReply *network_reply)
{
    const auto data = network_reply->readAll();
    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    if (network_reply->error() != QNetworkReply::NoError) {
        QString text = "Unable to play tracks: ";
        text += data.toStdString().c_str();

        emit UpdateOutputTextSignal(text,false);
        return;
    }

    network_reply->deleteLater();
}

void SpotifyAPI::SearchArtist(QString artistName)
{
    QString url = "https://api.spotify.com/v1/search?";

    QString query = "q=";

    artistName.replace(" ", "%20");

    query +="&type=artist&limit=5";

    QString result = url + query;

    QUrl queryUrl(result);

    QNetworkReply * reply = connectAuth.get(queryUrl);

    connect(reply,&QNetworkReply::finished,[=](){ this->SearchArtistReply(reply);} );
}

void SpotifyAPI::SearchArtistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to get artist data"<<endl;
        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    const auto artists_obj = root_obj["artists"].toObject();
    const auto items_array = artists_obj["items"].toArray();

    QJsonObject artist_chosen_obj;
    if(items_array.size()>0)
    {
        for (int item_index = 0; item_index < items_array.size(); ++item_index)
            QJsonObject item_obj = items_array[item_index].toObject();

        artist_chosen_obj = items_array[0].toObject();

        QString artistId = artist_chosen_obj.value("id").toString();
        QString artistName = artist_chosen_obj.value("name").toString();

        network_reply->deleteLater();

        QString text = "Artist found = " + artistName;
        emit UpdateOutputTextSignal(text,false);

        QString text2 = "Artist ID = " + artistId;
        emit UpdateOutputTextSignal(text2,false);

        SearchTopTracks(artistId);
    }
    else
    {
        emit UpdateOutputTextSignal("Error: Artist NOT found",false);
        return;
    }
    network_reply->deleteLater();

}

/**
Method for requesting a search of top tracks of a given artist through its ID identifier.
@param artist_id spotify ID identifier.
*/
void SpotifyAPI::SearchTopTracks(QString artist_id)
{
    playlist.ClearPlaylist();

    string url_tracks1 = "https://api.spotify.com/v1/artists/";
    string url_tracks2 = "/top-tracks?market=BR";

    string artist_id_str = artist_id.toStdString();

    string result =  url_tracks1 + artist_id_str + url_tracks2;
    cout<<"query trancks of artist = "<<result<<endl<<endl;

    QUrl query_url(result.c_str());

    auto reply = connectAuth.get(query_url);

    connect(reply,&QNetworkReply::finished,[=](){ this->SearchTopTracksReply(reply);} );

}

void SpotifyAPI::SearchTopTracksReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to retrieve top tracks data"<<endl;
        return;
    }

    const auto data = network_reply->readAll();
    cout<<"Top tracks found data = "<<data.toStdString()<<endl;

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();
    const auto tracks_array_obj = root_obj["tracks"].toArray();

    if(tracks_array_obj.size()>0)
    {
        for (int track_index = 0; track_index < tracks_array_obj.size(); ++track_index) {

            QJsonObject track_obj = tracks_array_obj[track_index].toObject();

            QString trackname = track_obj.value("name").toString();
            QString trackid = track_obj.value("id").toString();
            QString trackuri = track_obj.value("uri").toString();

            SpotifyTrack track(trackname.toStdString(),trackid.toStdString(),trackuri.toStdString());
            playlist.AddTrack(track);
        }

        emit ArtistTracksFoundSignal();
    }
    network_reply->deleteLater();
}


/**
*Method for requesting to create a playlist on spotify server.
*The request is performed through POST method.
*@param playlist_name name of spotify playlist to be created
*@param is_public param to set if the playlist will be public or private
*@param description Description of playlist
*/
void SpotifyAPI::CreatePlaylistWeb(QString playlist_name, bool is_public, QString description)
{

    string query = "https://api.spotify.com/v1/users/" + userName.toStdString() + "/playlists";
    cout<<"query create playlist= "<<query<<endl<<endl;

    QUrl query_url(query.c_str());

    //Set playlist data to send in request
    QJsonObject playlist_obj;
    playlist_obj.insert("name", QJsonValue::fromVariant(playlist_name));
    playlist_obj.insert("description", QJsonValue::fromVariant(description));
    playlist_obj.insert("public", QJsonValue::fromVariant(is_public));

    QJsonDocument playlist_doc(playlist_obj);

    auto network_manager = connectAuth.networkAccessManager();

    //Set headers of request
    QNetworkRequest request(query_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray code;
    code.append("application/json");
    request.setRawHeader("Accept",code);

    QString auth_header("Bearer ");

    code.clear();
    code.append(auth_header + connectAuth.token());
    request.setRawHeader("Authorization", code);

    // Send request
    auto reply = network_manager->post(request, playlist_doc.toJson());
    connect(reply,&QNetworkReply::finished,[=](){ this->CreatePlaylistReply(reply);} );

}

void SpotifyAPI::CreatePlaylistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        qDebug()<<"Unable criate playlist"<<endl;
        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();
    QString name = root_obj.value("name").toString();
    QString id = root_obj.value("id").toString();
    QString uri = root_obj.value("uri").toString();


    QString text = "Playlist created info = \n";
    text+= "name: " + name + "\n";
    text+= "id: " + id + "\n";
    text+= "uri: " + uri + "\n";

    playlist.SetId(id.toStdString());
    playlist.SetName(name.toStdString());
    playlist.SetURI(uri.toStdString());

    emit UpdateOutputTextSignal(text,false);

    network_reply->deleteLater();

}

/**
Method for requesting to add tracks to a given playlist on spotify server.
This method requires that a class of SpotifyPlaylist be set.
*/
void SpotifyAPI::AddTracksPlaylistWeb()
{
    if(!playlist.GetId().empty())
    {
        string query = "https://api.spotify.com/v1/playlists/" + playlist.GetId() + "/tracks?";

        //Set tracks uri's chosen to be inserted in playlist
        string tracks_query = "uris=" + playlist.GetTracksUrisListStr(",");
        query += tracks_query;

        QUrl query_url(query.c_str());

        auto network_manager = connectAuth.networkAccessManager();

        //Set request data header
        QNetworkRequest request(query_url);
        request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

        QByteArray code;
        code.append("application/json");
        request.setRawHeader("Accept",code);

        QString auth_header("Bearer ");

        code.clear();
        code.append(auth_header + connectAuth.token());
        request.setRawHeader("Authorization", code);

        // Send request
        auto reply = network_manager->post(request, QByteArray());
        connect(reply,&QNetworkReply::finished,[=](){ this->AddTracksPlaylistReply(reply);} );
    }
}

void SpotifyAPI::AddTracksPlaylistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        qDebug()<<"Unable add tracks to playlist"<<endl;
        const auto data = network_reply->readAll();
        qDebug()<<"Error body data ="<<data<<endl;

        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    QString text = "Tracks added succesfully";
    text += "snapshot_id = " + root_obj.value("snapshot_id").toString();
    emit UpdateOutputTextSignal(text,false);

    network_reply->deleteLater();

}

SpotifyAPI::~SpotifyAPI()
{
    delete replyHandler;
}
