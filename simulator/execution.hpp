#ifndef EXECUTION_H
#define EXECUTION_H

// ----------------------------------------------------------------------
//  Representation of runtime execution
//  stacks, threads, time

#include "classinfo.hpp"
#include "heap.hpp"

#include <iostream>
#include <map>
#include <deque>
#include <utility>

using namespace std;

class CCNode;
typedef map<unsigned int, CCNode *> CCMap;
typedef map<Method *, CCNode *> FullCCMap;

typedef map<ThreadId_t, Thread *> ThreadMap;
// TODO: This is the old context pair code:
typedef map<ContextPair, unsigned int> ContextPairCountMap;
typedef map<string, unsigned int> ContextCountMap;
typedef map<Object *, string> ObjectContextMap;


typedef deque<Method *> MethodDeque;
typedef deque<Thread *> ThreadDeque;
typedef set<Object *> LocalVarSet;
typedef deque<LocalVarSet *> LocalVarDeque;


// TODO typedef deque< pair<LastEvent, Object*> > LastEventDeque_t;
// TODO typedef map<threadId_t, LastEventDeque_t> LastMap_t;

// ----------------------------------------------------------------------
//   Calling context tree

// This is used to indicate the stack bookkeeping mode of class ExecState
enum class ExecMode {
    Full = 1,
    StackOnly = 2
};

std::ostream& operator << ( std::ostream &out, const ExecMode &value );


// Self-explanatory
enum class EventKind {
    Allocation = 1,
    Death = 2,
};


class CCNode
{
    private:
        Method* m_method;
        CCNode* m_parent;
        // -- Map from method IDs to callee contexts
        CCMap m_callees;
        // Flag indicating whether simple method trace has been saved
        bool m_done;
        // Caching the simple_stacktrace
        deque<Method *> m_simptrace;

        unsigned int m_node_id;
        static unsigned int m_ccnode_nextid;

        unsigned int get_next_node_id() {
            unsigned int result = this->m_ccnode_nextid;
            this->m_ccnode_nextid++;
            return result;
        }

    public:
        CCNode()
            : m_method(0)
            , m_parent(0)
            , m_done(false)
            , m_node_id(this->get_next_node_id()) {
        }

        CCNode( CCNode* parent, Method* m )
            : m_method(m)
            , m_parent(parent)
            , m_node_id(this->get_next_node_id()) {
        }

        // -- Get method
        Method* getMethod() const { return this->m_method; }

        // -- Get method
        MethodId_t getMethodId() const {
            Method *m = this->getMethod();
            return (m ? m->getId() : 0);
        }

        // -- Get parent context (if there is one)
        CCNode* getParent() const { return m_parent; }

        // -- Call a method, making a new child context if necessary
        CCNode* Call(Method* m);

        // -- Return from a method, returning the parent context
        CCNode* Return(Method* m);

        // -- Produce a string representation of the context
        string info();

        // -- Generate a stack trace
        string stacktrace();

        // -- Generate a stack trace and return the DequeId_t of function ids
        DequeId_t stacktrace_using_id();

        // Method name equality
        bool simple_cc_equal( CCNode &other );

        // TODO
        deque<Method *> simple_stacktrace();

        CCMap::iterator begin_callees() { return this->m_callees.begin(); }
        CCMap::iterator end_callees() { return this->m_callees.end(); }

        // Has simple trace been saved for this CCNode?
        bool isDone() { return this->m_done; }
        bool setDone() { this->m_done = true; }

        // Node Ids
        NodeId_t get_node_id() const { return this->m_node_id; }
};


// ----------------------------------------------------------------------
//   Thread representation

//  Two options for representing the stack
//    (1) Build a full calling context tree
//    (2) Keep a stack of methods
//  (I realize I could probably do this with fancy-dancy OO programming,
//  but sometimes that just seems like overkill

class ExecState; // forward declaration to include into Thread

class Thread
{
    private:
        // -- Thread ID
        unsigned int m_id;
        // -- Kind of stack
        ExecMode m_kind;
        // -- CC tree representation
        CCNode *m_curcc;
        // -- Stack of methods
        MethodDeque m_methods;
        // -- Local stack variables that have root events in this scope
        LocalVarDeque m_locals;
        // -- Local stack variables that have root events and died this scope
        LocalVarDeque m_deadlocals;
        // -- Current context pair
        ContextPair m_context;
        // -- Type of ContextPair m_context
        CPairType m_cptype;
        // -- Map of simple Allocation site (used to be context pair)
        //                     -> count of occurrences
        ContextCountMap &m_allocCountmap;
        // -- Map of simple Death death site (used to be context pair)
        //                     -> count of occurrences
        ContextPairCountMap &m_deathPairCountMap;
        // -- Map to ExecState
        ExecState &m_exec;

    public:
        Thread( unsigned int id,
                ExecMode kind,
                ContextCountMap &allocCountmap,
                ContextPairCountMap &deathCountMap,
                ExecState &execstate )
            : m_id(id)
            , m_kind(kind)
            , m_context( NULL, NULL )
            , m_cptype(CPairType::CP_None) 
            , m_allocCountmap(allocCountmap)
            , m_deathPairCountMap(deathCountMap)
            , m_exec(execstate) {
            m_locals.push_back(new LocalVarSet());
            m_deadlocals.push_back(new LocalVarSet());
        }

        unsigned int getId() const { return m_id; }

        // -- Call method m
        void Call(Method *m);
        // -- Return from method m
        void Return(Method* m);
        // -- Get current CC
        CCNode *TopCC();
        // -- Get current method
        Method *TopMethod();
        // -- Get method N from the top (0 based).
        //    This means the TopMethod() is equivalent to TopMethod(0)
        Method *TopMethod( unsigned int num );
        // -- Get top N methods
        MethodDeque top_N_methods(unsigned int N);
        // -- Get top N Java-library methods + first non-Java library method
        MethodDeque top_javalib_methods();
        // -- Get the full stack
        MethodDeque full_method_stack();
        // -- Get current dead locals
        LocalVarSet * TopLocalVarSet();
        // -- Get a stack trace
        string stacktrace();
        // -- Get a stack trace with method ids
        DequeId_t stacktrace_using_id();
        // -- Root event
        void objectRoot(Object *object);
        // -- Check dead object
        bool isLocalVariable(Object *object);
        // Root CCNode
        CCNode m_rootcc;
        // Get root node CC
        CCNode &getRootCCNode() { return m_rootcc; }
        // Get simple context pair
        ContextPair getContextPair() const { return m_context; }
        // Set simple context pair
        ContextPair setContextPair( ContextPair cpair, CPairType cptype ) {
            this->m_context = cpair;
            this->m_cptype = cptype;
            return cpair; 
        }
        // Get simple context pair type
        CPairType getContextPairType() const { return this->m_cptype; }
        // Set simple context pair type
        void setContextPairType( CPairType cptype ) { this->m_cptype = cptype; }

        // Debug
        void debug_cpair( ContextPair cpair,
                          string ptype ) {
            Method *m1 = std::get<0>(cpair);
            Method *m2 = std::get<1>(cpair);
            string method1 = (m1 ? m1->getName() : "NONAME1");
            string method2 = (m2 ? m2->getName() : "NONAME2");
            cout << "CPAIR-dbg< " << ptype << " >" 
                 << "[ " << method1 << ", " << method2 << "]" << endl;
        }
};

// ----------------------------------------------------------------------
// ----------------------------------------------------------------------
//   Execution state
// ----------------------------------------------------------------------

class ExecState
{
    private:
        // -- Stack kind (CC or methods)
        ExecMode m_kind;
        // -- Set of threads
        ThreadMap m_threads;
        // -- Method Time
        unsigned int m_meth_time;
        // -- Method Exit Time
        unsigned int m_methexit_time;
        // -- Update Time
        unsigned int m_uptime;
        // -- Alloc Time
        unsigned int m_alloc_time;
        // -- Map of Object pointer -> simple allocation context pair
        ObjectContextMap m_objAlloc2cmap;
        // -- Map of Object pointer -> simple death context pair
        ObjectContextMap m_objDeath2cmap;
        // Last method called
        ThreadDeque m_thread_stack;
        // When main function was called
        VTime_t main_func_uptime; // in logical update+method time
        VTime_t main_func_alloctime; // in logical allocation time (bytes)

    public:
        ExecState( ExecMode kind )
            : m_kind(kind)
            , m_threads()
            , m_meth_time(0)
            , m_uptime(0)
            , m_methexit_time(0)
            , m_alloc_time(0)
            , m_allocCountmap()
            , m_deathPairCountMap()
            , m_execPairCountMap()
            , m_objAlloc2cmap()
            , m_objDeath2cmap()
            , m_thread_stack() {
        }

        // -- Get the current method time
        unsigned int MethNow() const {
            return this->m_meth_time;
        }
        // -- Get the current method exit time
        unsigned int MethExitNow() const {
            return this->m_methexit_time;
        }

        // -- Get the current update time
        VTime_t NowUp() const {
            return this->m_uptime + this->m_methexit_time + this->m_meth_time;
        }

        // -- Get the current allocation time
        unsigned int NowAlloc() const { return m_alloc_time; }
        // -- Set the current allocation time
        void SetAllocTime( unsigned int newtime ) { this->m_alloc_time = newtime; }

        // -- Set the current update time
        inline unsigned int SetUpdateTime( unsigned int newutime ) {
            return this->m_uptime = newutime;
        }

        // -- Increment the current update time
        inline unsigned int IncUpdateTime() {
            return this->m_uptime++;
        }

        // -- Increment the current method time
        inline unsigned int IncMethodTime() {
            return this->m_meth_time++;
        }
        // -- Increment the current method EXIT time
        inline unsigned int IncMethodExitTime() {
            return this->m_methexit_time++;
        }

        // -- Look up or create a thread
        Thread* getThread(unsigned int threadid);

        // -- Call method m in thread t
        void Call(Method* m, unsigned int threadid);

        // -- Return from method m in thread t
        void Return(Method* m, unsigned int threadid);

        // -- Get the top method in thread t
        Method *TopMethod(unsigned int threadid);
        // -- Get n-th from the top method in thread t
        Method *TopMethod( unsigned int threadid,
                           unsigned int num );

        // -- Get the top calling context in thread t
        CCNode * TopCC(unsigned int threadid);

        // Get begin iterator of thread map
        ThreadMap::iterator begin_threadmap() { return this->m_threads.begin(); }
        ThreadMap::iterator end_threadmap() { return this->m_threads.end(); }

        // Update the Object pointer to simple Allocation context pair map
        void UpdateObj2AllocContext( Object *obj,
                                     string alloc_site )
        {
            this->m_objAlloc2cmap[obj] = alloc_site;
            // TODO: Old code using context pair
            // TODO ContextPair cpair,
            // TODO CPairType cptype ) {
            // TODO: UpdateObj2Context( obj,
            // TODO:                    cpair,
            // TODO:                    cptype,
            // TODO:                    EventKind::Allocation );
        }

        // Update the Object pointer to simple Death context pair map
        void UpdateObj2DeathContext( Object *obj,
                                     MethodDeque &methdeque );

        // -- Map of simple Allocation context -> count of occurrences
        // TODO: Think about hiding this in an abstraction TODO
        ContextCountMap m_allocCountmap;
        ContextCountMap::iterator begin_allocCountmap() { return this->m_allocCountmap.begin(); }
        ContextCountMap::iterator end_allocCountmap() { return this->m_allocCountmap.end(); }

        // -- Map of simple Death context -> count of occurrences
        // TODO: Think about hiding this in an abstraction TODO
        ContextPairCountMap m_deathPairCountMap;
        ContextPairCountMap::iterator begin_deathCountMap() {
            return this->m_deathPairCountMap.begin();
        }
        ContextPairCountMap::iterator end_deathCountMap() {
            return this->m_deathPairCountMap.end();
        }

        // -- Map of execution context Pair -> count of occurrences
        // TODO: Think about hiding this in an abstraction TODO
        ContextPairCountMap m_execPairCountMap;
        ContextPairCountMap::iterator begin_execPairCountMap() {
            return this->m_execPairCountMap.begin();
        }
        ContextPairCountMap::iterator end_execPairCountMap() {
            return this->m_execPairCountMap.end();
        }
        void IncCallContext( MethodDeque &top_2_methods ) {
            ContextPair cpair = std::make_pair( top_2_methods[0], top_2_methods[1] );
            this->m_execPairCountMap[cpair]++;
        }

        // Get last global thread called
        Thread *get_last_thread() const {
            return ( (this->m_thread_stack.size() > 0)
                     ? this->m_thread_stack.back()
                     : NULL );
        }

        ExecMode get_kind() const { return m_kind; }

        // Related to getting the time when the main function was called
        inline VTime_t get_main_func_uptime() {
            return this->main_func_uptime; // in logical update+method time
        }

        inline VTime_t get_main_func_alloctime() {
            return this->main_func_alloctime; // in logical allocation time (bytes)
        }

        inline void set_main_func_uptime(VTime_t new_uptime) {
            this->main_func_uptime = new_uptime; // in logical update+method time
        }

        inline VTime_t set_main_func_alloctime(VTime_t new_alloctime) {
            this->main_func_alloctime = new_alloctime; // in logical allocation time (bytes)
        }

    private:
        void debug_cpair( ContextPair cpair,
                          Object *object ) {
            Method *m1 = std::get<0>(cpair);
            Method *m2 = std::get<1>(cpair);
            string method1 = (m1 ? m1->getName() : "NONAME1");
            string method2 = (m2 ? m2->getName() : "NONAME2");
            cout << "CPAIR-update< " << object->getType() << " >"
                << "[ " << method1 << ", " << method2 << "]" << endl;
        }

};

#endif

