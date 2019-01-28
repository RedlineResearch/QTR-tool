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
        char m_record_type;
        VTime_t m_ET_timestamp;

    public:
        Record(char record_type, VTime_t et_timestamp)
            : m_record_type(record_type)
            , m_ET_timestamp(et_timestamp)
        {
        }

        inline char getRecordType() const
        {
            return this->m_record_type;
        }

        inline void set_ET_timestamp(VTime_t new_ts)
        {
            this->m_ET_timestamp = new_ts;
        }

        inline VTime_t get_ET_timestamp()
        {
            return this->m_ET_timestamp;
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
        unsigned int m_length;
        unsigned int m_size;
    public:
        // As documented in ETProxy.java:
        // <object-id> <size> <type-id> <site-id> <length> <thread-id>
        AllocRecord( ObjectId_t objectId,
                     SiteId_t siteId,
                     ThreadId_t threadId,
                     TypeId_t typeId,
                     unsigned int length,
                     unsigned int size,
                     VTime_t et_timestamp )
            : m_objectId(objectId)
            , m_siteId(siteId)
            , m_threadId(threadId)
            , m_typeId(typeId)
            , m_length(length)
            , m_size(size)
            , Record('A', et_timestamp)
        {
        }

        ObjectId_t getObjectId() { return this->m_objectId; }
        SiteId_t getSiteId() { return this->m_siteId; }
        ThreadId_t getThreadId() { return this->m_threadId; }
        TypeId_t getTypeId() { return this->m_typeId; }
        unsigned int getLength() { return this->m_length; }
        unsigned int getSize() { return this->m_size; }
};

class ExitRecord : public Record
{
    private:
        MethodId_t m_methId;
        ThreadId_t m_threadId;

    public:
        ExitRecord( MethodId_t methId,
                    ThreadId_t threadId,
                    VTime_t et_timestamp )
            : m_methId(methId)
            , m_threadId(threadId)
            , Record('E', et_timestamp)
        {
        }

        MethodId_t getMethId() { return this->m_methId; }
        ThreadId_t getThreadId() { return this->m_threadId; }
};

class MethodRecord : public Record
{
    private:
        MethodId_t m_methId;
        ObjectId_t m_objectId;
        ThreadId_t m_threadId;

    public:
        MethodRecord( MethodId_t methId,
                      ObjectId_t objectId,
                      ThreadId_t threadId,
                      VTime_t et_timestamp )
            : m_methId(methId)
            , m_objectId(objectId)
            , m_threadId(threadId)
            , Record('M', et_timestamp)
        {
        }

        MethodId_t getMethId() { return this->m_methId; }
        ObjectId_t getObjectId() { return this->m_objectId; }
        ThreadId_t getThreadId() { return this->m_threadId; }
};

class UpdateRecord : public Record
{
    // From ETProxy.java:
    // TODO: Conflicting documention: 2018-1112
    // 7, targetObjectHash, fieldID, srcObjectHash, timestamp
    // U <old-tgt-obj-id> <obj-id> <new-tgt-obj-id> <field-id> <thread-id>
    private:
        ObjectId_t m_tgtObjectHash;
        FieldId_t m_fieldId;
        ObjectId_t m_srcObjectHash;
        VTime_t m_timestamp;
    public:
        UpdateRecord( ObjectId_t tgtObjectHash,
                      FieldId_t fieldId,
                      ObjectId_t srcObjectHash,
                      VTime_t timestamp,
                      VTime_t et_timestamp )
            : m_tgtObjectHash(tgtObjectHash)
            , m_fieldId(fieldId)
            , m_srcObjectHash(srcObjectHash)
            , m_timestamp(timestamp)
            , Record('U', et_timestamp)
        {
        }

        ObjectId_t getTgtObjectHash() { return this->m_tgtObjectHash; }
        FieldId_t getFieldId() { return this->m_fieldId; }
        ObjectId_t getSrcObjectHash() { return this->m_srcObjectHash; }
        VTime_t getTimestamp() { return this->m_timestamp; }
};

class WitnessRecord : public Record
{
    // From ETProxy.java:
    // 8, aliveObjectHash, classID, timestamp
    private:
        ObjectId_t m_objectId;
        TypeId_t m_typeId;
        VTime_t m_timestamp;

    public:
        WitnessRecord( ObjectId_t objectId,
                       TypeId_t typeId,
                       VTime_t timestamp,
                       VTime_t et_timestamp )
            : m_objectId(objectId)
            , m_typeId(typeId)
            , m_timestamp(timestamp)
            , Record('U', et_timestamp)
        {
        }

        ObjectId_t getObjectId() { return this->m_objectId; }
        TypeId_t getTypeId() { return this->m_typeId; }
        VTime_t getTimestamp() { return this->m_timestamp; }
};

class DeathRecord : public Record
{
    private:
        ObjectId_t m_objectId;
        TypeId_t m_typeId;

    public:
        DeathRecord( ObjectId_t objectId,
                     TypeId_t typeId,
                     VTime_t et_timestamp )
            : m_objectId(objectId)
            , m_typeId(typeId)
            , Record('D', et_timestamp)
        {
        }

        ObjectId_t getObjectId() { return this->m_objectId; }
        TypeId_t getTypeId() { return this->m_typeId; }
};

#endif // RECORD_HPP
