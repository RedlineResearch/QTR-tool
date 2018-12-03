#include "ClassTable.h"
#include <utility>
#include <iostream>

using namespace std;

ClassTable::ClassTable() :
    counter(9)
{
    // pre-fill the table with primitive types i.e. "pseudo-classes"
    table.insert(make_pair("bool", 1));
    table.insert(make_pair("char", 2));
    table.insert(make_pair("float", 3));
    table.insert(make_pair("double", 4));
    table.insert(make_pair("byte", 5));
    table.insert(make_pair("short", 6));
    table.insert(make_pair("int", 7));
    table.insert(make_pair("long", 8));
}

void ClassTable::mapClass(string className)
{
    table.insert(make_pair(className, counter));
    // cerr << className << ": " << counter << endl;   
    counter++;
}

int ClassTable::findClassID(string className)
{
    if (table.find(className) != table.end()) {
        return table[className];
    } else {
        return -1;
    }
}

int ClassTable::mapOrFind(string className)
{
    if (table.find(className) == table.end()) {
        mapClass(className);
    }
    
    return table[className];
}

void ClassTable::dumpTable(ofstream &dumpFile)
{
    for ( auto it = table.begin();
          it != table.end();
          ++it ) {
        dumpFile << it->second << "  " << it->first << endl;
    }
}
