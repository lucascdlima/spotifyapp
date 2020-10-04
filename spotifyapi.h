#ifndef SPOTIFYAPI_H
#define SPOTIFYAPI_H

#include <QObject>
#include <QtNetworkAuth>
#include <QJsonDocument>
#include <QJsonObject>
#include <QNetworkAccessManager>
#include <QOAuth2AuthorizationCodeFlow>
#include <QDesktopServices>
#include <iostream>
#include <sstream>
#include "spotifyutils.h"

using namespace std;

struct OutputMessage {
    OutputMessage(QString text_in="", bool clear_in=false): text(text_in),clear(clear_in){ }
    QString text;
    bool clear;
};


class SpotifyAPI: public QObject
{
    Q_OBJECT
public:

    SpotifyAPI();
    ~SpotifyAPI();

    void ConnectToServer();
    bool IsConnected();
    bool IsProcessingRequest();
    void GetUserName(QNetworkReply* network_reply);
    void GetCurrentPlaylists();
    void GetCurrentPlaylistsReply(QNetworkReply *network_reply);
    void GetPlaylistsTracks();
    void GetPlaylistsTracksReply(QNetworkReply *network_reply, int indice);
    void SavePlaylistsJson();
    void SearchArtist(string artist_name);
    void SearchArtistReply(QNetworkReply* network_reply);
    void SearchTopTracks(QString artist_id);
    void SearchTopTracksReply(QNetworkReply* network_reply);
    void CreatePlaylistWeb(string playlist_name, bool is_public, string description);
    void CreatePlaylistReply(QNetworkReply* network_reply);
    void AddTracksPlaylistWeb();
    void AddTracksPlaylistReply(QNetworkReply* network_reply);
    void ClearArtistData();

    SpotifyPlaylist GetPlaylist();

    OutputMessage GetLastMessage();
    vector<OutputMessage> messagesArray;


private slots:
    void AccessGranted();
    void AuthStatusChanged(QAbstractOAuth::Status status);

signals:
    void UpdateOutputTextSignal();
    void ConnectedSignal();
    void ArtistTracksFoundSignal();


private:
    bool isConnected;
    QOAuthHttpServerReplyHandler* replyHandler;
    QOAuth2AuthorizationCodeFlow connectAuth;
    bool processingRequest;
    QString clientId;
    QString clientSecret;
    QString userName;

    QString token;
    string queryQ;
    QString artistId;
    QString artistName;
    vector<QString> artistTracksUri;
    SpotifyPlaylist playlist;
    vector<SpotifyPlaylist> playlistsUserArray;
    vector<SpotifyPlaylist> userPlaylistsArray;
    vector<QJsonObject> playlistsJsonVector;

    QJsonObject userPlaylistsJson;
    vector<QJsonObject> userPlaylistsTracksJson;

    int replyProcessing;
    

};

#endif // SPOTIFYAPI_H
