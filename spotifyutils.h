#ifndef SPOTIFYUTILS_H
#define SPOTIFYUTILS_H

#include "musicutils.h"

class SpotifyTrack: public Track
{

public:
    SpotifyTrack(){name=""; id=""; uri="";}
    SpotifyTrack(string track_name, string track_id, string track_uri){name = track_name; id = track_id; uri=track_uri;}
    void SetId(string id_in){id = id_in;}
    string GetId(){return id;}
    void SetURI(string uri_in){uri = uri_in;}
    string GetURI(){return uri;}

private:
    string id;
    string uri;

};

class SpotifyPlaylist: public Playlist<SpotifyTrack>
{

public:
    void SetId(string id_in){id = id_in;}
    string GetId(){return id;}
    void SetURI(string uri_in){uri = uri_in;}
    string GetURI(){return uri;}
    void SetHref(string href_in){href = href_in;}
    string GetHref(){return href;}

    string GetTracksListStr(string separator)
    {
        string result = "";
        if(tracksArray.size()>0)
        {
            for (vector<SpotifyTrack>::iterator it = tracksArray.begin() ; it != tracksArray.end(); ++it)
            {
                result += it->GetName() + separator;
            }
        }
        return result;
    }

    string GetTracksUrisListStr(string separator)
    {
        string result = "";
        if(tracksArray.size()>0)
        {
            result = tracksArray.begin()->GetURI();
            for (vector<SpotifyTrack>::iterator it = tracksArray.begin()+1 ; it != tracksArray.end(); ++it)
            {
                result += separator + it->GetURI() ;
            }
        }
        return result;
    }

    void ClearPlaylist()
    {
        tracksArray.clear();
        id = "";
        name = "";
        uri = "";
        href = "";
    }

private:
    string id;
    string uri;
    string href;



};

#endif // SPOTIFYUTILS_H
