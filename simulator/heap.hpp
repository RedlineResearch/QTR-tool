#ifndef HEAP_H
#define HEAP_H

// ----------------------------------------------------------------------
//   Representation of objects on the heap
//
#include "classinfo.hpp"
// TODO #include "refstate.hpp"

#include <algorithm>
#include <iostream>
#include <map>
#include <deque>
#include <limits.h>
#include <assert.h>
#include <boost/logic/tribool.hpp>
#include <boost/bimap.hpp>

class Object;
class Thread;
class Edge;

enum class Reason
    : std::uint8_t {
    STACK,
    HEAP,
    GLOBAL,
    END_OF_PROGRAM_REASON,
    UNKNOWN_REASON
};

enum class LastEvent
    : std::uint16_t {
    NEWOBJECT,
    ROOT,
    DECRC,
    UPDATE_UNKNOWN,
    UPDATE_AWAY_TO_NULL,
    UPDATE_AWAY_TO_VALID,
    OBJECT_DEATH_AFTER_ROOT,
    OBJECT_DEATH_AFTER_UPDATE,
    OBJECT_DEATH_AFTER_ROOT_DECRC, // from OBJECT_DEATH_AFTER_ROOT
    OBJECT_DEATH_AFTER_UPDATE_DECRC, // from OBJECT_DEATH_AFTER_UPDATE
    OBJECT_DEATH_AFTER_UNKNOWN,
    END_OF_PROGRAM_EVENT,
    UNKNOWN_EVENT,
};

string lastevent2str( LastEvent le );
bool is_object_death( LastEvent le );

enum DecRCReason {
    UPDATE_DEC = 2,
    DEC_TO_ZERO = 7,
    END_OF_PROGRAM_DEC = 8,
    UNKNOWN_DEC_EVENT = 99,
};

enum class KeyType {
    DAG = 1,
    DAGKEY = 2,
    CYCLE = 3,
    CYCLEKEY = 4,
    UNKNOWN_KEYTYPE = 99,
};

string keytype2str( KeyType ktype );

enum class CPairType
    : std::uint8_t {
    CP_Call,
    CP_Return,
    CP_Both,
    CP_None
};


typedef std::map<ObjectId_t, Object *> ObjectMap;
typedef std::map<FieldId_t, Edge *> EdgeMap;
typedef set<Object *> ObjectSet;
typedef set<Edge *> EdgeSet;
typedef deque< pair<int, int> > EdgeList;
typedef std::map< Object *, std::set< Object * > * > KeySet_t;


typedef std::map<Method *, set<string> *> DeathSitesMap;
// Where we save the method death sites. This has to be method pointer
// to:
// 1) set of object pointers
// 2) set of object types
// 2 seems better.
using namespace boost;
using namespace boost::logic;

typedef std::pair<int, int> GEdge_t;
typedef unsigned int  NodeId_t;
typedef std::map<int, int> Graph_t;
typedef std::map<Object *, Object *> ObjectPtrMap_t;
// TODO do we need to distinguish between FOUND and LOOKING?
// enum KeyStatus {
//     LOOKING = 1,
//     FOUND = 2,
//     UNKNOWN_STATUS = 99,
// };
// typedef std::map<ObjectId_t, std::pair<KeyStatus, ObjectId_t>> LookupMap;
//

// BiMap_t is used to map:
//     key object <-> some object
// where:
//     key objects map to themselves
//     non key objects to their root key objects
typedef bimap<ObjectId_t, ObjectId_t> BiMap_t;


struct compclass {
    bool operator() ( const std::pair< ObjectId_t, unsigned int >& lhs,
                      const std::pair< ObjectId_t, unsigned int >& rhs ) const {
        return lhs.second > rhs.second;
    }
};

class HeapState {
    friend class Object;

    private:
        // -- Map from IDs to objects
        ObjectMap m_objects;
        // Live set
        ObjectSet m_liveset;

        unsigned long int m_liveSize; // current live size of program in bytes
        unsigned long int m_maxLiveSize; // max live size of program in bytes
        unsigned int m_alloc_time; // current alloc time

        // Total number of objects that died by loss of heap reference version 2
        unsigned int m_totalDiedByHeap_ver2;
        // Total number of objects that died by stack frame going out of scope version 2
        unsigned int m_totalDiedByStack_ver2;
        // Total number of objects that died by loss of global/static reference
        unsigned int m_totalDiedByGlobal;
        // Total number of objects that live till the end of the program
        unsigned int m_totalDiedAtEnd;
        // Total number of objects unknown using version 2 method
        unsigned int m_totalDiedUnknown_ver2;
        // Size of objects that died by loss of heap reference
        unsigned int m_sizeDiedByHeap;
        // Size of objects that died by loss of global heap reference
        unsigned int m_sizeDiedByGlobal;
        // Size of objects that died by stack frame going out of scope
        unsigned int m_sizeDiedByStack;
        // Size of objects that live till the end of the program
        unsigned int m_sizeDiedAtEnd;

        // Total number of objects whose last update away from the object
        // was null
        unsigned int m_totalUpdateNull;
        //    -- that was part of the heap
        unsigned int m_totalUpdateNullHeap;
        //    -- that was part of the stack
        unsigned int m_totalUpdateNullStack;
        // Size of objects whose last update away from the object was null
        unsigned int m_totalUpdateNull_size;
        //    -- that was part of the heap
        unsigned int m_totalUpdateNullHeap_size;
        //    -- that was part of the stack
        unsigned int m_totalUpdateNullStack_size;

        // Died by stack with previous heap action
        unsigned int m_diedByStackAfterHeap;
        // Died by stack only
        unsigned int m_diedByStackOnly;
        // Died by stack with previous heap action -- size
        unsigned int m_diedByStackAfterHeap_size;
        // Died by stack only -- size
        unsigned int m_diedByStackOnly_size;

        // Number of objects with no death sites
        unsigned int m_no_dsites_count;
        // Number of VM objects that always have RC 0
        unsigned int m_vm_refcount_0;
        // Number of VM objects that have RC > 0 at some point
        unsigned int m_vm_refcount_positive;

        // Map of Method * to set of object type names
        DeathSitesMap m_death_sites_map;
        
        // Map of object to key object (in pointers)
        ObjectPtrMap_t& m_whereis;

        // Map of key object to set of objects
        KeySet_t& m_keyset;

        void update_death_counters( Object *obj );
        Method * get_method_death_site( Object *obj );

        NodeId_t getNodeId( ObjectId_t objId, bimap< ObjectId_t, NodeId_t >& bmap );

    public:

        HeapState( ObjectPtrMap_t& whereis, KeySet_t& keyset )
            : m_objects()
            , m_liveset()
            , m_death_sites_map()
            , m_whereis( whereis )
            , m_keyset( keyset )
            , m_maxLiveSize(0)
            , m_liveSize(0)
            , m_alloc_time(0)
            , m_totalDiedByHeap_ver2(0)
            , m_totalDiedByStack_ver2(0)
            , m_totalDiedByGlobal(0)
            , m_totalDiedAtEnd(0)
            , m_sizeDiedByHeap(0)
            , m_sizeDiedByGlobal(0)
            , m_sizeDiedByStack(0)
            , m_sizeDiedAtEnd(0)
            , m_totalDiedUnknown_ver2(0)
            , m_totalUpdateNull(0)
            , m_totalUpdateNullHeap(0)
            , m_totalUpdateNullStack(0)
            , m_totalUpdateNull_size(0)
            , m_totalUpdateNullHeap_size(0)
            , m_totalUpdateNullStack_size(0)
            , m_diedByStackAfterHeap(0)
            , m_diedByStackOnly(0)
            , m_no_dsites_count(0)
            , m_vm_refcount_positive(0)
            , m_vm_refcount_0(0)
            , m_obj_debug_flag(false) {
        }

        void enableObjectDebug() { m_obj_debug_flag = true; }
        void disableObjectDebug() { m_obj_debug_flag = false; }

        Object *allocate( ObjectId_t id,
                          unsigned int size,
                          char kind,
                          string type,
                          AllocSite *site, 
                          string &nonjava_site_name,
                          unsigned int els,
                          Thread *thread,
                          bool new_flag,
                          unsigned int create_time );
        // TODO: DOC
        Object *allocate( Object * objptr );

        Object *getObject( unsigned int id );

        Edge *make_edge( Object *source,
                         unsigned int field_id,
                         Object *target,
                         unsigned int cur_time );

        Edge *make_edge( Edge *edgeptr );

        void makeDead( Object * obj,
                       unsigned int death_time,
                       ofstream &eifile );
        void makeDead_nosave( Object * obj,
                              unsigned int death_time,
                              ofstream &eifile );
        void __makeDead( Object * obj,
                         unsigned int death_time,
                         ofstream *eifile_ptr );

        ObjectMap::iterator begin() { return m_objects.begin(); }
        ObjectMap::iterator end() { return m_objects.end(); }
        unsigned int size() const { return m_objects.size(); }
        unsigned long int liveSize() const { return m_liveSize; }
        unsigned long int maxLiveSize() const { return m_maxLiveSize; }

        unsigned int getAllocTime() const { return m_alloc_time; }
        void incAllocTime( unsigned int inc ) {
            this->m_alloc_time += inc;
        }

        unsigned int getTotalDiedByStack2() const { return m_totalDiedByStack_ver2; }
        unsigned int getTotalDiedByHeap2() const { return m_totalDiedByHeap_ver2; }
        unsigned int getTotalDiedByGlobal() const { return m_totalDiedByGlobal; }
        unsigned int getTotalDiedAtEnd() const { return m_totalDiedAtEnd; }
        unsigned int getTotalDiedUnknown() const { return m_totalDiedUnknown_ver2; }
        unsigned int getSizeDiedByHeap() const { return m_sizeDiedByHeap; }
        unsigned int getSizeDiedByStack() const { return m_sizeDiedByStack; }
        unsigned int getSizeDiedAtEnd() const { return m_sizeDiedAtEnd; }

        unsigned int getTotalLastUpdateNull() const { return m_totalUpdateNull; }
        unsigned int getTotalLastUpdateNullHeap() const { return m_totalUpdateNullHeap; }
        unsigned int getTotalLastUpdateNullStack() const { return m_totalUpdateNullStack; }
        unsigned int getSizeLastUpdateNull() const { return m_totalUpdateNull_size; }
        unsigned int getSizeLastUpdateNullHeap() const { return m_totalUpdateNullHeap_size; }
        unsigned int getSizeLastUpdateNullStack() const { return m_totalUpdateNullStack_size; }

        unsigned int getDiedByStackAfterHeap() const { return m_diedByStackAfterHeap; }
        unsigned int getDiedByStackOnly() const { return m_diedByStackOnly; }
        unsigned int getSizeDiedByStackAfterHeap() const { return m_diedByStackAfterHeap_size; }
        unsigned int getSizeDiedByStackOnly() const { return m_diedByStackOnly_size; }

        unsigned int getNumberNoDeathSites() const { return m_no_dsites_count; }

        unsigned int getVMObjectsRefCountZero() const { return m_vm_refcount_0; }
        unsigned int getVMObjectsRefCountPositive() const { return m_vm_refcount_positive; }

        DeathSitesMap::iterator begin_dsites() { return m_death_sites_map.begin(); }
        DeathSitesMap::iterator end_dsites() { return m_death_sites_map.end(); }

        // End of program event handling
        void end_of_program( unsigned int cur_time,
                             ofstream &edge_info_file );
        void end_of_program_nosave( unsigned int cur_time );
        void __end_of_program( unsigned int cur_time,
                               ofstream *eifile_ptr,
                               bool save_edge_flag );

        void set_candidate(unsigned int objId);
        void unset_candidate(unsigned int objId);
        deque< deque<int> > scan_queue( EdgeList& edgelist );
        void set_reason_for_cycles( deque< deque<int> >& cycles );

        ObjectPtrMap_t& get_whereis() { return m_whereis; }
        KeySet_t& get_keyset() { return m_keyset; }

        // -- Turn on debugging
        static bool debug;
        // -- Turn on output of objects to stdout
        bool m_obj_debug_flag;
};

enum class ObjectRefType
    : std::uint8_t {
    SINGLY_OWNED, // Only one incoming reference ever which 
                      // makes the reference RefType::SERIAL_STABLE
    MULTI_OWNED, // Gets passed around to difference edge sources
    UNKNOWN
};

enum class EdgeState 
    : std::uint8_t {
    NONE = 1,
    LIVE = 2,
    DEAD_BY_UPDATE = 3,
    DEAD_BY_OBJECT_DEATH = 4,
    DEAD_BY_PROGRAM_END = 5,
    DEAD_BY_OBJECT_DEATH_NOT_SAVED = 6,
    DEAD_BY_PROGRAM_END_NOT_SAVED = 7,
    // The not SAVED versions means that we haven't written the
    // edge out to the edgeinfo file yet.
};


class Object {
    private:
        // Object id
        unsigned int m_id;
        // Size in bytes
        unsigned int m_size;
        // Allocation type(?)
        char m_kind;
        // Java type/class
        string m_type;
        // Allocation sites
        AllocSite *m_site;
        string m_allocsite_name;
        string m_nonjavalib_allocsite_name;

        // AKA length of array if it is one
        unsigned int m_elements;
        // Thread object this object belongs to
        Thread *m_thread;

        // Time object was created (method time)
        unsigned int m_createTime;
        // Death time - set to 0 to start. Then adjusted by Merlin algorithm.
        unsigned int m_deathTime;
        // Time object was created (allocation time)
        unsigned int m_createTime_alloc;
        // Death time - set to 0 to start. Then adjusted by Merlin algorithm.
        unsigned int m_deathTime_alloc;

        EdgeMap m_fields;

        // Created via new?
        bool m_newFlag;

        // Pointer back to the heap
        HeapState *m_heapptr;
        bool m_deadFlag;

        // Merlin algorithm fields:
        // Time of last update away. This is the 'timestamp' in the Merlin algorithm
        unsigned int m_last_timestamp;

        // ==[ Death reasons ]==============================================================
        // Was this object ever a target of a heap pointer?
        bool m_pointed_by_heap;
        // Did last update move to NULL?
        tribool m_last_update_null; // If false, it moved to a differnet object
        // Was last update away from this object from a static field?
        bool m_last_update_away_from_static;
        // Did this object die by loss of heap reference?
        bool m_diedByHeap;
        // Did this object die by loss of stack reference?
        bool m_diedByStack;
        // Did this object die because the program ended?
        bool m_diedAtEnd;
        // Did this object die because of an update away from a global/static variable?
        bool m_diedByGlobal;

        // Has the diedBy***** flag been set?
        // This is for debug.
        bool m_diedFlagSet;

        // Reason for death
        Reason m_reason;
        // Time that m_reason happened
        unsigned int m_last_action_time;
        // Last action method
        Method *m_last_action_method;
        // Method where this object died
        Method *m_methodDeathSite; // level 1
        Method *m_methodDeathSite_l2; // level 2

        // METHOD 2: Use Elephant Track events instead
        LastEvent m_last_event;
        Object *m_last_object;

        string m_deathsite_name;
        string m_deathsite_name_l2;
        string m_nonjavalib_death_context;
        string m_nonjavalib_last_action_context;

        DequeId_t m_alloc_context;
        // If ExecMode is Full, this contains the full list of the stack trace at allocation.
        DequeId_t m_death_context;
        // If ExecMode is Full, this contains the full list of the stack trace at death.

        // Who's my key object? 0 means unassigned.
        Object *m_death_root;
        KeyType m_key_type;

        // Stability type
        ObjectRefType m_reftarget_type;

    public:
        Object( unsigned int id,
                unsigned int size,
                char kind,
                string type,
                AllocSite* site,
                string &nonjava_site_name,
                unsigned int els,
                Thread* thread,
                unsigned int create_time,
                bool new_flag,
                HeapState* heap )
            : m_id(id)
            , m_size(size)
            , m_kind(kind)
            , m_type(type)
            , m_site(site)
            , m_nonjavalib_allocsite_name(nonjava_site_name)
            , m_elements(els)
            , m_thread(thread)
            , m_deadFlag(false)
            , m_newFlag(new_flag)
            , m_createTime(create_time)
            , m_deathTime(UINT_MAX)
            , m_createTime_alloc( heap->getAllocTime() )
            , m_deathTime_alloc(UINT_MAX)
            , m_heapptr(heap)
            , m_pointed_by_heap(false)
            , m_diedByHeap(false)
            , m_diedByStack(false)
            , m_diedAtEnd(false)
            , m_diedByGlobal(false)
            , m_diedFlagSet(false)
            , m_reason(Reason::UNKNOWN_REASON)
            , m_last_action_time(0)
            , m_last_timestamp(0)
            , m_last_update_null(indeterminate)
            , m_last_update_away_from_static(false)
            , m_last_action_method(NULL)
            , m_methodDeathSite(0)
            , m_methodDeathSite_l2(0)
            , m_last_event(LastEvent::UNKNOWN_EVENT)
            , m_death_root(NULL)
            , m_last_object(NULL)
            , m_key_type(KeyType::UNKNOWN_KEYTYPE)
            , m_deathsite_name("NONE")
            , m_deathsite_name_l2("NONE")
            , m_nonjavalib_death_context("NONE")
            , m_nonjavalib_last_action_context("NONE")
            , m_reftarget_type(ObjectRefType::UNKNOWN)
        {
            // Allocation site
            if (site) {
                Method *mymeth = site->getMethod();
                this->m_allocsite_name = (mymeth ? mymeth->getName() : "NONAME");
            } else {
                this->m_allocsite_name = "NONAME";
            }
        }

        // -- Getters
        inline unsigned int getId() const
        {
            return this->m_id;
        }

        inline unsigned int getSize() const
        {
            return this->m_size;
        }

        inline const string& getType() const
        {
            return this->m_type;
        }

        inline char getKind() const
        {
            return this->m_kind;
        }
        // Allocation sites
        AllocSite * getAllocSite() const {
            return m_site;
        }
        string getAllocSiteName() const {
            return m_allocsite_name;
        }
        string getNonJavaLibAllocSiteName() const {
            return this->m_nonjavalib_allocsite_name;
        }
        void setNonJavaLibAllocSiteName( string &newname ) {
            this->m_nonjavalib_allocsite_name = newname;
        }

        Thread * getThread() const { return m_thread; }
        VTime_t getCreateTime() const { return m_createTime; }
        VTime_t getDeathTime() const {
            return m_deathTime;
        }
        void setDeathTime( VTime_t new_deathtime );

        VTime_t getCreateTimeAlloc() const { return this->m_createTime_alloc; }
        VTime_t getDeathTimeAlloc() const { return m_deathTime_alloc; }
        EdgeMap::iterator const getEdgeMapBegin() { return m_fields.begin(); }
        EdgeMap::iterator const getEdgeMapEnd() { return m_fields.end(); }
        Edge * getEdge(FieldId_t fieldId) const;
        bool isDead() const { return m_deadFlag; }

        bool isNewedObject() const { return m_newFlag; }
        bool setNewedObject(bool flag) { m_newFlag = flag; return m_newFlag; }

        bool wasPointedAtByHeap() const { return m_pointed_by_heap; }
        void setPointedAtByHeap() { m_pointed_by_heap = true; }

        // ==================================================
        // The diedBy***** flags
        // - died by STACK
        bool getDiedByStackFlag() const { return m_diedByStack; }
        void setDiedByStackFlag() {
            // REMOVE this DEBUG for now
            // TODO if (this->m_diedFlagSet) {
            // TODO     // Check to see if different
            // TODO     if ( this->m_diedByHeap || this->m_diedByGlobal) {
            // TODO         cerr << "Object[" << this->m_id << "]"
            // TODO              << " was originally died by heap. Overriding." << endl;
            // TODO     }
            // TODO }
            this->m_diedByStack = true;
            this->m_reason = Reason::STACK;
            this->m_diedFlagSet = true;
        }
        void unsetDiedByStackFlag() { m_diedByStack = false; }
        void setStackReason( unsigned int t ) { m_reason = Reason::STACK; m_last_action_time = t; }
        // -----------------------------------------------------------------
        // - died by HEAP 
        bool getDiedByHeapFlag() const { return m_diedByHeap; }
        void setDiedByHeapFlag() {
            // REMOVE this DEBUG for now
            // TODO if (this->m_diedFlagSet) {
            // TODO     // Check to see if different
            // TODO     if (this->m_diedByStack) {
            // TODO         cerr << "Object[" << this->m_id << "]"
            // TODO              << " was originally died by stack. Overriding." << endl;
            // TODO     }
            // TODO }
            this->m_diedByHeap = true;
            this->m_reason = Reason::HEAP;
            this->m_diedFlagSet = true;
        }
        void unsetDiedByHeapFlag() { m_diedByHeap = false; }
        // -----------------------------------------------------------------
        // - died by GLOBAL
        bool getDiedByGlobalFlag() const { return m_diedByGlobal; }
        void setDiedByGlobalFlag() {
            this->m_diedByGlobal = true;
            this->m_reason = Reason::GLOBAL;
            this->m_diedFlagSet = true;
        }
        void unsetDiedByGlobalFlag() { m_diedByGlobal = false; }
        // -----------------------------------------------------------------
        // - died at END
        bool getDiedAtEndFlag() const { return m_diedAtEnd; }
        void setDiedAtEndFlag() {
            if (this->m_diedFlagSet) {
                cerr << "Object[" << this->m_id << "]"
                     << " was has died by " << this->flagName()
                     <<  "flag set. NOT overriding." << endl;
                return;
            }
            this->m_diedAtEnd = true;
            this->m_reason = Reason::END_OF_PROGRAM_REASON;
            this->m_diedFlagSet = true;
        }
        void unsetDiedAtEndFlag() { m_diedAtEnd = false; }
        bool isDiedFlagSet() { return this->m_diedFlagSet; }
        // -----------------------------------------------------------------
        string flagName() {
            return ( this->getDiedByHeapFlag() ? "HEAP" : 
                     ( this->getDiedByStackFlag() ? "STACK" :
                       ( this->getDiedAtEndFlag() ? "END" :
                         "OTHER" ) ) );
        }
        // -----------------------------------------------------------------

        void setHeapReason( unsigned int t ) { m_reason = Reason::HEAP; m_last_action_time = t; }
        Reason setReason( Reason r, unsigned int t ) { m_reason = r; m_last_action_time = t; return m_reason; }
        Reason getReason() const { return m_reason; }
        // Last action related
        unsigned int getLastActionTime() const {
            return m_last_action_time;
        }
        Method *getLastActionSite() const {
            return this->m_last_action_method;
        }
        void setLastActionSite( Method *newsite ) {
            this->m_last_action_method = newsite;
        }

        // Return the Merlin timestamp
        auto getLastTimestamp() -> unsigned int const {
            return this->m_last_timestamp;
        }
        // Set the Merlin timestamp
        void setLastTimestamp( unsigned int new_ts ) {
            this->m_last_timestamp = new_ts;
        }

        // Returns whether last update to this object was NULL.
        // If indeterminate, then there have been no updates
        tribool wasLastUpdateNull() const { return m_last_update_null; }
        // Set the last update null flag to true
        void setLastUpdateNull() { m_last_update_null = true; }
        // Set the last update null flag to false
        void unsetLastUpdateNull() { m_last_update_null = false; }
        // Check if last update was from static
        bool wasLastUpdateFromStatic() const { return m_last_update_away_from_static; }
        // Set the last update from static flag to true
        void setLastUpdateFromStatic() { m_last_update_away_from_static = true; }
        // Set the last update from static flag to false
        void unsetLastUpdateFromStatic() { m_last_update_away_from_static = false; }
        // Get the death site according the the Death event
        Method *getDeathSite() const { return this->m_methodDeathSite; }
        // Get the death site according the the Death event using the level
        Method *getL1DeathSite() const { return this->m_methodDeathSite; }
        Method *getL2DeathSite() const { return this->m_methodDeathSite_l2; }
        // Set the death site because of a Death event
        void setDeathSite(Method * method) {
            // Assumes first level only
            this->m_methodDeathSite = method;
        }
        void setDeathSite( Method * method,
                           unsigned int count ) {
            if (count == 1) {
                this->m_methodDeathSite = method;
            } else if (count == 2) {
                this->m_methodDeathSite_l2 = method;
            } else {
                assert(false);
            }
        }

        // Set and get last event
        void setLastEvent( LastEvent le ) { m_last_event = le; }
        LastEvent getLastEvent() const { return m_last_event; }
        // Set and get last Object 
        void setLastObject( Object *obj ) { m_last_object = obj; }
        Object * getLastObject() const { return m_last_object; }
        // Set and get death root
        void setDeathRoot( Object *newroot ) { this->m_death_root = newroot; }
        Object * getDeathRoot() const { return this->m_death_root; }
        // Set and get key type 
        void setKeyType( KeyType newtype )
        {
            this->m_key_type = newtype;
        }
        void setKeyTypeIfNotKnown( KeyType newtype )
        {
            if (this->m_key_type == KeyType::UNKNOWN_KEYTYPE) {
                this->m_key_type = newtype;
            }
            // else {
            //     // TODO: Log some debugging.
            //     cerr << "Object[ " << this->m_id
            //          << "] keytype prev[ " << keytype2str(this->m_key_type)
            //          << "] new [ " << keytype2str(newtype) << "]"
            //          << endl;
            // }
        }
        KeyType getKeyType() const { return this->m_key_type; }

        // Set and get stability taret types
        void setRefTargetType( ObjectRefType newtype ) { this->m_reftarget_type = newtype; }
        ObjectRefType getRefTargetType() const { return this->m_reftarget_type; }


        // First level death context
        // Getter
        string getDeathContextSiteName( unsigned int level ) const
        {
            // Method *mymeth = this->m_methodDeathSite;
            // return (mymeth ? mymeth->getName() : "NONAME");
            if (level == 1) {
                return this->m_deathsite_name;
            } else if (level == 2) {
                return this->m_deathsite_name_l2;
            } else {
                assert(false);
            }
        }
        // Setter
        void setDeathContextSiteName( string &new_dsite,
                                      unsigned int level ) {
            if (level == 1) {
                this->m_deathsite_name = new_dsite;
            } else if (level == 2) {
                this->m_deathsite_name_l2 = new_dsite;
            } else {
                assert(false);
            }
        }


        string get_nonJavaLib_death_context() const
        {
            return this->m_nonjavalib_death_context;
        }

        void set_nonJavaLib_death_context( string &new_dcontext )
        {
            this->m_nonjavalib_death_context = new_dcontext;
        }

        string get_nonJavaLib_last_action_context() const
        {
            return this->m_nonjavalib_last_action_context;
        }

        void set_nonJavaLib_last_action_context( string &new_dcontext )
        {
            this->m_nonjavalib_last_action_context = new_dcontext;
        }

        // Set allocation context list
        void setAllocContextList( DequeId_t acontext_list ) {
            this->m_alloc_context = acontext_list;
        }
        // Get allocation context type
        DequeId_t getAllocContextList() const {
            return this->m_alloc_context;
        }

        // Set death context list
        void setDeathContextList( DequeId_t dcontext_list ) {
            this->m_alloc_context = dcontext_list;
        }

        // Get death context type
        DequeId_t getDeathContextList() const {
            return this->m_death_context;
        }


        // -- Access the fields
        const EdgeMap& getFields() const { return m_fields; }
        // -- Get a string representation
        string info();
        // -- Get a string representation for a dead object
        string info2();
        // -- Check live
        bool isLive(unsigned int tm) const {
            return (this->m_deathTime >= tm);
        }
        // ET2 version
        void updateField( Edge *edge, FieldId_t fieldId );

#if 0 // ET1 version
        // -- Update a field
        void updateField_save( Edge *edge,
                               FieldId_t fieldId,
                               unsigned int cur_time,
                               Method *method,
                               Reason reason,
                               Object *death_root,
                               LastEvent last_event,
                               ofstream &eifile );
        // -- Update a field
        void updateField( Edge* edge,
                          FieldId_t fieldId,
                          unsigned int cur_time,
                          Method *method,
                          Reason reason,
                          Object *death_root,
                          LastEvent last_event );
        // -- Update a field
        void __updateField( Edge* edge,
                            FieldId_t fieldId,
                            unsigned int cur_time,
                            Method *method,
                            Reason reason,
                            Object *death_root,
                            LastEvent last_event,
                            ofstream *eifile_ptr );
#endif // ET1 version END
        // -- Record death time
        void makeDead( unsigned int death_time,
                       unsigned int death_time_alloc,
                       ofstream &eifile,
                       Reason newreason );
        void makeDead_nosave( unsigned int death_time,
                              unsigned int death_time_alloc,
                              EdgeState estate,
                              ofstream &eifile,
                              Reason newreason );
        void __makeDead( unsigned int death_time,
                         unsigned int death_time_alloc,
                         ofstream *eifile_ptr,
                         Reason newreason,
                         bool save_edge_flag );

        // Global debug counter
        static unsigned int g_counter;
};

class Edge {
    private:
        // -- Source object
        Object *m_source;
        // -- Source field
        unsigned int m_sourceField;
        // -- Target object
        Object *m_target;
        // -- Creation time
        unsigned int m_createTime;
        // -- End time
        //    If 0 then ends when source object dies
        unsigned int m_endTime;
        // Died with source? (tribool state)
        // MAYBE == Unknown
        tribool m_died_with_source;
        // Flag on whether edge has been outputed to edgeinfo file
        bool m_output_done;

    public:
        Edge( Object *source, unsigned int field_id,
              Object *target, unsigned int cur_time )
            : m_source(source)
            , m_sourceField(field_id)
            , m_target(target)
            , m_createTime(cur_time)
            , m_endTime(0)
            , m_died_with_source(indeterminate)
            , m_output_done(false) {
        }

        Object *getSource() const {
            return m_source;
        }

        Object *getTarget() const {
            return m_target;
        }

        FieldId_t getSourceField() const {
            return m_sourceField;
        }

        unsigned int getCreateTime() const {
            return m_createTime;
        }

        unsigned int getEndTime() const {
            return m_endTime;
        }

        void setEndTime(unsigned int end) {
            m_endTime = end;
        }

        // Flag on whether edge has been sent to edgeinfo output file
        //   setter/getter
        void setOutputFlag( bool flag ) {
            this->m_output_done = flag;
        }
        bool getOutputFlag() const {
            return this->m_output_done;
        }
};


#endif
