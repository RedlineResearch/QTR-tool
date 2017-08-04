#include "ClassTable.h"
#include <utility>
#include <iostream>

using namespace std;

ClassTable::ClassTable() :
    counter(1) {}

void ClassTable::mapClass(string className)
{
    table.insert(make_pair(className, counter));
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

void ClassTable::dumpTable()
{
    for (auto it = table.begin(); it != table.end(); ++it) {
        cout << it->second << ", " << it->first << endl;
    }
}
