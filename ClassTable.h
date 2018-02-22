#ifndef ET2_CLASSTABLE_H_
#define ET2_CLASSTABLE_H_

#include <unordered_map>
#include <iostream>
#include <fstream>

class ClassTable {
public:
    ClassTable();
    void mapClass(std::string className);
    int findClassID(std::string className);
    int mapOrFind(std::string className);
    void dumpTable(std::ofstream &dumpFile);
private:
    std::unordered_map<std::string, int> table;
    int counter;
};

#endif
