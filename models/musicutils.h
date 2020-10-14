#ifndef MUSICUTILS_H
#define MUSICUTILS_H

#include <iostream>
#include <vector>

using namespace std;

/**
 * Implementation of basic class to handle tracks data.
 * This class doens't have communication with User Interface.
*/
class Track{

public:
    void SetName(string text){ name = text;}
    string GetName(){return name;}
protected:
    string name;

};

/**
 * Implementation of basic class to handle Playlists data. It is based in
 * tree structure where playlists have tracks children.
 * This class doens't have communication with User Interface.
*/
template <class T>
class Playlist{
public:
    void SetName(string text){ name = text;}
    string GetName(){return name;}
    void AddTrack(T track){ tracksArray.push_back(track);}
    void RemoveLastTrack(){ tracksArray.pop_back();}

protected:
    string name;
    vector<T> tracksArray;

};

#endif // MUSICUTILS_H
