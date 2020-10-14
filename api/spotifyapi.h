#ifndef SPOTIFYAPI_H
#define SPOTIFYAPI_H

#include <QObject>
#include <QtNetworkAuth>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QOAuth2AuthorizationCodeFlow>
#include <QDesktopServices>
#include <QXmlStreamReader>
#include <QFile>
#include <iostream>
#include <sstream>
#include "models/spotifyutils.h"
#include "models/treemodel.h"

using namespace std;

class SpotifyAPI: public QObject
{
    Q_OBJECT
public:

    SpotifyAPI(const char* fileName);
    ~SpotifyAPI();

    bool ReadUserKeys(const char* fileName);
    void ConnectToServer();
    bool IsConnected();
    bool IsProcessingRequest();

    void GetUserName(QNetworkReply* network_reply);

    void GetCurrentPlaylists();
    void GetCurrentPlaylistsReply(QNetworkReply *network_reply);

    void GetPlaylistsTracks();
    void GetPlaylistsTracksReply(QNetworkReply *network_reply, int indice);

    bool SavePlaylistsJsonFromWeb(QString fileName);

    void SearchArtist(QString artistName);
    void SearchArtistReply(QNetworkReply* network_reply);

    void SearchTopTracks(QString artist_id);
    void SearchTopTracksReply(QNetworkReply* network_reply);

    void CreatePlaylistWeb(QString playlist_name, bool is_public, QString description);
    void CreatePlaylistReply(QNetworkReply* network_reply);

    void AddTracksPlaylistWeb();
    void AddTracksPlaylistReply(QNetworkReply* network_reply);

    void SearchTrack(QString name);
    void SearchTrackReply(QNetworkReply *network_reply);

    void PlayTracks(QString uri);
    void PlayTracksReply(QNetworkReply *network_reply);

    SpotifyPlaylist GetPlaylist();


private slots:
    void AccessGranted();
    void AuthStatusChanged(QAbstractOAuth::Status status);

signals:
    void UpdateOutputTextSignal(QString text, bool clear);
    void ConnectedSignal();
    void ArtistTracksFoundSignal();
    void TracksFoundSignal(QJsonObject data);


private:

    bool copyJsonData(QStringList jsonHeaders, QJsonObject &destData, QJsonObject sourceData);

    bool isConnected;
    QOAuthHttpServerReplyHandler* replyHandler;
    QOAuth2AuthorizationCodeFlow connectAuth;
    bool processingRequest;
    QString clientId;
    QString clientSecret;
    QString userName;

    QString token;

    vector<QString> artistTracksUri;
    SpotifyPlaylist playlist;
    vector<SpotifyPlaylist> playlistsUserArray;
    vector<SpotifyPlaylist> userPlaylistsArray;

    QJsonObject userPlaylistsJson;
    vector<QJsonObject> userPlaylistsFullJson;


};

#endif // SPOTIFYAPI_H
