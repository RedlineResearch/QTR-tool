#ifndef RECORD_H
#define RECORD_H

// ----------------------------------------------------------------------
//   Record
//   TODO

#include <iostream>

using namespace std;

// -- Size of temporary char arrays (yes, I know, this could cause buffer overflow)
#define LINESIZE 2048
#define TOKENSIZE 50

class Record
{
    private:
        char m_type;

    public:
        Record() {}

        inline char getType() const
        {
            return this->m_type;
        }

};

#endif // RECORD_H
