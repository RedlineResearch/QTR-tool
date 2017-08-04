#ifndef ET2_CLASSTABLE_H_
#define ET2_CLASSTABLE_H_

#include <map>

class ClassTable {
public:
    ClassTable();
    void mapClass(std::string className);
    int findClassID(std::string className);
    int mapOrFind(std::string className);
    void dumpTable();
private:
    std::map<std::string, int> table;
    int counter;
};

#endif
