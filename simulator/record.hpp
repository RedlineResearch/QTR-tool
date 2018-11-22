#ifndef RECORD_HPP
#define RECORD_HPP

// ----------------------------------------------------------------------
//   Record
//   TODO

#include <iostream>

using namespace std;

// Base class
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

// Derived record classes
class AllocRecord : public Record
{
    public:
        AllocRecord() {}

};

class ArrayRecord : public Record
{
    public:
        ArrayRecord() {}

};

class MethodRecord : public Record
{
    public:
        MethodRecord() {}

};

#endif // RECORD_HPP
