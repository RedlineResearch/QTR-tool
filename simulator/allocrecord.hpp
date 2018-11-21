#ifndef ALLOCRECORD_H
#define ALLOCRECORD_H

// ----------------------------------------------------------------------
//   AllocRecord
//   TODO

#include "record.hpp"

#include <iostream>

using namespace std;

// -- Size of temporary char arrays (yes, I know, this could cause buffer overflow)
#define LINESIZE 2048
#define TOKENSIZE 50

class AllocRecord : public Record
{
    public:
        AllocRecord() {}

};

#endif // ALLOCRECORD_H
