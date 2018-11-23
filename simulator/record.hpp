#ifndef RECORD_HPP
#define RECORD_HPP

// ----------------------------------------------------------------------
//   Record
//   TODO

#include "classinfo.hpp"

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
    private:
        ObjectId_t m_objectId;
        SiteId_t m_siteId;
        ThreadId_t m_threadId;
        TypeId_t m_typeId;
        unsigned int length;
        unsigned int size;
        // <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
    public:
        // <object-id> <size> <type-id> <site-id> <length (0)> <thread-id>
        AllocRecord()
        {
        }

};

class ArrayRecord : public Record
{
    public:
        ArrayRecord() {}

};

class ExitRecord : public Record
{
    private:
        MethodId_t m_methId;
        ThreadId_t m_threadId;

    public:
        ExitRecord( MethodId_t methId,
                    ThreadId_t threadId )
            : m_methId(methId)
            , m_threadId(threadId)
        {
        }

};

class MethodRecord : public Record
{
    private:
        MethodId_t m_methId;
        ObjectId_t m_objId;
        ThreadId_t m_threadId;

    public:
        MethodRecord( MethodId_t methId,
                      ObjectId_t objId,
                      ThreadId_t threadId )
            : m_methId(methId)
            , m_objId(objId)
            , m_threadId(threadId)
        {
        }

};

class UpdateRecord : public Record
{
    public:
        UpdateRecord() {}

};

class WitnessRecord : public Record
{
    public:
        WitnessRecord() {}

};

#endif // RECORD_HPP
