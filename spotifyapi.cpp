#include "spotifyapi.h"


SpotifyAPI::SpotifyAPI()
{

    replyHandler = new QOAuthHttpServerReplyHandler(8080, this);

    connectAuth.setReplyHandler(replyHandler);
    connectAuth.setAuthorizationUrl(QUrl("https://accounts.spotify.com/authorize"));
    connectAuth.setAccessTokenUrl(QUrl("https://accounts.spotify.com/api/token"));



    connectAuth.setClientIdentifier(clientId);
    connectAuth.setClientIdentifierSharedKey(clientSecret);
    connectAuth.setScope("user-read-private user-top-read playlist-read-private playlist-modify-public playlist-modify-private");

    replyProcessing = 0;

    connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::authorizeWithBrowser,
            &QDesktopServices::openUrl);

    connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::statusChanged,
            this, &SpotifyAPI::AuthStatusChanged);

    connect(&connectAuth, &QOAuth2AuthorizationCodeFlow::granted,
            this, &SpotifyAPI::AccessGranted);


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

    OutputMessage message{"Authorization status = " + s,false};
    messagesArray.push_back(message);
    emit UpdateOutputTextSignal();


}

void SpotifyAPI::AccessGranted()
{
    isConnected=true;

    token = connectAuth.token();

    //cout<<"Token = "<<token.toStdString()<<endl<<endl;
    QUrl u ("https://api.spotify.com/v1/me");

    OutputMessage message{"Client connected to spotfy server",false};
    messagesArray.push_back(message);
    emit UpdateOutputTextSignal();

    OutputMessage message2{"Token = " + token,false};
    messagesArray.push_back(message2);
    emit UpdateOutputTextSignal();

    auto reply = connectAuth.get(u);
    connect(reply,&QNetworkReply::finished,[=](){ this->GetUserName(reply);} );

}

void SpotifyAPI::GetUserName(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Not able to get user info"<<endl;
        return;
    }
    const auto data = network_reply->readAll();

    cout<<"All data = "<<data.toStdString()<<endl;


    const auto document = QJsonDocument::fromJson(data);
    const auto root = document.object();
    userName = root.value("id").toString();

    cout<<"Username = "<<userName.toStdString()<<endl;


    OutputMessage message{"Client username = " + userName,false};
    messagesArray.push_back(message);
    emit UpdateOutputTextSignal();

    emit ConnectedSignal();

    network_reply->deleteLater();

    GetCurrentPlaylists();

}

void SpotifyAPI::GetCurrentPlaylists()
{

    QUrl u ("https://api.spotify.com/v1/users/" + userName + "/playlists");

    replyProcessing+=1;

    auto reply = connectAuth.get(u);

    connect(reply,&QNetworkReply::finished,[=](){ this->GetCurrentPlaylistsReply(reply);} );


}

void SpotifyAPI::GetCurrentPlaylistsReply(QNetworkReply *network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to get list of current playlists"<<endl;
        return;
    }

    const auto data = network_reply->readAll();



    cout<<"current playlists data = "<<data.toStdString()<<endl<<endl;

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();


    const auto items_array = root_obj.value("items").toArray();


    userPlaylistsJson = root_obj;

    userPlaylistsArray.resize(ulong(items_array.size()));
    userPlaylistsTracksJson.resize(ulong(items_array.size()));

    for(int i=0; i<items_array.size(); i++)
    {
        const auto play_obj = items_array[i].toObject();
        const auto tracks_obj = play_obj.value("tracks").toObject();

        userPlaylistsArray[i].SetHref(tracks_obj.value("href").toString().toStdString());

    }

     replyProcessing-=1;

    GetPlaylistsTracks();

}

void SpotifyAPI::GetPlaylistsTracks()
{
    for (int i=0; i<userPlaylistsArray.size(); i++)
    {
       SpotifyPlaylist playlist_current = userPlaylistsArray[i];

       QUrl u (playlist_current.GetHref().c_str());

       replyProcessing+=1;
       auto reply = connectAuth.get(u);

       connect(reply,&QNetworkReply::finished,[=](){ this->GetPlaylistsTracksReply(reply, i);} );

       while(!reply->isFinished() || replyProcessing)
       {
           QCoreApplication::processEvents();
       }
     }

    SavePlaylistsJson();



}

void SpotifyAPI::GetPlaylistsTracksReply(QNetworkReply *network_reply, int indice)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to  tracks data"<<endl;
        return;
    }

    const auto data = network_reply->readAll();


    cout<<"playlist data = "<<data.toStdString()<<endl<<endl;

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();
    if(indice < userPlaylistsTracksJson.size())
        userPlaylistsTracksJson[indice] = root_obj;

    replyProcessing-=1;


}

void SpotifyAPI::SavePlaylistsJson()
{
    QJsonDocument playlistsJsonDoc;
    QJsonObject root_obj;
    root_obj.insert("info", QJsonValue::fromVariant("Document of Spotify playlists in JSOn format"));
    QJsonArray playlist_array;

    const auto playlist_items_array = userPlaylistsJson.value("items").toArray();

    for(int i=0; i < playlist_items_array.size();i++)
    {
        QJsonObject playlist_obj;
       playlist_obj.insert("name",playlist_items_array[i].toObject().value("name").toString());
       playlist_obj.insert("id",playlist_items_array[i].toObject().value("id").toString());
       playlist_obj.insert("href",playlist_items_array[i].toObject().value("href").toString());
       playlist_obj.insert("uri",playlist_items_array[i].toObject().value("uri").toString());

       QJsonArray tracks_array;

        const auto playlist_items_array = userPlaylistsTracksJson[i].value("items").toArray();
        for (int j=0; j<playlist_items_array.size(); j++)
        {
            const auto playlist_track = playlist_items_array[j].toObject().value("track").toObject();
            QJsonObject track_obj;
            track_obj.insert("name",playlist_track.value("name").toString());
            track_obj.insert("href",playlist_track.value("href").toString());
            track_obj.insert("id",playlist_track.value("id").toString());
            track_obj.insert("uri",playlist_track.value("uri").toString());
            tracks_array.append(track_obj);

        }

        playlist_obj.insert("items",tracks_array);
        playlist_array.append(playlist_obj);


    }

    root_obj.insert("items",playlist_array);

    //QJsonObject playlist_obj;
    //playlist_obj.insert()

 /*   QJsonArray playlists_json_array;

    for(int i=0; i<playlistsJsonVector.size(); i++)
        playlists_json_array.append(playlistsJsonVector[i]);

    root_obj.insert("items", playlists_json_array);*/
    playlistsJsonDoc.setObject(root_obj);

    QFile saveFile(QStringLiteral("playlistsdata.json"));

        if (!saveFile.open(QIODevice::WriteOnly)) {
            qWarning("Couldn't open save file.");
            return;
        }

        saveFile.write(playlistsJsonDoc.toJson());

}

void SpotifyAPI::SearchArtist(string artist_name)
{

    ClearArtistData();

    string url_search = "https://api.spotify.com/v1/search?";

    queryQ = "q=";

    // Used to split string around spaces.
    istringstream ss(artist_name);

    bool first_call=true;
    // Traverse through all words
    while (ss) {
        // Read a word
        string word;
        ss >> word;

        if(word.compare("")!=0)
        {
            if(first_call)
            {
                queryQ += word;
                first_call = false;
            }
            else
                queryQ +="&20" + word;
        }

    }

    queryQ +="&type=artist&limit=5";

    string result = url_search + queryQ;

    cout<<"URL search = "<<result<<endl;
    QUrl query_url(result.c_str());

    QNetworkReply * reply = connectAuth.get(query_url);

    connect(reply,&QNetworkReply::finished,[=](){ this->SearchArtistReply(reply);} );
}

void SpotifyAPI::SearchArtistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable to get artist data"<<endl;
        return;
    }

    const auto data = network_reply->readAll();


    //cout<<"artists data = "<<data.toStdString()<<endl<<endl;

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    const auto artists_obj = root_obj["artists"].toObject();
    const auto items_array = artists_obj["items"].toArray();

    QJsonObject artist_chosen_obj;
    if(items_array.size()>0)
    {
        for (int item_index = 0; item_index < items_array.size(); ++item_index) {

            QJsonObject item_obj = items_array[item_index].toObject();
            cout<<"artist found = "<<item_obj.value("name").toString().toStdString()<<endl<<endl;
        }
        artist_chosen_obj = items_array[0].toObject();

        artistId = artist_chosen_obj.value("id").toString();
        artistName = artist_chosen_obj.value("name").toString();

        cout<<"artist chosen: "<<artist_chosen_obj.value("name").toString().toStdString()<<" ID = "<<artistId.toStdString()<<endl;

        // artist_reply_finished = true;
        network_reply->deleteLater();


        OutputMessage message{"Artist found = " + artistName,false};
        messagesArray.push_back(message);
        emit UpdateOutputTextSignal();

        OutputMessage message2{"Artist ID = " + artistId,false};
        messagesArray.push_back(message2);
        emit UpdateOutputTextSignal();


        SearchTopTracks(artistId);


    }
    else
    {
        OutputMessage message{"Error: Artist NOT found",false};
        messagesArray.push_back(message);
        emit UpdateOutputTextSignal();

        cout<<"Error when getting artist names"<<endl;
        return;
    }



    /*
    string playlist_name = artistName.toStdString();
    playlist_name+= "TOP";
    string description = "Playlist created by cplusplus APP spotfy API";

    //CreatePlaylist(playlist_name,false,description);
    */

}

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


            cout<<"track name = "<<track_obj.value("name").toString().toStdString()<< " / uri = "<<track_obj.value("uri").toString().toStdString()<<endl;
        }

        string text = "Artist Tracks = " + playlist.GetTracksListStr("/");
        OutputMessage message{text.c_str(),false};
        messagesArray.push_back(message);
        emit UpdateOutputTextSignal();

        emit ArtistTracksFoundSignal();

    }
    else
        cout<<"Error when getting tracks names"<<endl;


}

void SpotifyAPI::CreatePlaylistWeb(string playlist_name, bool is_public, string description)
{

    string query = "https://api.spotify.com/v1/users/" + userName.toStdString() + "/playlists";
    cout<<"query create playlist= "<<query<<endl<<endl;

    QUrl query_url(query.c_str());

    QJsonObject playlist_obj;
    playlist_obj.insert("name", QJsonValue::fromVariant(playlist_name.c_str()));
    playlist_obj.insert("description", QJsonValue::fromVariant(description.c_str()));
    playlist_obj.insert("public", QJsonValue::fromVariant(is_public));

    QJsonDocument playlist_doc(playlist_obj);

    cout<<"Creates playlist with data= "<<playlist_doc.toJson().toStdString()<<endl;

    auto network_manager = connectAuth.networkAccessManager();


    QNetworkRequest request(query_url);
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");

    QByteArray code;
    code.append("application/json");
    request.setRawHeader("Accept",code);

    QString auth_header("Bearer ");

    code.clear();
    code.append(auth_header + connectAuth.token());
    request.setRawHeader("Authorization", code);

    /* cout<<"Token = "<<connectAuth.token().toStdString()<<endl<<endl;
    QList<QByteArray> headerList = request.rawHeaderList();

       for (int i = 0; i < headerList.count(); ++i) {
           cout<< "header = " << headerList[i].toStdString()<<endl;
       }
*/

    // Send request
    auto reply = network_manager->post(request, playlist_doc.toJson());
    connect(reply,&QNetworkReply::finished,[=](){ this->CreatePlaylistReply(reply);} );

}

void SpotifyAPI::CreatePlaylistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable criate playlist"<<endl;
        cout<<"error = "<<network_reply->error()<<endl;

        const auto data = network_reply->readAll();
        cout<<"Error body data ="<<data.toStdString()<<endl;

        return;
    }

    const auto data = network_reply->readAll();

    cout<<"Playlist created info = "<<data.toStdString()<<endl;

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


    OutputMessage message{text,false};
    messagesArray.push_back(message);
    emit UpdateOutputTextSignal();

    network_reply->deleteLater();

    AddTracksPlaylistWeb();

}

void SpotifyAPI::AddTracksPlaylistWeb()
{
    if(!playlist.GetId().empty())
    {
        string query = "https://api.spotify.com/v1/playlists/" + playlist.GetId() + "/tracks?";

        string tracks_query = "uris=" + playlist.GetTracksUrisListStr(",");
        query += tracks_query;

        QUrl query_url(query.c_str());

        cout<<"query create playlist= "<<query<<endl<<endl;

        QJsonObject json_obj;
        QJsonDocument json_doc(json_obj);

        auto network_manager = connectAuth.networkAccessManager();


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
        auto reply = network_manager->post(request, QByteArray());//json_doc.toJson());
        connect(reply,&QNetworkReply::finished,[=](){ this->AddTracksPlaylistReply(reply);} );


    }
    else
    {

    }

}

void SpotifyAPI::AddTracksPlaylistReply(QNetworkReply* network_reply)
{
    if (network_reply->error() != QNetworkReply::NoError) {
        cout<<"Unable add tracks to playlist"<<endl;
        cout<<"error = "<<network_reply->error()<<endl;

        const auto data = network_reply->readAll();
        cout<<"Error body data ="<<data.toStdString()<<endl;

        return;
    }

    const auto data = network_reply->readAll();

    const auto document = QJsonDocument::fromJson(data);
    const auto root_obj = document.object();

    QString text = "Tracks added succesfully \n ";
    text += "snapshot_id = " + root_obj.value("snapshot_id").toString();
    OutputMessage message{text,false};
    messagesArray.push_back(message);
    emit UpdateOutputTextSignal();

    cout<<"Tracks added to playlist = "<<data.toStdString()<<endl;
}

OutputMessage SpotifyAPI::GetLastMessage()
{
    OutputMessage result;
    if(messagesArray.size()>0)
    {
        result = messagesArray[0];
        messagesArray.erase(messagesArray.begin());
    }

    return result;

}

SpotifyPlaylist SpotifyAPI::GetPlaylist()
{
    return playlist;
}

void SpotifyAPI::ClearArtistData()
{
    artistName = "";
    artistId="";
}

SpotifyAPI::~SpotifyAPI()
{
    delete replyHandler;
}
