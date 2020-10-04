#ifndef MUSICUTILS_H
#define MUSICUTILS_H

#include <iostream>
#include <vector>

using namespace std;

class Track{

public:
    void SetName(string text){ name = text;}
    string GetName(){return name;}
protected:
    string name;

};

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
