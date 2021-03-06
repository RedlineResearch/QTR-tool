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
    protected:
        ThreadId_t m_threadId;
        char m_record_type;
        VTime_t m_ET_timestamp;

    public:
        Record(char record_type, VTime_t et_timestamp, ThreadId_t thread_id)
            : m_record_type(record_type)
            , m_ET_timestamp(et_timestamp)
            , m_threadId(thread_id)
        {
        }

        inline char getRecordType() const { return this->m_record_type; }

        inline void set_ET_timestamp(VTime_t new_ts) { this->m_ET_timestamp = new_ts; }

        inline VTime_t get_ET_timestamp() const { return this->m_ET_timestamp; }

        inline ThreadId_t getThreadId() const { return this->m_threadId; }

        static bool s_debug_flag;
        static void setDebug(bool flag) { Record::s_debug_flag = flag; }
        static bool isDebugOn() { return Record::s_debug_flag; }
};

bool Record::s_debug_flag = false;

// Derived record classes
class AllocRecord : public Record
{
    private:
        ObjectId_t m_objectId;
        SiteId_t m_siteId;
        TypeId_t m_typeId;
        unsigned int m_length;
        unsigned int m_size;
        int m_num_dims;
        deque<int> dims;
    public:
        // As documented in ETProxy.java:
        // <object-id> <size> <type-id> <site-id> <length> <thread-id>
        AllocRecord( ObjectId_t objectId,
                     SiteId_t siteId,
                     ThreadId_t threadId,
                     TypeId_t typeId,
                     unsigned int length,
                     unsigned int size,
                     char rec_type,
                     string dims,
                     VTime_t et_timestamp )
            : m_objectId(objectId)
            , m_siteId(siteId)
            , m_typeId(typeId)
            , m_length(length)
            , m_size(size)
            , Record(rec_type, et_timestamp, threadId)
        {
            if (rec_type == 'A') {
                string numdims_str = dims.substr(0, dims.find(','));
                if (Record::isDebugOn()) {
                    cerr << "[DBG " << rec_type << " record]: ojbId=\"" << objectId << "\", typeId=\"" << typeId << "\", ";
                    cerr << "size=\"" << size << "\", length=\"" << length << "\", ";
                    cerr << "token=\"" << numdims_str << "\", fullstr=\"" << dims << "\"" << endl;
                }
                if (numdims_str != "0") {
                    this->m_num_dims = stoi(numdims_str, NULL, 10);
                }
            } else {
                this->m_num_dims = 0;
            }
        }

        ObjectId_t getObjectId() { return this->m_objectId; }
        SiteId_t getSiteId() { return this->m_siteId; }
        TypeId_t getTypeId() { return this->m_typeId; }
        unsigned int getLength() { return this->m_length; }
        unsigned int getSize() { return this->m_size; }
        int getNumDims() { return this->m_num_dims; }
        deque<int> getDims() { return this->dims; } 
};

class ExitRecord : public Record
{
    private:
        MethodId_t m_methId;

    public:
        ExitRecord( MethodId_t methId,
                    ThreadId_t threadId,
                    VTime_t et_timestamp )
            : m_methId(methId)
            , Record('E', et_timestamp, threadId)
        {
        }

        MethodId_t getMethId() { return this->m_methId; }
};

class MethodRecord : public Record
{
    private:
        MethodId_t m_methId;
        ObjectId_t m_objectId;

    public:
        MethodRecord( MethodId_t methId,
                      ObjectId_t objectId,
                      ThreadId_t threadId,
                      VTime_t et_timestamp )
            : m_methId(methId)
            , m_objectId(objectId)
            , Record('M', et_timestamp, threadId)
        {
        }

        MethodId_t getMethId() { return this->m_methId; }
        ObjectId_t getObjectId() { return this->m_objectId; }
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
                      ThreadId_t thread_id,
                      VTime_t et_timestamp )
            : m_tgtObjectHash(tgtObjectHash)
            , m_fieldId(fieldId)
            , m_srcObjectHash(srcObjectHash)
            , Record('U', et_timestamp, thread_id)
        {
            if (Record::isDebugOn()) {
                cerr << "[DBG U record]: srcOjbId=\"" << srcObjectHash << "\", tgtObjId=\"" << tgtObjectHash << "\", ";
                cerr << "fieldId=\"" << fieldId << "\", threadId=\"" << thread_id << "\"" << endl;
            }
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
            , Record('U', et_timestamp, 0) // TODO: What is the thread_id for a witness event?
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
            , Record('D', et_timestamp, 0) // TODO: What is the thread_id for a death event?
        {
        }

        ObjectId_t getObjectId() { return this->m_objectId; }
        TypeId_t getTypeId() { return this->m_typeId; }
};

#endif // RECORD_HPP
