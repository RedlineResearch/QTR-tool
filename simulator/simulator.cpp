using namespace std;

#include "classinfo.hpp"
#include "execution.hpp"
#include "heap.hpp"
// #include "lastmap.h"
#include "record.hpp"
#include "refstate.hpp"
#include "summary.hpp"
#include "tokenizer.hpp"
#include "version.hpp"

#include <cstdio>
#include <deque>
#include <fstream>
#include <iostream>
#include <limits>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <utility>
#include <vector>



// ----------------------------------------------------------------------
// Types
class Object;
class CCNode;

typedef std::map< string, std::vector< Summary * > > GroupSum_t;
typedef std::map< string, Summary * > TypeTotalSum_t;
typedef std::map< unsigned int, Summary * > SizeSum_t;

// An Edge Source is an object and its specific field.
// Global edge sources have a NULL object pointer.
typedef std::pair< Object *, FieldId_t > EdgeSrc_t; 
// ----------------------------------------------------------------------
// We then map Edge Sources to a vector/list of Object pointers.
// For every edge source, we keep a list of all the objects (pointers)
// it has ever pointed to.
// This is the in-memory stl version
typedef std::map< EdgeSrc_t, std::vector< Object * > > EdgeSummary_t;
// We need a reverse map from object pointer to the edge sources that ever
// pointed to that object.
typedef std::map< Object *, std::vector< EdgeSrc_t > > Object2EdgeSrcMap_t;


// EdgeSummary_t and Object2EdgeSrcMap_t contain the raw data. We need some
// summaries.
// enum class ObjectRefType moved to heap.h - RLV

enum class EdgeSrcType {
    STABLE = 1, // Like True Love. Only one target ever.
    // TODO: This is an original idea that maybe singly-owned objects
    // can be considered part of the stable group.
    SERIAL_STABLE = 2, // Kind of like a serial monogamist.
                       // Can point to different objects but never shares the
                       // target object
    UNSTABLE = 32, // Points to different objects and
                   // shares objects with other references
    UNKNOWN = 1024
};

string edgesrctype2str( EdgeSrcType r)
{
    if (r == EdgeSrcType::STABLE) {
        return "STABLE";
    } else if (r == EdgeSrcType::SERIAL_STABLE) {
        return "SERIAL_STABLE";
    } else if (r == EdgeSrcType::UNSTABLE) {
        return "UNSTABLE";
    } else {
        return "UNKNOWN";
    }
}

string edgesrctype2shortstr( EdgeSrcType r)
{
    if (r == EdgeSrcType::STABLE) {
        return "S"; // STABLE
    } else if (r == EdgeSrcType::SERIAL_STABLE) {
        return "ST"; // SERIAL_STABLE
    } else if (r == EdgeSrcType::UNSTABLE) {
        return "U"; // UNSTABLE
    } else {
        return "X"; // UNKNOWN
    }
}

// Map a reference to its type
typedef std::map< EdgeSrc_t, EdgeSrcType > EdgeSrc2Type_t;
// Map an object to its type
// TODO: Is this needed?
typedef std::map< Object *, ObjectRefType > Object2Type_t;

// ----------------------------------------------------------------------
//   Globals

// -- Key object map to set of objects
KeySet_t keyset;
// -- Object to key object map
ObjectPtrMap_t whereis;

// -- The heap
HeapState Heap( whereis, keyset );

// -- Execution state
#ifdef ENABLE_TYPE1
ExecMode cckind = ExecMode::Full; // Full calling context
#else
ExecMode cckind = ExecMode::StackOnly; // Stack-only context
#endif // ENABLE_TYPE1

ExecState Exec(cckind);

// -- Turn on debugging
bool debug = false;

// -- Remember the last event by thread ID
// TODO: Types for object and thread IDs?
// TODO LastMap last_map;

// ----------------------------------------------------------------------
//   Analysis
deque< deque<Object*> > cycle_list;
set<unsigned int> root_set;


EdgeSummary_t edge_summary;
Object2EdgeSrcMap_t obj2ref_map;


bool verify_trace( std::deque< Record * > &trace )
{
    VTime_t rec_timestamp = 0;
    VTime_t prev_timestamp = 0;
    unsigned int recnum = 0;
    for ( auto iter = trace.begin();
          iter != trace.end();
          iter++ ) {
        prev_timestamp = rec_timestamp;
        rec_timestamp = (*iter)->get_ET_timestamp();
        cerr << "prev: " << prev_timestamp << "    rec: " << rec_timestamp << endl;
        if (rec_timestamp < prev_timestamp) {
            cerr << "ERROR at record number: " << recnum << endl;
            // TODO
            assert(false);
        }
        recnum++;
    }
}

// Uses global:
//     * Heap
unsigned int insert_death_records_into_trace( std::deque< Record * > &trace )
{
    auto reciter = trace.begin(); 
    VTime_t rec_timestamp = 0;
    VTime_t prev_timestamp = 0;
    for ( auto iter = Heap.begin();
          iter != Heap.end();
          iter++ ) {
        ObjectId_t object_id = iter->first;
        Object *obj = iter->second;
        VTime_t ettime = obj->getLastTimestamp();
        while (reciter != trace.end()) {
            prev_timestamp = rec_timestamp;
            rec_timestamp = (*reciter)->get_ET_timestamp();
            cerr << "prev: " << prev_timestamp << "    rec: " << rec_timestamp << endl;
            if (rec_timestamp > ettime) {
                break;
            }
            reciter++;
        }
        DeathRecord *drec = new DeathRecord( object_id,
                                             0, // TODO: type_id
                                             prev_timestamp );

        trace.insert(reciter, drec);
    }
}

bool verify_garbage_order( std::deque< Object * > &garbage )
{
    VTime_t rec_timestamp = std::numeric_limits<unsigned int>::min();
    VTime_t prev_timestamp = rec_timestamp;
    unsigned int recnum = 0;
    for ( auto iter = garbage.begin();
          iter != garbage.end();
          iter++ ) {
        prev_timestamp = rec_timestamp;
        rec_timestamp = (*iter)->getLastTimestamp();
        cerr << "prev: " << prev_timestamp << "    rec: " << rec_timestamp << endl;
        if (rec_timestamp < prev_timestamp) {
            cerr << "ERROR at record number: " << recnum << endl;
            // TODO
            // assert(false);
        }
        recnum++;
    }
    return true;
}

// ----------------------------------------------------------------------
//   Uses global:
//       * Heap
void apply_merlin( std::deque< Record * > &trace )
{
    cerr << "apply_merlin: " << endl;
    // TODO: Use setDeathTime( new_dtime );
    std::deque< Object * > garbage;
    for ( auto iter = Heap.begin();
          iter != Heap.end();
          iter++ ) {
        garbage.push_back(iter->second);
    }
    cerr << garbage.size() << " objects in garbage." << endl;
    // Sort the garbage vector according to latest last_timestamp.
    std::sort( garbage.begin(),
               garbage.end(),
               []( Object *left, Object *right ) {
                   return (left->getLastTimestamp() > right->getLastTimestamp());
               } );
    assert(verify_garbage_order(garbage));
    // Do a standard DFS.
    while (garbage.size() > 0) {
        // Start with the latest for the DFS.
        Object *cur = garbage[0];
        garbage.pop_front();
        std::deque< Object * > mystack; // stack for DFS
        std::set< Object * > labeled; // What Objects have been labeled
        mystack.push_front( cur );
        while (mystack.size() > 0) {
            cerr << ".";
            Object *objtmp = mystack[0];
            mystack.pop_front();
            assert(objtmp);
            // The current timestamp max
            unsigned int tstamp_max = objtmp->getLastTimestamp();
            objtmp->setDeathTime( tstamp_max );
            // Remove from garbage deque
            auto ngiter = std::find( garbage.begin(),
                                     garbage.end(),
                                     objtmp );
            if (ngiter != garbage.end()) {
                // still in the garbage deque
                garbage.erase(ngiter);
            }
            // Check if labeled already
            auto iter = labeled.find(objtmp);
            if (iter == labeled.end()) {
                // Not found => not labeled

                labeled.insert(objtmp);
                // Go through all the edges out of objtmp
                // -- Visit all edges
                for ( auto ptr = objtmp->getFields().begin();
                      ptr != objtmp->getFields().end();
                      ptr++ ) {
                    cerr << "-";
                    Edge *edge = ptr->second;
                    if (edge) {
                        edge->setEndTime( tstamp_max );
                        Object *mytgt = edge->getTarget();
                        if (mytgt) {
                            // Get timestamp
                            unsigned int tstmp = mytgt->getLastTimestamp();
                            if (tstamp_max > tstmp) {
                                mytgt->setLastTimestamp( tstamp_max );
                                mytgt->setDeathTime( tstamp_max );
                            }
                            // This might be a bug. I probably shouldn't be setting the tstamp_max here.
                            // But then again where should it be set? It seems that this is the right thing
                            // to do here.
                            // TODO else {
                            // TODO     tstamp_max = tstmp;
                            // TODO }
                            mystack.push_front( mytgt );
                        }
                    }
                }
            }
        }
    }
    cerr << endl;
    // Until the vector is empty
    insert_death_records_into_trace(trace);
}


// ----------------------------------------------------------------------
//   TODO: todo
void get_size( std::map< TypeId_t, unsigned int > &size_map )
{
}

// ----------------------------------------------------------------------
//   Read and process trace events. This implements the Merlin algorithm.
unsigned int read_trace_file_part1( FILE *f, // source trace file
                                    std::deque< Record * > &trace )
{
    Tokenizer tokenizer(f);

    unsigned int total_objects = 0;
    // Map of objects without N/A allocation events:
    std::map< ObjectId_t, Object * > no_alloc_map;
    // Size map
    std::map< TypeId_t, unsigned int > class2size_map;
    // Edge map
    std::set< Edge * > edge_set;
    // Merlin aglorithm: 
    std::deque< Object * > new_garbage;
    // TODO: There's no main method here?
    //       Method *main_method = ClassInfo::get_main_method();
    //       unsigned int main_id = main_method->getId();

    // TODO: No death times yet
    //      VTime_t latest_death_time = 0;
    VTime_t allocation_time = 0;
        
    while (!tokenizer.isDone()) {
        MethodId_t method_id;
        ObjectId_t object_id;
        ObjectId_t target_id;
        FieldId_t field_id;
        ThreadId_t thread_id;
        Object *obj = NULL;
        Object *target = NULL;
        Method *method = NULL;
        Thread *thread = NULL;
        string tmp_todo_str("TODO"); // To mark whatever TODO
        tokenizer.getLine();
        if (tokenizer.isDone()) {
            break;
        }
        char rec_type = tokenizer.getChar(0);
        switch (rec_type) {

            case 'M':
                {
                    // M <methodid> <objectId> <threadid>
                    // 0      1         2           3
                    assert(tokenizer.numTokens() == 4);
                    method_id = tokenizer.getInt(1);
                    object_id = tokenizer.getInt(2);
                    // TODO: method = ClassInfo::TheMethods[method_id];
                    thread_id = tokenizer.getInt(3);
                    Exec.IncUpdateTime();
                    VTime_t current_time = Exec.NowUp();
                    auto recptr = new MethodRecord( method_id,
                                                    object_id,
                                                    thread_id,
                                                    current_time );
                    trace.push_back(recptr);
                    thread = Exec.getThread(thread_id);
                    if (object_id > 0) {
                        obj = Heap.getObject(object_id);
                        if (obj == NULL) {
                            // Object isn't in the heap yet!
                            // NOTE: new_flag is false.
                            obj = Heap.allocate( object_id,
                                                 4, // TODO: we don't have the size!
                                                 rec_type, // kind of alloc - TODO: M isn't an allocation type really.
                                                 tmp_todo_str, // get from type_id
                                                 NULL, // AllocSite pointer - TODO: Would this NULL cause problems somewhere else?
                                                 tmp_todo_str, // TODO: njlib_sitename, // NonJava-library alloc sitename
                                                 1, // length - TODO: No length either
                                                 thread, // thread Id
                                                 false, // new_flag
                                                 Exec.NowUp() ); // Current time
                            ++total_objects;
                        }
                        assert(obj);
                        obj->setLastTimestamp( current_time ); // TODO: Maybe not needed?
                    }
                }
                break;

            case 'E':
                {
                    // ET1 looked like this:
                    //     E <methodid> <receiver> [<exceptionobj>] <threadid>
                    // ET2 is now:
                    //     E <method-id> <thread-id>
                    assert(tokenizer.numTokens() == 3);
                    method_id = tokenizer.getInt(1);
                    // TODO: method = ClassInfo::TheMethods[method_id];
                    thread_id = tokenizer.getInt(2);
                    VTime_t current_time = Exec.NowUp();
                    auto recptr = new ExitRecord( method_id, thread_id, current_time );
                    trace.push_back(recptr);
                    if (object_id > 0) {
                        obj = Heap.getObject(object_id);
                        assert(obj != NULL);
                    }
                }
                break;

            case 'A':
            case 'N':
                {
                    // A/N <id> <size> <type> <site> <length?> <threadid>
                    //   0   1    2      3      4      5           6
                    assert(tokenizer.numTokens() == 7);
                    object_id = tokenizer.getInt(1);
                    unsigned int size = tokenizer.getInt(2);
                    TypeId_t type_id = tokenizer.getInt(3);
                    SiteId_t site_id = tokenizer.getInt(4);
                    unsigned int length = tokenizer.getInt(5);
                    thread_id = tokenizer.getInt(6);
                    VTime_t current_time = Exec.NowUp();
                    auto recptr = new AllocRecord( object_id,
                                                   site_id,
                                                   thread_id,
                                                   type_id,
                                                   length,
                                                   size,
                                                   current_time );
                    trace.push_back(recptr);
                    // Add to heap
                    // TODO: AllocSites
                    thread = Exec.getThread(thread_id);
                    allocation_time = Heap.getAllocTime();
                    Exec.SetAllocTime(allocation_time);
                    AllocSite *as = ClassInfo::TheAllocSites[tokenizer.getInt(4)];
#if 0 // TODO ALLOCSITE
                        string njlib_sitename;
                        if (thread) {
                            MethodDeque javalib_context = thread->top_javalib_methods();
                            assert(javalib_context.size() > 0);
                            Method *meth = javalib_context.back();
                            njlib_sitename = ( meth ? meth->getName() : "NONAME" );
                        } else {
                            assert(false);
                        } // if (thread) ... else
#endif // TODO ALLOCSITE
                    if (object_id > 0) {
                        obj = Heap.allocate( object_id,
                                             size,
                                             rec_type, // kind of alloc
                                             tmp_todo_str, // get from type_id
                                             as, // AllocSite pointer
                                             tmp_todo_str, // TODO: njlib_sitename, // NonJava-library alloc sitename
                                             length, // length
                                             thread, // thread Id
                                             true, // new_flag
                                             Exec.NowUp() ); // Current time
                        ++total_objects;
                        Exec.IncUpdateTime();
                        obj->setLastTimestamp( current_time );
                    }
                }
                break;

            case 'U':
                {
                    // ET1 looked like this:
                    //       U <old-target> <object> <new-target> <field> <thread>
                    // ET2 is now:
                    //       U targetObjectHash fieldID srcObjectHash threadId?
                    //       0         1           2          3           4
                    // -- Look up objects and perform update
                    assert(tokenizer.numTokens() == 5);
                    target_id = tokenizer.getInt(1);
                    field_id = tokenizer.getInt(2);
                    object_id = tokenizer.getInt(3);
                    ThreadId_t thread_id = tokenizer.getInt(4);
                    Exec.IncUpdateTime();
                    VTime_t current_time = Exec.NowUp();
                    auto recptr = new UpdateRecord( target_id,
                                                    field_id,
                                                    object_id,
                                                    thread_id,
                                                    current_time );
                    trace.push_back(recptr);
                    // ---------------------------------------------------------------------------
                    // Set the Merlin timestamps:
                    obj = Heap.getObject(object_id);
                    if (obj == NULL) {
                        auto iter = no_alloc_map.find(object_id);
                        if (iter == no_alloc_map.end()) {
                            cerr << "-- No alloc event: " << object_id << endl;
                            thread = Exec.getThread(thread_id);
                            assert(thread);
                            obj = Heap.allocate( object_id,
                                                 4, // TODO: we don't have the size!
                                                 rec_type, // kind of alloc - TODO: M isn't an allocation type really.
                                                 tmp_todo_str, // get from type_id
                                                 NULL, // AllocSite pointer - TODO: Would this NULL cause problems somewhere else?
                                                 tmp_todo_str, // TODO: njlib_sitename, // NonJava-library alloc sitename
                                                 1, // length - TODO: No length either
                                                 thread, // thread Id
                                                 false, // new_flag
                                                 Exec.NowUp() ); // Current time
                            ++total_objects;
                            no_alloc_map[object_id] = obj;
                        } else {
                            cerr << "-- No alloc event DUPE: " << object_id << endl;
                            assert(false);
                        }
                    }
                    assert(obj != NULL);
                    target = NULL;
                    if (target_id > 0) {
                        target = Heap.getObject(target_id);
                        if (target == NULL) {
                            auto iter = no_alloc_map.find(target_id);
                            if (iter == no_alloc_map.end()) {
                                cerr << "-- No alloc event: " << target_id << endl;
                                thread = Exec.getThread(thread_id);
                                target = Heap.allocate( target_id,
                                                        4, // TODO: we don't have the size!
                                                        rec_type, // kind of alloc - TODO: M isn't an allocation type really.
                                                        tmp_todo_str, // get from type_id
                                                        NULL, // AllocSite pointer - TODO: Would this NULL cause problems somewhere else?
                                                        tmp_todo_str, // TODO: njlib_sitename, // NonJava-library alloc sitename
                                                        1, // length - TODO: No length either
                                                        thread, // thread Id
                                                        false, // new_flag
                                                        Exec.NowUp() ); // Current time
                                ++total_objects;
                                no_alloc_map[target_id] = target;
                            } else {
                                cerr << "-- No alloc event DUPE: " << object_id << endl;
                                assert(false);
                            }
                        }
                        assert(target != NULL);
                        target->setLastTimestamp( current_time );
                    }
                    obj->setLastTimestamp( current_time );

                    Edge *old_target_edge;
                    Object *old_target;
                    ObjectId_t old_target_id = 0;
                    Edge *new_edge;

                    old_target_edge = obj->getEdge(field_id);
                    if (old_target_edge) {
                        old_target = old_target_edge->getTarget();
                        delete old_target_edge;
                    }
                    if (old_target) {
                        old_target_id = old_target->getId();
                        if (old_target_id != target_id) {
                            if (target && target_id > 0) {
                                Edge *new_edge = Heap.make_edge( obj,
                                                                 field_id,
                                                                 target,
                                                                 Exec.NowUp() );
                                obj->updateField( new_edge,
                                                  field_id );
                             }
                         }
                    }
                }
                break;

            case 'W':
                {
                    // W aliveObjectHash classID timestamp
                    //        1              2       3
                    // TODO: cerr << "DBG: W--" << endl;
                    if (tokenizer.numTokens() != 4) {
                        unsigned int num = tokenizer.numTokens();
                        cerr << "ERROR -- W[ " << num << " ] :";
                        for (int i = 0; i < num; ++i) {
                            cerr << tokenizer.getInt(i);
                        }
                        cerr << endl;
                    }
                    object_id = tokenizer.getInt(1);
                    TypeId_t type_id = tokenizer.getInt(2);
                    VTime_t timestamp = tokenizer.getInt(3);
                    VTime_t current_time = Exec.NowUp();
                    auto recptr = new WitnessRecord( object_id,
                                                     type_id,
                                                     timestamp,
                                                     current_time );
                    trace.push_back(recptr);
                    // Set the Merlin timestamps:
                    obj = Heap.getObject(object_id);
                    assert(obj != NULL);
                    obj->setLastTimestamp( current_time );
                }
                break;

            default:
                cerr << "ERROR: Unknown entry " << tokenizer.getChar(0) << endl;
                break;
        } // switch (tokenizer.getChar(0))
    } // while (!tokenizer.isDone())

    return total_objects;
}


// ---------------------------------------------------------------------------
// ------[ OUTPUT FUNCTIONS ]-------------------------------------------------
// ---------------------------------------------------------------------------

void output_all_objects2( string &objectinfo_filename,
                          HeapState &myheap,
                          std::set<ObjectId_t> dag_keys,
                          std::set<ObjectId_t> dag_all_set,
                          std::set<ObjectId_t> all_keys,
                          unsigned int final_time )
{
    ofstream object_info_file(objectinfo_filename);
    object_info_file << "---------------[ OBJECT INFO ]--------------------------------------------------" << endl;
    const vector<string> header( { "objId", "createTime", "deathTime", "size", "type",
                                   "diedBy", "lastUpdate", "subCause", "clumpKind",
                                   "deathContext1", "deathContext2", "firstNonJavaLibMethod",
                                   "deatchContext_height", "allocContext2", "allocContextType",
                                   "createTime_alloc", "deathTime_alloc",
                                   "allocSiteName", "stability", "last_actual_timestamp",
                                   "nonJavaLibAllocSiteName" } );

    for ( ObjectMap::iterator it = myheap.begin();
          it != myheap.end();
          ++it ) {
        Object *object = it->second;
        ObjectId_t objId = object->getId();
        KeyType ktype = object->getKeyType();
        string dgroup_kind;
        if (ktype == KeyType::CYCLE) {
            dgroup_kind = "CYC";
        } else if (ktype == KeyType::CYCLEKEY) {
            dgroup_kind = "CYCKEY";
        } else if (ktype == KeyType::DAG) {
            dgroup_kind = "DAG";
        } else if (ktype == KeyType::DAGKEY) {
            dgroup_kind = "DAGKEY";
        } else {
            dgroup_kind = "CYC";
        }
        string dtype;
        if (object->getDiedByStackFlag()) {
            dtype = "S"; // by stack
        } else if (object->getDiedByHeapFlag()) {
            if (object->wasLastUpdateFromStatic()) {
                dtype = "G"; // by static global
            } else {
                dtype = "H"; // by heap
            }
        } else {
            dtype = "E"; // program end
        }
        // TODO: Commented out the CONTEXT PAIR functionality that I'm not using.
        // TODO // Get the context pair and type for the allocation event
        // TODO ContextPair allocCpair = object->getAllocContextPair();
        // TODO Method *alloc_meth_ptr1 = std::get<0>(allocCpair);
        // TODO Method *alloc_meth_ptr2 = std::get<1>(allocCpair);
        // TODO string alloc_method1 = (alloc_meth_ptr1 ? alloc_meth_ptr1->getName() : "NONAME");
        // TODO string alloc_method2 = (alloc_meth_ptr2 ? alloc_meth_ptr2->getName() : "NONAME");
        // TODO // Get the context pair and type for the death event
        // TODO ContextPair deathCpair = object->getDeathContextPair();
        // TODO Method *death_meth_ptr1 = std::get<0>(deathCpair);
        // TODO Method *death_meth_ptr2 = std::get<1>(deathCpair);
        // TODO string death_method1 = (death_meth_ptr1 ? death_meth_ptr1->getName() : "NONAME");
        // TODO string death_method2 = (death_meth_ptr2 ? death_meth_ptr2->getName() : "NONAME");
        // END TODO: CONTEXT PAIR funcionality
        
        string death_method_l1 = object->getDeathContextSiteName(1);
        string death_method_l2 = object->getDeathContextSiteName(2);
        string death_method_nonjavalib = object->get_nonJavaLib_death_context();
        string last_action_method_nonjavalib = object->get_nonJavaLib_last_action_context();
        string allocsite_name = object->getAllocSiteName();
        string nonjlib_allocsite_name = object->getNonJavaLibAllocSiteName();
        ObjectRefType objstability = object->getRefTargetType();
        // S -> Stable
        // U -> Unstable
        // X -> Unknown
        string stability = ( (objstability == ObjectRefType::SINGLY_OWNED) ? "S"
                                   : (objstability == ObjectRefType::MULTI_OWNED ? "M" : "X") );
        unsigned int dtime = object->getDeathTime();
        object_info_file << objId
            << "," << object->getCreateTime()
            << "," << (dtime > 0 ? dtime : final_time)
            << "," << object->getSize()
            << "," << object->getType()
            << "," << dtype
            << "," << (object->wasLastUpdateNull() ? "NULL" : "VAL")
            << "," << (object->getDiedByStackFlag() ? (object->wasPointedAtByHeap() ? "SHEAP" : "SONLY")
                                                    : "H")
            << "," << dgroup_kind
            //--------------------------------------------------------------------------------
            << "," << death_method_l1 // Fisrt level context for death
            //--------------------------------------------------------------------------------
            << "," << death_method_l2 // Second level context for death
            //--------------------------------------------------------------------------------
            << "," << last_action_method_nonjavalib // first nonJavaLib method
            //--------------------------------------------------------------------------------
            << "," << "TODO" //  Full death context height
            // TODO << "," << alloc_method1 // Part 1 of simple context pair - alloc site
            //--------------------------------------------------------------------------------
            << "," << "X" //  padding - used to be allocContextType
            // TODO << "," << (object->getAllocContextType() == CPairType::CP_Call ? "C" : "R") // C is call. R is return.
            //--------------------------------------------------------------------------------
            << "," << object->getCreateTimeAlloc()
            << "," << object->getDeathTimeAlloc()
            << "," << allocsite_name
            << "," << stability  // S, U, or X
            << "," << object->getLastTimestamp()
            << "," << nonjlib_allocsite_name
            << endl;
            // TODO: The following can be made into a lookup table:
            //       method names
            //       allocsite names
            //       type names
            // May only be necessary for performance reasons (ie, simulator eats up too much RAM 
            // on the larger benchmarks/programs.)
    }
    object_info_file << "---------------[ OBJECT INFO END ]----------------------------------------------" << endl;
    object_info_file.close();
}



void print_usage(string exec_name)
{
    cout << "Usage choices: " << endl;
    cout << "       " << exec_name << " SIM <classesfile> <fieldsfile> <methodsfile> <output base name> <IGNORED> <OBJDEBUG/NOOBJDEBUG> <main.class> <main.function>" << endl;
    cout << "       " << exec_name << " CLASS <classesfile>" << endl;
    cout << "       " << exec_name << " FIELDS <fieldsfile>" << endl;
    cout << "       " << exec_name << " METHODS <methodsfile>" << endl;
    cout << "      git version: " << build_git_sha << endl;
    cout << "      build date : " << build_git_time << endl;
    cout << "      CC kind    : " << Exec.get_kind() << endl;
    exit(1);
}

// Forward declarations
void sim_main(int argc, char* argv[]);
void class_main(int argc, char* argv[]);
void fields_main(int argc, char* argv[]);
void methods_main(int argc, char* argv[]);

// ----------------------------------------------------------------------

int main(int argc, char* argv[])
{
    bool print_help = false;
    if ((argc == 10) && (string("SIM") == argv[1])) {
        sim_main(argc, argv);
    } else if (argc == 3) {
        if (string("CLASS") == argv[1]) {
            class_main(argc, argv);
        } else if (string("FIELDS") == argv[1]) {
            fields_main(argc, argv);
        } else if (string("METHODS") == argv[1]) {
            methods_main(argc, argv);
        } else {
            print_help = true;
        }
        // Fall through to print_usage.
    } else {
        print_help = true;
    }
    if (print_help) {
        print_usage(string(argv[0]));
    }
    return 0;
}


void sim_main(int argc, char* argv[])
{
    cout << "---------------[ START ]-----------------------------------------------------------" << endl;
    //--------------------------------------------------------------------------------
    // Setup filenames for output files:
    string basename(argv[5]);
    //--------------------------------------------------------------------------------

    // TODO: This sets the 'main' class. But what was exactly the main  class?
    // TODO: 2018-11-10
    string main_class(argv[8]);
    string main_function(argv[9]);
    cout << "Main class: " << main_class << endl;
    cout << "Main function: " << main_function << endl;

    // Set up 'CYCLE' option.
    // TODO: Document what exactly the CYCLE option is.
    string cycle_switch(argv[6]);
    bool cycle_flag = ((cycle_switch == "NOCYCLE") ? false : true);
    
    // Setup 'DEBUG' option.
    string obj_debug_switch(argv[7]);
    bool obj_debug_flag = ((obj_debug_switch == "OBJDEBUG") ? true : false);
    if (obj_debug_flag) {
        cout << "Enable OBJECT DEBUG." << endl;
        Heap.enableObjectDebug(); // default is no debug
    }

    // TODO: Names file is likely different
    // TODO: 2018-11-10
    cout << "Read names file..." << endl;
    ClassInfo::read_names_file_et2( argv[2],
                                    argv[3], // TODO: fields_filename
                                    argv[4] ); // TODO: methods_filename 

    cout << "Start trace..." << endl;
    FILE *f = fdopen(STDIN_FILENO, "r");
    std::deque< Record * > trace; // IN memory trace deque
    unsigned int total_objects = read_trace_file_part1(f, trace);
    verify_trace(trace);
    // Do the Merlin algorithm.
    apply_merlin( trace );
    // TODO:
    cout << "---------------[ DONE ]------------------------------------------------------------" << endl;
    cout << "#     git version: " <<  build_git_sha << endl;
    cout << "#     build date : " <<  build_git_time << endl;
    exit(0);
}

// Just read in the class file and output it.
void class_main(int argc, char* argv[])
{
    cout << "#     git version: " <<  build_git_sha << endl;
    cout << "#     build date : " <<  build_git_time << endl;
    cout << "---------------[ START ]-----------------------------------------------------------" << endl;
    //--------------------------------------------------------------------------------
    
    ClassInfo::impl_read_classes_file_et2(argv[2]);
    for (auto iter : ClassInfo::rev_map) {
        cout << "CLASS," << iter.first << "," << iter.second << endl;
    }
}

void fields_main(int argc, char* argv[])
{
    cout << "#     git version: " <<  build_git_sha << endl;
    cout << "#     build date : " <<  build_git_time << endl;
    cout << "---------------[ START ]-----------------------------------------------------------" << endl;
    //--------------------------------------------------------------------------------
    cout << "Read names file..." << endl;
    string fields_filename(argv[2]);
    std::map<FieldId_t, string> fields_map = ClassInfo::impl_read_fields_file_et2(argv[2]);
    auto iter = fields_map.begin();
    while (iter != fields_map.end()) {
        iter++;
        cerr << ".";
    }
    cerr << "DONE" << endl;
}

void methods_main(int argc, char* argv[])
{
}
