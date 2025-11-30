#include "CampusCompass.h"

#include <string>
#include <fstream>
#include <sstream>
#include <iostream>
#include <queue>
#include <regex>
#include <algorithm>
#include <limits>

using namespace std;

CampusCompass::CampusCompass() {
    // initialize your object
}

int CampusCompass::parseTime(const string &t) {
    // format HH:MM
    if (t.size() != 5 || t[2] != ':') return -1;
    int hh = stoi(t.substr(0,2));
    int mm = stoi(t.substr(3,2));
    return hh*60 + mm;
}

pair<unordered_map<int,int>, unordered_map<int,int>> CampusCompass::dijkstra_with_parents(int src) const {
    unordered_map<int,int> dist;
    unordered_map<int,int> parent;
    struct Node { int v; int d; };
    struct Cmp { bool operator()(const Node&a,const Node&b) const { return a.d>b.d; } };
    priority_queue<Node, vector<Node>, Cmp> pq;

    // init
    for (auto &p : adj) {
        dist[p.first] = numeric_limits<int>::max();
    }
    if (dist.find(src) == dist.end()) {
        // ensure src present
        dist[src] = 0;
    }
    dist[src] = 0;
    pq.push({src,0});

    while (!pq.empty()) {
        Node cur = pq.top(); pq.pop();
        if (cur.d != dist[cur.v]) continue;
        auto it = adj.find(cur.v);
        if (it == adj.end()) continue;
        for (auto &e : it->second) {
            if (!e.open) continue; // closed edge
            int nd = cur.d + e.weight;
            if (nd < dist[e.to]) {
                dist[e.to] = nd;
                parent[e.to] = cur.v;
                pq.push({e.to, nd});
            }
        }
    }

    return {dist, parent};
}

bool CampusCompass::ParseCSV(const string &edges_filepath, const string &classes_filepath) {
    // parse edges
    ifstream fe(edges_filepath);
    if (!fe.is_open()) return false;
    string line;
    // header
    if (!getline(fe, line)) return false;
    while (getline(fe, line)) {
        if (line.size() == 0) continue;
        // split by comma
        vector<string> tokens;
        string token;
        stringstream ss(line);
        while (getline(ss, token, ',')) tokens.push_back(token);
        if (tokens.size() < 5) continue;
        int a = stoi(tokens[0]);
        int b = stoi(tokens[1]);
        string name_a = tokens[2];
        string name_b = tokens[3];
        int w = stoi(tokens[4]);

        id_to_name[a] = name_a;
        id_to_name[b] = name_b;

        adj[a].push_back({b, w, true});
        adj[b].push_back({a, w, true});
        edges_map[keyPair(a,b)] = {w, true};
    }
    fe.close();

    // parse classes
    ifstream fc(classes_filepath);
    if (!fc.is_open()) return false;
    if (!getline(fc, line)) return false; // header
    while (getline(fc, line)) {
        if (line.size() == 0) continue;
        vector<string> tokens;
        string token;
        stringstream ss(line);
        while (getline(ss, token, ',')) tokens.push_back(token);
        if (tokens.size() < 4) continue;
        string code = tokens[0];
        int loc = stoi(tokens[1]);
        int st = parseTime(tokens[2]);
        int en = parseTime(tokens[3]);
        classes[code] = {loc, st, en};
        // ensure node is present even if it had no edges
        if (adj.find(loc) == adj.end()) adj[loc] = {};
    }
    fc.close();

    return true;
}

bool CampusCompass::ParseCommand(const string &command) {
    // Tokenize but handle quoted name for insert
    string cmd;
    stringstream ss(command);
    if (!(ss >> cmd)) return false;

    // helper lambdas
    auto is_valid_ufid = [&](const string &id)->bool {
        if (id.size() != 8) return false;
        return all_of(id.begin(), id.end(), ::isdigit);
    };
    auto is_valid_name = [&](const string &name)->bool {
        if (name.empty()) return false;
        for (char c : name) if (!(isalpha(c) || c==' ')) return false;
        return true;
    };
    auto valid_classcode = [&](const string &code)->bool {
        regex r("^[A-Z]{3}[0-9]{4}$");
        return regex_match(code, r);
    };

    if (cmd == "insert") {
        // insert "NAME" ID RES N CLASSCODES...
        string rest = command.substr(6);
        // find quoted name
        size_t p1 = rest.find('"');
        size_t p2 = string::npos;
        if (p1 == string::npos) { cout << "unsuccessful" << '\n'; return true; }
        p2 = rest.find('"', p1+1);
        if (p2 == string::npos) { cout << "unsuccessful" << '\n'; return true; }
        string name = rest.substr(p1+1, p2-p1-1);
        // move pointer after last quote
        string after = rest.substr(p2+1);
        stringstream s2(after);
        string id; int residence; int N;
        if (!(s2 >> id >> residence >> N)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id) || !is_valid_name(name) || N < 1 || N > 6) { cout << "unsuccessful" << '\n'; return true; }
        vector<string> codes; string c;
        for (int i=0;i<N;i++) { if (!(s2 >> c)) { cout << "unsuccessful" << '\n'; return true; } codes.push_back(c); }
        // validate codes
        for (auto &cc : codes) if (!valid_classcode(cc) || classes.find(cc)==classes.end()) { cout << "unsuccessful" << '\n'; return true; }
        // unique id
        if (students.find(id) != students.end()) { cout << "unsuccessful" << '\n'; return true; }
        // name validation passed
        Student st; st.name = name; st.id = id; st.residence = residence;
        for (auto &cc : codes) st.classes.insert(cc);
        students[id] = st;
        cout << "successful" << '\n';
        return true;
    } else if (cmd == "remove") {
        string id; if (!(ss>>id)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        students.erase(it);
        cout << "successful" << '\n';
        return true;
    } else if (cmd == "dropClass") {
        string id, code; if (!(ss>>id>>code)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id) || !valid_classcode(code)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        auto &cls = it->second.classes;
        if (cls.find(code) == cls.end()) { cout << "unsuccessful" << '\n'; return true; }
        cls.erase(code);
        if (cls.empty()) students.erase(it);
        cout << "successful" << '\n';
        return true;
    } else if (cmd == "replaceClass") {
        string id, c1, c2; if (!(ss>>id>>c1>>c2)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id) || !valid_classcode(c1) || !valid_classcode(c2)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        if (classes.find(c2) == classes.end()) { cout << "unsuccessful" << '\n'; return true; }
        auto &cls = it->second.classes;
        if (cls.find(c1) == cls.end()) { cout << "unsuccessful" << '\n'; return true; }
        if (cls.find(c2) != cls.end()) { cout << "unsuccessful" << '\n'; return true; }
        cls.erase(c1);
        cls.insert(c2);
        cout << "successful" << '\n';
        return true;
    } else if (cmd == "removeClass") {
        string code; if (!(ss>>code)) { cout << "unsuccessful" << '\n'; return true; }
        if (!valid_classcode(code)) { cout << "unsuccessful" << '\n'; return true; }
        int removed = 0;
        vector<string> dropIds;
        for (auto &p : students) {
            if (p.second.classes.find(code) != p.second.classes.end()) {
                dropIds.push_back(p.first);
            }
        }
        for (auto &id : dropIds) {
            auto it = students.find(id);
            if (it == students.end()) continue;
            it->second.classes.erase(code);
            removed++;
            if (it->second.classes.empty()) students.erase(it);
        }
        cout << removed << '\n';
        return true;
    } else if (cmd == "toggleEdgesClosure") {
        int N; if (!(ss>>N)) { cout << "unsuccessful" << '\n'; return true; }
        for (int i=0;i<N;i++) {
            int a,b; if (!(ss>>a>>b)) { cout << "unsuccessful" << '\n'; return true; }
            auto key = keyPair(a,b);
            auto it = edges_map.find(key);
            if (it == edges_map.end()) {
                // they said always valid edges but be safe
                continue;
            }
            it->second.open = !it->second.open;
            // update adjacency entries both ways
            for (auto &e : adj[a]) if (e.to==b) e.open = it->second.open;
            for (auto &e : adj[b]) if (e.to==a) e.open = it->second.open;
        }
        cout << "successful" << '\n';
        return true;
    } else if (cmd == "checkEdgeStatus") {
        int a,b; if (!(ss>>a>>b)) { cout << "unsuccessful" << '\n'; return true; }
        auto key = keyPair(a,b);
        auto it = edges_map.find(key);
        if (it == edges_map.end()) { cout << "DNE" << '\n'; return true; }
        cout << (it->second.open?"open":"closed") << '\n';
        return true;
    } else if (cmd == "isConnected") {
        int a,b; if (!(ss>>a>>b)) { cout << "unsuccessful" << '\n'; return true; }
        // BFS or DFS using only open edges
        unordered_set<int> visited;
        queue<int> q;
        q.push(a); visited.insert(a);
        bool ok = false;
        while (!q.empty()) {
            int v = q.front(); q.pop();
            if (v == b) { ok = true; break; }
            auto it = adj.find(v);
            if (it==adj.end()) continue;
            for (auto &e : it->second) {
                if (!e.open) continue;
                if (!visited.count(e.to)) { visited.insert(e.to); q.push(e.to); }
            }
        }
        cout << (ok?"successful":"unsuccessful") << '\n';
        return true;
    } else if (cmd == "printShortestEdges") {
        string id; if (!(ss>>id)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        auto &student = it->second;
        cout << "Name: " << student.name << '\n';
        // get distances + parents from residence
        auto [dist, parent] = dijkstra_with_parents(student.residence);
        // prepare vector of classcodes sorted lexicographically
        vector<string> codes(student.classes.begin(), student.classes.end());
        sort(codes.begin(), codes.end());
        for (auto &cc : codes) {
            int loc = classes[cc].loc;
            int d = -1;
            if (dist.find(loc) != dist.end() && dist.at(loc) != numeric_limits<int>::max()) d = dist.at(loc);
            cout << cc << " | Total Time: " << d << '\n';
        }
        return true;
    } else if (cmd == "printStudentZone") {
        string id; if (!(ss>>id)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        auto &student = it->second;
        // find shortest path to each class and collect nodes on the path
        unordered_set<int> nodeset;
        auto [dist, parent] = dijkstra_with_parents(student.residence);
        for (auto &cc : student.classes) {
            int loc = classes[cc].loc;
            if (dist.find(loc) == dist.end() || dist.at(loc)==numeric_limits<int>::max()) continue; // unreachable
            // backtrack path using parent map
            int cur = loc; nodeset.insert(cur);
            while (cur != student.residence) {
                auto pit = parent.find(cur);
                if (pit==parent.end()) break;
                cur = pit->second;
                nodeset.insert(cur);
            }
        }
        // create subgraph edges among nodes in nodeset (only open edges)
        // compute MST using Kruskal
        vector<tuple<int,int,int>> edges; // w,a,b
        for (auto nid : nodeset) {
            auto it2 = adj.find(nid);
            if (it2==adj.end()) continue;
            for (auto &e : it2->second) {
                if (!e.open) continue;
                if (!nodeset.count(e.to)) continue;
                if (nid < e.to) edges.emplace_back(e.weight, nid, e.to);
            }
        }
        // Kruskal
        sort(edges.begin(), edges.end());
        unordered_map<int,int> comp;
        function<int(int)> findp = [&](int x)->int{
            if (comp.find(x)==comp.end()) comp[x]=x;
            if (comp[x]==x) return x;
            return comp[x]=findp(comp[x]);
        };
        auto unite = [&](int a,int b){ int pa=findp(a), pb=findp(b); if (pa!=pb) comp[pa]=pb; };
        int total = 0; int count = 0;
        for (auto &t : edges) {
            int w,a,b; tie(w,a,b)=t;
            if (findp(a) != findp(b)) { unite(a,b); total += w; count++; }
        }
        cout << "Student Zone Cost For " << student.name << ": " << total << '\n';
        return true;
    } else if (cmd == "verifySchedule") {
        string id; if (!(ss>>id)) { cout << "unsuccessful" << '\n'; return true; }
        if (!is_valid_ufid(id)) { cout << "unsuccessful" << '\n'; return true; }
        auto it = students.find(id);
        if (it == students.end()) { cout << "unsuccessful" << '\n'; return true; }
        auto &student = it->second;
        if (student.classes.size() <= 1) { cout << "unsuccessful" << '\n'; return true; }
        // build vector of class codes sorted by start time
        vector<pair<int,string>> order;
        for (auto &cc : student.classes) {
            if (classes.find(cc)==classes.end()) continue;
            order.emplace_back(classes[cc].start_minutes, cc);
        }
        sort(order.begin(), order.end());
        cout << "Schedule Check for " << student.name << ':' << '\n';
        for (size_t i=0;i+1<order.size();++i) {
            string cA = order[i].second; string cB = order[i+1].second;
            int locA = classes[cA].loc; int locB = classes[cB].loc;
            // shortest path between locA and locB using current open edges
            // run dijkstra from locA
            auto [distA, parentA] = dijkstra_with_parents(locA);
            int needed = -1;
            if (distA.find(locB) != distA.end() && distA[locB] != numeric_limits<int>::max()) needed = distA[locB];
            int gap = classes[cB].start_minutes - classes[cA].end_minutes;
            cout << cA << " - " << cB << ' ' << ( (gap >= needed)?"\"Can make it!\"":"\"Cannot make it!\"" ) << '\n';
        }
        return true;
    }

    // unknown command
    cout << "unsuccessful" << '\n';
    return false;
}
