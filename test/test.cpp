#include <catch2/catch_test_macros.hpp>
#include <iostream>
#include <sstream>

#include "CampusCompass.h"

using namespace std;

// Test invalid insert commands and bad formatting
TEST_CASE("Invalid inserts are rejected","[invalid]") {
    CampusCompass C;
    REQUIRE(C.ParseCSV("data/edges.csv", "data/classes.csv") == true);

    // capture cout
    ostringstream out;
    auto *old = cout.rdbuf(out.rdbuf());

    // invalid UFID (not 8 digits)
    C.ParseCommand("insert \"BadName\" 1234 1 1 COP3502");
    // invalid name (contains digits)
    C.ParseCommand("insert \"Name1\" 10000001 1 1 COP3502");
    // invalid class count (N=0)
    C.ParseCommand("insert \"Good Name\" 10000002 1 0 COP3502");

    cout.rdbuf(old);
    string s = out.str();
    REQUIRE(s.find("unsuccessful") != string::npos);
}

// Test add, drop and removeClass mechanics
TEST_CASE("Student insert/delete and removeClass works","[modify]") {
    CampusCompass C;
    REQUIRE(C.ParseCSV("data/edges.csv", "data/classes.csv") == true);
    ostringstream out; auto *old = cout.rdbuf(out.rdbuf());

    // insert two students
    C.ParseCommand("insert \"Alice\" 20000001 23 2 COP3503 COP3530");
    C.ParseCommand("insert \"Bob\" 20000002 23 1 COP3503");
    // drop class from Alice
    C.ParseCommand("dropClass 20000001 COP3530");
    // remove non-existent student
    C.ParseCommand("remove 11111111");
    // removeClass COP3503 should remove from two students (Bob and Alice had COP3503)
    C.ParseCommand("removeClass COP3503");

    cout.rdbuf(old);
    string outstr = out.str();
    // check expected lines exist
    REQUIRE(outstr.find("successful") != string::npos);
    REQUIRE(outstr.find("2\n") != string::npos);
}

// Test replaceClass validity
TEST_CASE("Replace class works and enforces rules","[modify]") {
    CampusCompass C;
    REQUIRE(C.ParseCSV("data/edges.csv", "data/classes.csv") == true);
    ostringstream out; auto *old = cout.rdbuf(out.rdbuf());

    C.ParseCommand("insert \"Claire\" 30000001 23 1 COP3503");
    // replace to a valid new code
    C.ParseCommand("replaceClass 30000001 COP3503 COP3530");
    // try replacing to non-existent code
    C.ParseCommand("replaceClass 30000001 COP3530 ABC9999");

    cout.rdbuf(old);
    string s = out.str();
    // first replacement should be successful, second unsuccessful
    REQUIRE(s.find("successful") != string::npos);
    REQUIRE(s.rfind("unsuccessful") != string::npos);
}

// Test shortest path then toggle edges so a class becomes unreachable
TEST_CASE("printShortestEdges toggles and becomes unreachable","[paths]") {
    CampusCompass C;
    REQUIRE(C.ParseCSV("data/edges.csv", "data/classes.csv") == true);

    // student located at node 23 (Carleton Auditorium)
    // classes at COP3503 (start loc 23) and COP3530 (loc 14)
    ostringstream out; auto *old = cout.rdbuf(out.rdbuf());

    C.ParseCommand("insert \"TestUser\" 40000001 23 2 COP3503 COP3530");
    C.ParseCommand("printShortestEdges 40000001");
    // close all edges incident to 23 so 23 becomes isolated from the rest of the graph
    // edges touching 23 are: 13-23, 14-23, 22-23, 23-24
    C.ParseCommand("toggleEdgesClosure 4 13 23 14 23 22 23 23 24");
    C.ParseCommand("printShortestEdges 40000001");

    cout.rdbuf(old);
    string s = out.str();
    // first print should show non -1 for COP3530 and COP3503
    REQUIRE(s.find("COP3530 | Total Time:") != string::npos);
    // after toggle COP3530 should show Total Time: -1
    auto pos = s.rfind("COP3530 | Total Time: -1");
    REQUIRE(pos != string::npos);
}
