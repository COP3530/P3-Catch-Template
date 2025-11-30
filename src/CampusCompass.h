#pragma once
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <set>
#include <optional>

using namespace std;

struct EdgeInfo {
    int to;
    int weight;
    bool open;
};

struct ClassInfo {
    int loc;
    int start_minutes; // minutes since midnight
    int end_minutes;
};

struct Student {
    string name;
    string id; // 8-digit string
    int residence;
    set<string> classes; // class codes
};

class CampusCompass {
private:
    // Graph representation: map node id -> list of edges
    unordered_map<int, vector<EdgeInfo>> adj;
    // To find edges for toggle/check, keep canonical pair -> weight + open
    struct EdgeKey { int a, b; };

    // map of canonical (min,max) -> index in adjacency lists or weight+open flag
    struct EdgeRecord { int weight; bool open; };
    unordered_map<long long, EdgeRecord> edges_map;

    // id -> human readable name
    unordered_map<int, string> id_to_name;

    // classes: code -> ClassInfo
    unordered_map<string, ClassInfo> classes;

    // students: id -> Student
    unordered_map<string, Student> students;

    // helper routines
    static long long keyPair(int a, int b) {
        if (a > b) std::swap(a,b);
        return (static_cast<long long>(a) << 32) | static_cast<unsigned long long>(b);
    }

    // parse time "HH:MM" -> minutes since midnight
    static int parseTime(const string &t);

    // shortest path dijkstra that returns (distance, parents map)
    pair<unordered_map<int,int>, unordered_map<int,int>> dijkstra_with_parents(int src) const;

public:
    CampusCompass(); // constructor

    // loads edges and classes from CSV paths. returns false on failure
    bool ParseCSV(const string &edges_filepath, const string &classes_filepath);

    // parse a single-line command, do the action and write any required output
    // returns true if the command was validly parsed (even if semantically unsuccessful)
    bool ParseCommand(const string &command);
};
