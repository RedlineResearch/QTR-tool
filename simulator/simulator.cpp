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

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <map>
#include <set>
#include <vector>
#include <deque>
#include <string>
#include <utility>



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


// ----------------------------------------------------------------------
void sanity_check()
{
/*
   if (Now > obj->getDeathTime() && obj->getRefCount() != 0) {
   nonzero_ref++;
   printf(" Non-zero-ref-dead %X of type %s\n", obj->getId(), obj->getType().c_str());
   }
   */
}

bool member( Object* obj, const ObjectSet& theset )
{
    return theset.find(obj) != theset.end();
}

unsigned int closure( ObjectSet& roots,
                      ObjectSet& premarked,
                      ObjectSet& result )
{
    unsigned int mark_work = 0;

    vector<Object*> worklist;

    // -- Initialize the worklist with only unmarked roots
    for ( ObjectSet::iterator i = roots.begin();
          i != roots.end();
          i++ ) {
        Object* root = *i;
        if ( !member(root, premarked) ) {
            worklist.push_back(root);
        }
    }

    // -- Do DFS until the worklist is empty
    while (worklist.size() > 0) {
        Object* obj = worklist.back();
        worklist.pop_back();
        result.insert(obj);
        mark_work++;

        const EdgeMap& fields = obj->getFields();
        for ( EdgeMap::const_iterator p = fields.begin();
              p != fields.end();
              p++ ) {
            Edge* edge = p->second;
            Object* target = edge->getTarget();
            if (target) {
                // if (target->isLive(Exec.NowUp())) {
                if ( !member(target, premarked) &&
                     !member(target, result) ) {
                    worklist.push_back(target);
                }
                // } else {
                // cout << "WEIRD: Found a dead object " << target->info() << " from " << obj->info() << endl;
                // }
            }
        }
    }

    return mark_work;
}

unsigned int count_live( ObjectSet & objects, unsigned int at_time )
{
    int count = 0;
    // -- How many are actually live
    for ( ObjectSet::iterator p = objects.begin();
          p != objects.end();
          p++ ) {
        Object* obj = *p;
        if (obj->isLive(at_time)) {
            count++;
        }
    }

    return count;
}

void update_reference_summaries( Object *src,
                                 FieldId_t fieldId,
                                 Object *tgt )
{
    // edge_summary : EdgeSummary_t is a global
    // obj2ref_map : Object2EdgeSrcMap_t is a global
    EdgeSrc_t ref = std::make_pair( src, fieldId );
    // The reference 'ref' points to the new target
    auto iter = edge_summary.find(ref);
    if (iter == edge_summary.end()) {
        // Not found. Create a new vector of Object pointers
        edge_summary[ref] = std::vector< Object * >();
    }
    edge_summary[ref].push_back(tgt);
    // Do the reverse mapping ONLY if tgt is not NULL
    if (tgt) {
        auto rev = obj2ref_map.find(tgt);
        if (rev == obj2ref_map.end()) {
            obj2ref_map[tgt] = std::move(std::vector< EdgeSrc_t >());
        }
        obj2ref_map[tgt].push_back(ref);
    }
}

// ----------------------------------------------------------------------
//   Read and process trace events
void apply_merlin( std::deque< Object * > &new_garbage )
{
    if (new_garbage.size() == 0) { 
        // TODO Log an ERROR or a WARNING
        return;
    }
    // TODO: Use setDeathTime( new_dtime );
    // Sort the new_garbage vector according to latest last_timestamp.
    std::sort( new_garbage.begin(),
               new_garbage.end(),
               []( Object *left, Object *right ) {
                   return (left->getLastTimestamp() > right->getLastTimestamp());
               } );
    // Do a standard DFS.
    while (new_garbage.size() > 0) {
        // Start with the latest for the DFS.
        Object *cur = new_garbage[0];
        new_garbage.pop_front();
        std::deque< Object * > mystack; // stack for DFS
        std::set< Object * > labeled; // What Objects have been labeled
        mystack.push_front( cur );
        while (mystack.size() > 0) {
            Object *otmp = mystack[0];
            mystack.pop_front();
            assert(otmp);
            // The current timestamp max
            unsigned int tstamp_max = otmp->getLastTimestamp();
            otmp->setDeathTime( tstamp_max );
            // Remove from new_garbage deque
            auto ngiter = std::find( new_garbage.begin(),
                                     new_garbage.end(),
                                     otmp );
            if (ngiter != new_garbage.end()) {
                // still in the new_garbage deque
                new_garbage.erase(ngiter);
            }
            // Check if labeled already
            auto iter = labeled.find(otmp);
            if (iter == labeled.end()) {
                // Not found => not labeled

                labeled.insert(otmp);
                // Go through all the edges out of otmp
                // -- Visit all edges
                for ( auto ptr = otmp->getFields().begin();
                      ptr != otmp->getFields().end();
                      ptr++ ) {
                    Edge *edge = ptr->second;
                    // TODO: What to do here now there's no edgestate? TODO
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
    // Until the vector is empty
}

// ----------------------------------------------------------------------
//   Read and process trace events. This implements the Merlin algorithm.
unsigned int read_trace_file_part1( FILE *f ) // source trace file
{
    Tokenizer tokenizer(f);

    MethodId_t method_id;
    ObjectId_t object_id;
    ObjectId_t target_id;
    FieldId_t field_id;
    ThreadId_t thread_id;
    Object *obj;
    Object *target;
    Method *method;
    unsigned int total_objeobcts = 0;
    // Save the trace in memory:
    std::deque<Record *> trace;
    // Merlin aglorithm: 
    std::deque< Object * > new_garbage;
    // TODO: There's no main method here?
    //       Method *main_method = ClassInfo::get_main_method();
    //       unsigned int main_id = main_method->getId();

    // TODO: No death times yet
    //      VTime_t latest_death_time = 0;
    VTime_t allocation_time = 0;
    unsigned int total_objects = 0;
        
    while (!tokenizer.isDone()) {
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
                    auto recptr = new MethodRecord( method_id,
                                                    object_id,
                                                    thread_id );
                    trace.push_back(recptr);
                    Exec.IncUpdateTime();
                    // If object Id isn't available via an allocation event,
                    // then it's a stack object.
                    VTime_t current_time = Exec.NowUp();
                    Thread *thread = Exec.getThread(thread_id);
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
                    obj->setLastTimestamp( current_time ); // TODO: Maybe not needed?
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
                    auto recptr = new ExitRecord( method_id, thread_id );
                    trace.push_back(recptr);
                    obj = Heap.getObject(object_id);
                    assert(obj != NULL);
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
                    auto recptr = new AllocRecord( object_id,
                                                   site_id,
                                                   thread_id,
                                                   type_id,
                                                   length,
                                                   size );
                    trace.push_back(recptr);
                    // Add to heap
                    // TODO: AllocSites
                    Thread *thread = Exec.getThread(thread_id);
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
                    VTime_t current_time = Exec.NowUp();
                    obj->setLastTimestamp( current_time );
                }
                break;

            case 'U':
                {
                    // ET1 looked like this:
                    //       U <old-target> <object> <new-target> <field> <thread>
                    // ET2 is now:
                    //       U targetObjectHash fieldID srcObjectHash timestamp
                    //       0         1           2          3           4
                    // -- Look up objects and perform update
                    assert(tokenizer.numTokens() == 5);
                    target_id = tokenizer.getInt(1);
                    field_id = tokenizer.getInt(2);
                    object_id = tokenizer.getInt(3);
                    VTime_t timestamp = tokenizer.getInt(4);
                    auto recptr = new UpdateRecord( target_id,
                                                    field_id,
                                                    object_id,
                                                    timestamp );
                    trace.push_back(recptr);
                    Exec.IncUpdateTime();
                    // Set the Merlin timestamps:
                    VTime_t current_time = Exec.NowUp();
                    obj = Heap.getObject(object_id);
                    assert(obj != NULL);
                    target = ((target_id > 0) ? Heap.getObject(target_id) : NULL);
                    obj->setLastTimestamp( current_time );
                    if (target) {
                        target->setLastTimestamp( current_time );
                    }
                    Edge *old_target_edge = obj->getEdge(field_id);
                    Object *old_target = old_target_edge->getTarget();
                    ObjectId_t old_target_id = old_target->getId();
                    // TODO [ START ]
                    if (old_target_id == target_id) {
                        // It sometimes happens that the newtarget is the same as
                        // the old target. So we won't create any more new edges.
                        // DEBUG: cout << "UPDATE same new == old: " << target << endl;
                    } else {
                        Edge *new_edge = NULL;
                        // Can't call updateField if target is NULL
                        if (target) {
                            new_edge = Heap.make_edge( obj,
                                                       field_id,
                                                       target,
                                                       Exec.NowUp() );
                        }
                        obj->updateField( new_edge,
                                          field_id );
                    }
                    // TODO [ END ]
                }
                break;

            case 'W':
                {
                    // W aliveObjectHash classID timestamp
                    //        1              2       3
                    assert(tokenizer.numTokens() == 4);
                    object_id = tokenizer.getInt(1);
                    TypeId_t type_id = tokenizer.getInt(2);
                    VTime_t timestamp = tokenizer.getInt(3);
                    auto recptr = new WitnessRecord( object_id,
                                                     type_id,
                                                     timestamp );
                    trace.push_back(recptr);
                    // Set the Merlin timestamps:
                    VTime_t current_time = Exec.NowUp();
                    obj = Heap.getObject(object_id);
                    if (obj) {
                        obj->setLastTimestamp( current_time );
                    } else {
                        cerr << "NULL object: " << object_id << endl;
                    }
                }
                break;

            default:
                cerr << "ERROR: Unknown entry " << tokenizer.getChar(0) << endl;
                break;
        } // switch (tokenizer.getChar(0))
    } // while (!tokenizer.isDone())

    // TODO: What should return value be?
    return total_objects;
}


unsigned int sumSize( std::set< Object * >& s )
{
    unsigned int total = 0;
    for ( auto it = s.begin();
          it != s.end();
          ++it ) {
        total += (*it)->getSize();
    }
    return total;
}

void update_summaries( Object *key,
                       std::set< Object * >& tgtSet,
                       GroupSum_t& pgs,
                       TypeTotalSum_t& tts,
                       SizeSum_t& ssum )
{
    string mytype = key->getType();
    unsigned gsize = tgtSet.size();
    // per group summary
    auto git = pgs.find(mytype);
    if (git == pgs.end()) {
        pgs[mytype] = std::move(std::vector< Summary * >());
    }
    unsigned int total_size = sumSize( tgtSet );
    Summary *s = new Summary( gsize, total_size, 1 );
    // -- third parameter is number of groups which is simply 1 here.
    pgs[mytype].push_back(s);
    // type total summary
    auto titer = tts.find(mytype);
    if (titer == tts.end()) {
        Summary *t = new Summary( gsize, total_size, 1 );
        tts[mytype] = t;
    } else {
        tts[mytype]->size += total_size;
        tts[mytype]->num_objects += tgtSet.size();
        tts[mytype]->num_groups++;
    }
    // size summary
    auto sit = ssum.find(gsize);
    if (sit == ssum.end()) {
        Summary *u = new Summary( gsize, total_size, 1 );
        ssum[gsize] = u;
    } else {
        ssum[gsize]->size += total_size;
        ssum[gsize]->num_groups++;
        // Doesn't make sense to make use of the num_objects field.
    }
}

void update_summary_from_keyset( KeySet_t &keyset,
                                 GroupSum_t &per_group_summary,
                                 TypeTotalSum_t &type_total_summary,
                                 SizeSum_t &size_summary )
{
    for ( auto it = keyset.begin();
          it != keyset.end();
          ++it ) {
        Object *key = it->first;
        std::set< Object * > *tgtSet = it->second;
        // TODO TODO 7 March 2016 - Put into CSV file.
        cout << "[ " << key->getType() << " ]: " << tgtSet->size() << endl;
        update_summaries( key,
                          *tgtSet,
                          per_group_summary,
                          type_total_summary,
                          size_summary );
    }
}


void summarize_reference_stability( EdgeSrc2Type_t &stability,
                                    EdgeSummary_t &my_refsum,
                                    Object2EdgeSrcMap_t &my_obj2ref )
{
    // Check each object to see if it's stable...(see ObjectRefType)
    for ( auto it = my_obj2ref.begin();
          it != my_obj2ref.end();
          ++it ) {
        // my_obj2ref : Object2EdgeSrcMap_t is a reverse map of sorts.
        //      For each object in the map, it maps to a vector of references
        //          that point/refer to that object
        // This Object is the target
        Object *obj = it->first;
        // This is the vector of references
        std::vector< EdgeSrc_t > reflist = it->second;
        // Convert to a set in order to remove possible duplicates
        std::set< EdgeSrc_t > refset( reflist.begin(), reflist.end() );
        if (!obj) {
            continue;
        }
        // obj is not NULL.
        // if ( (refset.size() <= 1) && (obj->wasLastUpdateNull() != true) ) {
        if (refset.size() <= 1) {
            obj->setRefTargetType( ObjectRefType::SINGLY_OWNED );
        } else {
            obj->setRefTargetType( ObjectRefType::MULTI_OWNED );
        }
    }
    // Check each edge source to see if its stable...(see EdgeSrcType)
    for ( auto it = my_refsum.begin();
          it != my_refsum.end();
          ++it ) {
        // my_refsum : EdgeSummary_t is a map from reference to a vector of
        //      objects that the reference pointed to. A reference is a pair
        //      of (Object pointer, fieldId).
        // Get the reference and deconstruct into parts.
        EdgeSrc_t ref = it->first;
        Object *obj = std::get<0>(ref); 
        FieldId_t fieldId = std::get<1>(ref); 
        // Get the vector/list of object pointers
        std::vector< Object * > objlist = it->second;
        // We need to make sure that all elements are not duplicates.
        std::set< Object * > objset( objlist.begin(), objlist.end() );
        // Is NULL in there?
        auto findit = objset.find(NULL);
        bool nullflag = (findit != objset.end());
        if (nullflag) {
            // If NULL is in the list, remove NULL.
            objset.erase(findit);
        }
        unsigned int size = objset.size();
        if (size == 1) {
            // The reference only points to one object!
            // Convert object set back into a vector.
            std::vector< Object * > tmplist( objset.begin(), objset.end() );
            assert( tmplist.size() == 1 );
            Object *tgt = tmplist[0];
            // Check for source and target death times
            // The checks for NULL are probably NOT necessary.
            assert(obj != NULL);
            VTime_t src_dtime = obj->getDeathTime();
            assert(tgt != NULL);
            VTime_t tgt_dtime = tgt->getDeathTime();
            if (tgt_dtime != src_dtime) {
                // UNSTABLE if target and source deathtimes are different
                // NOTE: This used to be UNSTABLE if target died before source.
                //       We are using a more conservative definition of stability here.
                stability[ref] = EdgeSrcType::UNSTABLE;
            } else {
                stability[ref] = (tgt->wasLastUpdateNull() ? EdgeSrcType::UNSTABLE : EdgeSrcType::STABLE);
            }
        } else {
            // objlist is of length > 1
            // This may still be stable if all objects in
            // the list are STABLE. Assume they're all stable unless
            // otherwise proven.
            stability[ref] = EdgeSrcType::SERIAL_STABLE; // aka serial monogamist
            for ( auto objit = objset.begin();
                  objit != objset.end();
                  ++objit ) {
                ObjectRefType objtype = (*objit)->getRefTargetType();
                if (objtype == ObjectRefType::MULTI_OWNED) {
                    stability[ref] = EdgeSrcType::UNSTABLE;
                    break;
                } else if (objtype == ObjectRefType::UNKNOWN) {
                    // Not sure if this is even possible.
                    // TODO: Make this an assertion?
                    // Let's assume that UNKNOWNs make it UNSTABLE.
                    stability[ref] = EdgeSrcType::UNSTABLE;
                    break;
                } // else continue
            }
        }
    }
}

// ---------------------------------------------------------------------------
// ------[ OUTPUT FUNCTIONS ]-------------------------------------------------
// ---------------------------------------------------------------------------
void output_size_summary( string &dgroups_filename,
                          SizeSum_t &size_summary )
{
    ofstream dgroups_file(dgroups_filename);
    dgroups_file << "\"num_objects\",\"size_bytes\",\"num_groups\"" << endl;
    for ( auto it = size_summary.begin();
          it != size_summary.end();
          ++it ) {
        unsigned int gsize = it->first;
        Summary *s = it->second;
        dgroups_file << s->num_objects << ","
            << s->size << ","
            << s->num_groups << endl;
    }
    dgroups_file.close();
}

void output_type_summary( string &dgroups_by_type_filename,
                          TypeTotalSum_t &type_total_summary )
{
    ofstream dgroups_by_type_file(dgroups_by_type_filename);
    for ( TypeTotalSum_t::iterator it = type_total_summary.begin();
          it != type_total_summary.end();
          ++it ) {
        string myType = it->first;
        Summary *s = it->second;
        dgroups_by_type_file << myType << "," 
            << s->size << ","
            << s->num_groups  << ","
            << s->num_objects << endl;
    }
    dgroups_by_type_file.close();
}

// All sorts of hacky debug function. Very brittle.
void debug_type_algo( Object *object,
                      string& dgroup_kind )
{
    KeyType ktype = object->getKeyType();
    unsigned int objId = object->getId();
    if (ktype == KeyType::UNKNOWN_KEYTYPE) {
        cout << "ERROR: objId[ " << objId << " ] : "
             << "Keytype not set but algo determines [ " << dgroup_kind << " ]" << endl;
        return;
    }
    if (dgroup_kind == "CYCLE") {
        if (ktype != KeyType::CYCLE) {
            goto fail;
        }
    } else if (dgroup_kind == "CYCKEY") {
        if (ktype != KeyType::CYCLEKEY) {
            goto fail;
        }
    } else if (dgroup_kind == "DAG") {
        if (ktype != KeyType::DAG) {
            goto fail;
        }
    } else if (dgroup_kind == "DAGKEY") {
        if (ktype != KeyType::DAGKEY) {
            goto fail;
        }
    } else {
        cout << "ERROR: objId[ " << objId << " ] : "
             << "Unknown key type: " << dgroup_kind << endl;
    }
    return;
fail:
    cout << "ERROR: objId[ " << objId << " ] : "
         << "Keytype [ " << keytype2str(ktype) << " ]"
         << " doesn't match [ " << dgroup_kind << " ]" << endl;
    return;
}


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

void output_cycles( KeySet_t &keyset,
                    string &cycle_filename,
                    std::set<int> &node_set )
{
    ofstream cycle_file(cycle_filename);
    cycle_file << "---------------[ CYCLES ]-------------------------------------------------------" << endl;
    for ( KeySet_t::iterator it = keyset.begin();
          it != keyset.end();
          ++it ) {
        Object *obj = it->first;
        set< Object * > *sptr = it->second;
        unsigned int keyObjId = obj->getId();
        cycle_file << keyObjId;
        for ( set<Object *>::iterator tmp = sptr->begin();
              tmp != sptr->end();
              ++tmp ) {
            unsigned int tmpId = (*tmp)->getId();
            if (tmpId != keyObjId) {
                cycle_file  << "," << tmpId;
            }
            node_set.insert((*tmp)->getId());
        }
        cycle_file << endl;
    }
    cycle_file << "---------------[ CYCLES END ]---------------------------------------------------" << endl;
    cycle_file.close();
}


// ----------------------------------------------------------------------
// Output the map of death context site -> count of obects dying
void output_context_summary( string &context_death_count_filename,
                             ExecState &exstate )
{
    ofstream cdeathfile(context_death_count_filename);
    cdeathfile << "\"method1\",\"method2\",\"total_count\",\"death_count\""
               << endl;
    for ( auto it = exstate.begin_execPairCountMap();
          it != exstate.end_execPairCountMap();
          ++it ) {
        ContextPair cpair = it->first;
        unsigned int total = it->second;
        unsigned int dcount;
        auto iter = exstate.m_deathPairCountMap.find(cpair);
        dcount = ( (iter == exstate.m_deathPairCountMap.end())
                   ? 0
                   : exstate.m_deathPairCountMap[cpair] );
        Method *first = std::get<0>(cpair); 
        Method *second = std::get<1>(cpair); 
        string meth1_name = (first ? first->getName() : "NONAME");
        string meth2_name = (second ? second->getName() : "NONAME");
        cdeathfile << meth1_name << "," 
                   << meth2_name << "," 
                   << total << endl;
    }
    cdeathfile.close();
}


void output_reference_summary( string &reference_summary_filename,
                               string &ref_reverse_summary_filename,
                               string &stability_summary_filename,
                               EdgeSummary_t &my_refsum,
                               Object2EdgeSrcMap_t &my_obj2ref,
                               EdgeSrc2Type_t &stability )
{
    ofstream edge_summary_file(reference_summary_filename);
    ofstream reverse_summary_file(ref_reverse_summary_filename);
    ofstream stability_summary_file(stability_summary_filename);
    //
    // Summarizes the objects pointed at by the reference (object Id + field Id)
    // This is the REF-SUMMARY output file. In garbology, the ReferenceReader
    // is responsible for reading it.
    for ( auto it = my_refsum.begin();
          it != my_refsum.end();
          ++it ) {
        // Key is an edge source (which we used to call 'ref')
        EdgeSrc_t ref = it->first;
        Object *obj = std::get<0>(ref); 
        FieldId_t fieldId = std::get<1>(ref); 
        // Value is a vector of Object pointers
        std::vector< Object * > objlist = it->second;
        ObjectId_t objId = (obj ? obj->getId() : 0);
        edge_summary_file << objId << ","      // 1 - object Id
                          << fieldId << ",";   // 2 - field Id
        unsigned int actual_size = 0;
        for ( auto vecit = objlist.begin();
              vecit != objlist.end();
              ++vecit ) {
            if (*vecit) {
                ++actual_size;
            }
        }
        edge_summary_file << actual_size; // 3 - number of objects pointed at
        if (actual_size > 0) {
            for ( auto vecit = objlist.begin();
                  vecit != objlist.end();
                  ++vecit ) {
                if (*vecit) {
                    edge_summary_file << "," << (*vecit)->getId(); // 4+ - objects pointed at
                }
            }
        }
        edge_summary_file << endl;
    }
    //
    // Summarizes the reverse, which is for each object (using its Id), give
    // the references that point to it.
    // This is the REF-REVERSE-SUMMARY output file. In garbology, the ReverseRefReader
    // is responsible for reading it.
    for ( auto it = my_obj2ref.begin();
          it != my_obj2ref.end();
          ++it ) {
        Object *obj = it->first;
        std::vector< EdgeSrc_t > reflist = it->second;
        if (!obj) {
            continue;
        }
        // obj is not NULL.
        ObjectId_t objId = obj->getId();
        reverse_summary_file << objId << ","     // 1 - object Id
                             << reflist.size();  // 2 - number of references in lifetime
        for ( auto vecit = reflist.begin();
              vecit != reflist.end();
              ++vecit ) {
            Object *srcObj = std::get<0>(*vecit); 
            FieldId_t fieldId = std::get<1>(*vecit); 
            ObjectId_t srcId = (srcObj ? srcObj->getId() : 0);
            reverse_summary_file << ",(" << srcId << "," << fieldId << ")"; // 3+ - list of incoming references
        }
        reverse_summary_file << endl;
    }
    //
    // Summarize the stability attributes of features.
    //     output file is   *-STABILITY-SUMMARY.csv
    for ( auto it = stability.begin();
          it != stability.end();
          ++it ) {
        EdgeSrc_t ref = it->first;
        EdgeSrcType reftype = it->second;
        Object *obj = std::get<0>(ref); 
        FieldId_t fieldId = std::get<1>(ref); 
        ObjectId_t objId = (obj ? obj->getId() : 0);
        stability_summary_file << objId << ","            // 1 - object Id
                               << fieldId << ","          // 2 - field Id
                               << edgesrctype2shortstr(reftype)    // 3 - reference stability type
                               << endl;
    }
    // Close the files.
    edge_summary_file.close();
    reverse_summary_file.close();
    stability_summary_file.close();
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
    cout << "#     git version: " <<  build_git_sha << endl;
    cout << "#     build date : " <<  build_git_time << endl;
    cout << "---------------[ START ]-----------------------------------------------------------" << endl;
    //--------------------------------------------------------------------------------
    // Setup filenames for output files:
    string basename(argv[5]);
    string objectinfo_filename( basename + "-OBJECTINFO.txt" );
    string edgeinfo_filename( basename + "-EDGEINFO.txt" );
    string summary_filename( basename + "-SUMMARY.csv" );
    string dsite_filename( basename + "-DSITES.csv" );
    string dgroups_filename( basename + "-DGROUPS.csv" );
    string dgroups_by_type_filename( basename + "-DGROUPS-BY-TYPE.csv" );
    string reference_summary_filename( basename + "-REF-SUMMARY.csv" );
    string ref_reverse_summary_filename( basename + "-REF-REVERSE-SUMMARY.csv" );
    string stability_summary_filename( basename + "-STABILITY-SUMMARY.csv" );

    string call_context_filename( basename + "-CALL-CONTEXT.csv" );
    ofstream call_context_file(call_context_filename);
    Exec.set_output( &call_context_file );

    string nodemap_filename( basename + "-NODEMAP.csv" );
    ofstream nodemap_file(nodemap_filename);
    Exec.set_nodefile( &nodemap_file );

    // TODO string cycle_filename( basename + "-CYCLES.csv" );
    // TODO: UNUSED string typeinfo_filename( basename + "-TYPEINFO.txt" );
    // TODO: UNUSED string context_death_count_filename( basename + "-CONTEXT-DCOUNT.csv" );
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
    ofstream edge_info_file(edgeinfo_filename);
    edge_info_file << "---------------[ EDGE INFO ]----------------------------------------------------" << endl;
    unsigned int total_objects = read_trace_file_part1(f);
    exit(100);
    unsigned int final_time = Exec.NowUp() + 1;
    unsigned int final_time_alloc = Heap.getAllocTime();
    cout << "Done at update time: " << Exec.NowUp() << endl;
    cout << "Total objects: " << total_objects << endl;
    cout << "Heap.size:     " << Heap.size() << endl;
    // TODO: Why is this commented out? 2018-11-10
    // assert( total_objects == Heap.size() );

    // End of program processing.
    // TODO: Document what happens here.
    Heap.end_of_program( final_time, edge_info_file );

    ofstream summary_file(summary_filename);
    summary_file << "---------------[ SUMMARY INFO ]----------------------------------------------------" << endl;
    summary_file << "number_of_objects," << Heap.size() << endl
                 << "died_by_stack," << Heap.getTotalDiedByStack2() << endl
                 << "died_by_heap," << Heap.getTotalDiedByHeap2() << endl
                 << "died_by_global," << Heap.getTotalDiedByGlobal() << endl
                 << "died_at_end," << Heap.getTotalDiedAtEnd() << endl
                 << "last_update_null," << Heap.getTotalLastUpdateNull() << endl
                 << "last_update_null_heap," << Heap.getTotalLastUpdateNullHeap() << endl
                 << "last_update_null_stack," << Heap.getTotalLastUpdateNullStack() << endl
                 << "last_update_null_size," << Heap.getSizeLastUpdateNull() << endl
                 << "last_update_null_heap_size," << Heap.getSizeLastUpdateNullHeap() << endl
                 << "last_update_null_stack_size," << Heap.getSizeLastUpdateNullStack() << endl
                 << "died_by_stack_only," << Heap.getDiedByStackOnly() << endl
                 << "died_by_stack_after_heap," << Heap.getDiedByStackAfterHeap() << endl
                 << "died_by_stack_only_size," << Heap.getSizeDiedByStackOnly() << endl
                 << "died_by_stack_after_heap_size," << Heap.getSizeDiedByStackAfterHeap() << endl
                 << "no_death_sites," << Heap.getNumberNoDeathSites() << endl
                 << "size_died_by_stack," << Heap.getSizeDiedByStack() << endl
                 << "size_died_by_heap," << Heap.getSizeDiedByHeap() << endl
                 << "size_died_at_end," << Heap.getSizeDiedAtEnd() << endl
                 << "vm_RC_zero," << Heap.getVMObjectsRefCountZero() << endl
                 << "vm_RC_positive," << Heap.getVMObjectsRefCountPositive() << endl
                 << "max_live_size," << Heap.maxLiveSize() << endl
                 << "main_func_uptime," << Exec.get_main_func_uptime() << endl
                 << "main_func_alloctime," << Exec.get_main_func_alloctime() << endl
                 << "final_time," << final_time << endl
                 << "final_time_alloc," << final_time_alloc << endl;
    summary_file << "---------------[ SUMMARY INFO END ]------------------------------------------------" << endl;
    summary_file.close();
    //---------------------------------------------------------------------
    ofstream dsite_file(dsite_filename);
    dsite_file << "---------------[ DEATH SITES INFO ]------------------------------------------------" << endl;
    for ( DeathSitesMap::iterator it = Heap.begin_dsites();
          it != Heap.end_dsites();
          ++it ) {
        Method *meth = it->first;
        set<string> *types = it->second;
        if (meth && types) {
            dsite_file << meth->getName() << "," << types->size();
            for ( set<string>::iterator sit = types->begin();
                  sit != types->end();
                  ++sit ) {
                dsite_file << "," << *sit;
            }
            dsite_file << endl;
        }
    }
    dsite_file << "---------------[ DEATH SITES INFO END ]--------------------------------------------" << endl;
    dsite_file.close();
    dsite_file << "---------------[ DONE ]------------------------------------------------------------" << endl;
    cout << "---------------[ DONE ]------------------------------------------------------------" << endl;
    cout << "#     git version: " <<  build_git_sha << endl;
    cout << "#     build date : " <<  build_git_time << endl;
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
}

void methods_main(int argc, char* argv[])
{
}
