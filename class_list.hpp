#ifndef CLASS_LIST_HPP
#define CLASS_LIST_HPP

#include <set>

using namespace std;

// Contains the list of classes to instrument for the all of their method call events.

class class_list {
    public:
        static set<string> white_list;
};

#endif // CLASS_LIST_HPP
