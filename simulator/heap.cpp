#include "heap.hpp"

// -- Global flags
bool HeapState::do_refcounting = true;
bool HeapState::debug = false;
unsigned int Object::g_counter = 0;

string keytype2str( KeyType ktype )
{
    if (ktype == KeyType::DAG) {
        return "DAG";
    } else if (ktype == KeyType::DAGKEY) {
        return "DAGKEY";
    } else if (ktype == KeyType::CYCLE) {
        return "CYCLE";
    } else if (ktype == KeyType::CYCLEKEY) {
        return "CYCLEKEY";
    } else if (ktype == KeyType::UNKNOWN_KEYTYPE) {
        return "UNKNOWN_KEYTYPE";
    }
    assert(0); // Shouldn't make it here.
}

string lastevent2str( LastEvent le )
{
    if (le == LastEvent::NEWOBJECT) {
        return "NEWOBJECT";
    } else if (le == LastEvent::ROOT) {
        return "ROOT";
    } else if (le == LastEvent::DECRC) {
        return "DECRC";
    } else if (le == LastEvent::UPDATE_UNKNOWN) {
        return "UPDATE_UNKNOWN";
    } else if (le == LastEvent::UPDATE_AWAY_TO_NULL) {
        return "UPDATE_AWAY_TO_NULL";
    } else if (le == LastEvent::UPDATE_AWAY_TO_VALID) {
        return "UPDATE_AWAY_TO_VALID";
    } else if (le == LastEvent::OBJECT_DEATH_AFTER_ROOT) {
        return "OBJECT_DEATH_AFTER_ROOT";
    } else if (le == LastEvent::OBJECT_DEATH_AFTER_UPDATE) {
        return "OBJECT_DEATH_AFTER_UPDATE";
    } else if (le == LastEvent::OBJECT_DEATH_AFTER_ROOT_DECRC) {
        return "OBJECT_DEATH_AFTER_ROOT_DECRC";
    } else if (le == LastEvent::OBJECT_DEATH_AFTER_UPDATE_DECRC) {
        return "OBJECT_DEATH_AFTER_UPDATE_DECRC";
    } else if (le == LastEvent::OBJECT_DEATH_AFTER_UNKNOWN) {
        return "OBJECT_DEATH_AFTER_UNKNOWN";
    } else if (le == LastEvent::END_OF_PROGRAM_EVENT) {
        return "END_OF_PROGRAM_EVENT";
    } else if (le == LastEvent::UNKNOWN_EVENT) {
        return "UNKNOWN_EVENT";
    }
    assert(0); // Shouldn't make it here.
}


bool is_object_death( LastEvent le )
{
    return ( (le == LastEvent::OBJECT_DEATH_AFTER_ROOT) || 
             (le == LastEvent::OBJECT_DEATH_AFTER_UPDATE) ||
             (le == LastEvent::OBJECT_DEATH_AFTER_ROOT_DECRC) ||
             (le == LastEvent::OBJECT_DEATH_AFTER_UPDATE_DECRC) ||
             (le == LastEvent::OBJECT_DEATH_AFTER_UNKNOWN) );
}

// TODO
void output_edge( Edge *edge,
                  unsigned int endtime,
                  EdgeState estate,
                  ofstream &edge_info_file )
{
    Object *source = edge->getSource();
    Object *target = edge->getTarget();
    assert(source);
    assert(target);
    unsigned int srcId = source->getId();
    unsigned int tgtId = target->getId();
    // Format is
    // srcId, tgtId, createTime, deathTime, sourceField, edgeState
    edge_info_file << srcId << ","
        << tgtId << ","
        << edge->getCreateTime() << ","
        << endtime << ","
        << edge->getSourceField() << ","
        << static_cast<int>(estate) << endl;
}


// =================================================================

Object* HeapState::allocate( unsigned int id,
                             unsigned int size,
                             char kind,
                             string type,
                             AllocSite *site, 
                             string &nonjavalib_site_name,
                             unsigned int els,
                             Thread *thread,
                             unsigned int create_time )
{
    // Design decision: allocation time isn't 0 based.
    this->m_alloc_time += size;
    Object* obj = new Object( id,
                              size,
                              kind,
                              type,
                              site,
                              nonjavalib_site_name,
                              els,
                              thread,
                              create_time,
                              this );
    // Add to object map
    this->m_objects[obj->getId()] = obj;
    // Add to live set. Given that it's a set, duplicates are not a problem.
    this->m_liveset.insert(obj);

#ifndef DEBUG_SPECJBB
    if (this->m_objects.size() % 100000 == 0) {
        cout << "OBJECTS: " << this->m_objects.size() << endl;
    }
#endif // DEBUG_SPECJBB
    unsigned long int temp = this->m_liveSize + obj->getSize();
    // Max live size calculation
    this->m_liveSize = ( (temp < this->m_liveSize) ? ULONG_MAX : temp );
    if (this->m_maxLiveSize < this->m_liveSize) {
        this->m_maxLiveSize = this->m_liveSize;
    }
    return obj;
}

Object *HeapState::allocate( Object * obj )
{
    // Design decision: allocation time isn't 0 based.
    this->m_alloc_time += obj->getSize();
    // Add to object map
    this->m_objects[obj->getId()] = obj;
    // Add to live set. Given that it's a set, duplicates are not a problem.
    this->m_liveset.insert(obj);

    if (this->m_objects.size() % 100000 == 0) {
        cout << "OBJECTS: " << this->m_objects.size() << endl;
    }
    unsigned long int temp = this->m_liveSize + obj->getSize();
    // Max live size calculation
    this->m_liveSize = ( (temp < this->m_liveSize) ? ULONG_MAX : temp );
    if (this->m_maxLiveSize < this->m_liveSize) {
        this->m_maxLiveSize = this->m_liveSize;
    }
    return obj;
}

// -- Manage heap
Object* HeapState::getObject(unsigned int id)
{
    ObjectMap::iterator p = m_objects.find(id);
    if (p != m_objects.end()) {
        return (*p).second;
    }
    else {
        return 0;
    }
}

Edge* HeapState::make_edge( Object *source,
                            FieldId_t field_id,
                            Object *target,
                            unsigned int cur_time )
{
    Edge* new_edge = new Edge( source, field_id,
                               target, cur_time );
    m_edges.insert(new_edge);
    assert(target != NULL);
    // TODO target->setPointedAtByHeap();

#ifndef DEBUG_SPECJBB
    if (m_edges.size() % 100000 == 0) {
        cout << "EDGES: " << m_edges.size() << endl;
    }
#endif // DEBUG_SPECJBB

    return new_edge;
}

Edge *
HeapState::make_edge( Edge *edgeptr )
{
    // Edge* new_edge = new Edge( source, field_id,
    //                            target, cur_time );
    this->m_edges.insert(edgeptr);
    Object *target = edgeptr->getTarget();
    assert(target != NULL);
    // TODO target->setPointedAtByHeap();

    if (this->m_edges.size() % 100000 == 0) {
        cout << "EDGES: " << this->m_edges.size() << endl;
    }

    return edgeptr;
}

void HeapState::makeDead( Object *obj,
                          unsigned int death_time,
                          ofstream &eifile )
{
    this->__makeDead( obj,
                      death_time,
                      &eifile );
}

void HeapState::makeDead_nosave( Object *obj,
                                 unsigned int death_time,
                                 ofstream &eifile )
{
    this->__makeDead( obj,
                      death_time,
                      &eifile );
}

void HeapState::__makeDead( Object *obj,
                            unsigned int death_time,
                            ofstream *eifile_ptr )
{
    if (!obj->isDead()) {
        // Livesize maintenance
        auto iter = this->m_liveset.find(obj);
        if (iter != this->m_liveset.end()) {
            unsigned long int temp = this->m_liveSize - obj->getSize();
            if (temp > this->m_liveSize) {
                // OVERFLOW, underflow?
                this->m_liveSize = 0;
                cerr << "UNDERFLOW of substraction." << endl;
                // TODO If this happens, maybe we should think about why it happens.
            } else {
                // All good. Fight on.
                this->m_liveSize = temp;
            }
        }
        // Get the actual reason
        Reason objreason = obj->getReason();
        // Note that the death_time might be incorrect
        obj->makeDead( death_time,
                       this->m_alloc_time,
                       EdgeState::DEAD_BY_OBJECT_DEATH_NOT_SAVED,
                       *eifile_ptr,
                       objreason );
    }
}

// TODO Documentation :)
void HeapState::update_death_counters( Object *obj )
{
    unsigned int obj_size = obj->getSize();
    // VERSION 1
    // TODO: This could use some refactoring.
    //
    // TODO TODO TODO DEBUG
    // TODO TODO TODO DEBUG
    // TODO TODO TODO DEBUG
    // TODO TODO TODO END DEBUG
    // Check for end of program kind first.
    if ( (obj->getReason() ==  Reason::END_OF_PROGRAM_REASON) ||
         obj->getDiedAtEndFlag() ) {
        this->m_totalDiedAtEnd++;
        this->m_sizeDiedAtEnd += obj_size;
    } else if ( obj->getDiedByStackFlag() ||
         ( ((obj->getReason() == Reason::STACK) ||
            (obj->getLastEvent() == LastEvent::ROOT)) &&
            !obj->getDiedByHeapFlag() )
         ) {
        if (m_obj_debug_flag) {
            cout << "S> " << obj->info2();
        }
        this->m_totalDiedByStack_ver2++;
        this->m_sizeDiedByStack += obj_size;
        if (obj->wasPointedAtByHeap()) {
            this->m_diedByStackAfterHeap++;
            this->m_diedByStackAfterHeap_size += obj_size;
            if (m_obj_debug_flag) {
                cout << " AH>" << endl;
            }
        } else {
            this->m_diedByStackOnly++;
            this->m_diedByStackOnly_size += obj_size;
            if (m_obj_debug_flag) {
                cout << " SO>" << endl;
            }
        }
        if (obj->wasLastUpdateNull()) {
            this->m_totalUpdateNullStack++;
            this->m_totalUpdateNullStack_size += obj_size;
        }
    } else if ( obj->getDiedByHeapFlag() ||
                (obj->getReason() == Reason::HEAP) ||
                (obj->getLastEvent() == LastEvent::UPDATE_AWAY_TO_NULL) ||
                (obj->getLastEvent() == LastEvent::UPDATE_AWAY_TO_VALID) ||
                obj->wasPointedAtByHeap() ||
                obj->getDiedByGlobalFlag() ) {
        // TODO: If we decide that Died By GLOBAL is a separate category from
        //       died by HEAP, we will need to change the code in this block.
        //       - RLV 2016-0803
        if (m_obj_debug_flag) {
            cout << "H> " << obj->info2();
        }
        this->m_totalDiedByHeap_ver2++;
        this->m_sizeDiedByHeap += obj_size;
        obj->setDiedByHeapFlag();
        if (obj->wasLastUpdateNull()) {
            this->m_totalUpdateNullHeap++;
            this->m_totalUpdateNullHeap_size += obj_size;
            if (m_obj_debug_flag) {
                cout << " NL>" << endl;
            }
        } else {
            if (m_obj_debug_flag) {
                cout << " VA>" << endl;
            }
        }
        // Check to see if the last update was from global
        if (obj->getDiedByGlobalFlag()) {
            this->m_totalDiedByGlobal++;
            this->m_sizeDiedByGlobal += obj_size;
        }
    } else {
        // cout << "X: ObjectID [" << obj->getId() << "][" << obj->getType()
        //      << "] RC = " << obj->getRefCount() << " maxRC: " << obj->getMaxRefCount()
        //      << " Atype: " << obj->getKind() << endl;
        // All these objects were never a target of an Update event. For example,
        // most VM allocated objects (by event V) are never targeted by
        // the Java user program, and thus end up here. We consider these
        // to be "STACK" caused death as we can associate these with the main function.
        if (m_obj_debug_flag) {
            cout << "S> " << obj->info2() << " SO>" << endl;
        }
        this->m_totalDiedByStack_ver2++;
        this->m_sizeDiedByStack += obj_size;
        this->m_diedByStackOnly++;
        this->m_diedByStackOnly_size += obj_size;
        if (obj->wasLastUpdateNull()) {
            this->m_totalUpdateNullStack++;
            this->m_totalUpdateNullStack_size += obj_size;
        }
    }
    if (obj->wasLastUpdateNull()) {
        this->m_totalUpdateNull++;
        this->m_totalUpdateNull_size += obj_size;
    }
    // END VERSION 1
    
    // VM type objects
    if (obj->getKind() == 'V') {
        if (obj->getRefCount() == 0) {
            m_vm_refcount_0++;
        } else {
            m_vm_refcount_positive++;
        }
    }
}

Method * HeapState::get_method_death_site( Object *obj )
{
    Method *dsite = obj->getDeathSite();
    if (obj->getDiedByHeapFlag()) {
        // DIED BY HEAP
        if (!dsite) {
            if (obj->wasDecrementedToZero()) {
                // So died by heap but no saved death site. First alternative is
                // to look for the a site that decremented to 0.
                dsite = obj->getMethodDecToZero();
            } else {
                // TODO: No dsite here yet
                // TODO TODO TODO
                // This probably should be the garbage cycles. Question is 
                // where should we get this?
            }
        }
    } else {
        if (obj->getDiedByStackFlag()) {
            // DIED BY STACK.
            //   Look for last heap activity.
            dsite = obj->getLastMethodDecRC();
        }
    }
    return dsite;
}

// TODO Documentation :)
void HeapState::end_of_program( unsigned int cur_time,
                                ofstream &eifile )
{
    this->__end_of_program( cur_time,
                            &eifile,
                            true );
}

// TODO Documentation :)
void HeapState::end_of_program_nosave( unsigned int cur_time )
{
    this->__end_of_program( cur_time,
                            NULL,
                            false );
}

// TODO Documentation :)
void HeapState::__end_of_program( unsigned int cur_time,
                                  ofstream *eifile_ptr,
                                  bool save_edge_flag )
{
    if (save_edge_flag) {
        assert(eifile_ptr);
    }
    // -- Set death time of all remaining live objects
    //    Also set the flags for the interesting classifications.
    for ( ObjectMap::iterator i = m_objects.begin();
          i != m_objects.end();
          ++i ) {
        Object* obj = i->second;
        if (obj->isLive(cur_time)) {
            // OLD DEBUG: cerr << "ALIVE" << endl;
            // Go ahead and ignore the call to HeapState::makeDead
            // as we won't need to update maxLiveSize here anymore.
            if (!obj->isDead()) {
                // A hack: not sure why this check may be needed.
                // TODO: Debug this.
                if (true) {
                    obj->makeDead( cur_time,
                                   this->m_alloc_time,
                                   EdgeState::DEAD_BY_PROGRAM_END,
                                   *eifile_ptr,
                                   Reason::END_OF_PROGRAM_REASON );
                } else {
                    obj->makeDead_nosave( cur_time,
                                          this->m_alloc_time,
                                          EdgeState::DEAD_BY_PROGRAM_END,
                                          *eifile_ptr,
                                          Reason::END_OF_PROGRAM_REASON );
                }
                obj->setActualLastTimestamp( cur_time );
            }
            string progend("PROG_END");
            obj->unsetDiedByStackFlag();
            obj->unsetDiedByHeapFlag();
            obj->setDiedAtEndFlag();
            obj->setReason( Reason::END_OF_PROGRAM_REASON, cur_time );
            obj->setLastEvent( LastEvent::END_OF_PROGRAM_EVENT );
            // Unset the death site.
                obj->setDeathContextSiteName( progend, 1 );
                obj->setDeathContextSiteName( progend, 2 );
        } else {
            if (obj->getDeathTime() == cur_time) {
                string progend("PROG_END");
                obj->unsetDiedByStackFlag();
                obj->unsetDiedByHeapFlag();
                obj->setDiedAtEndFlag();
                obj->setReason( Reason::END_OF_PROGRAM_REASON, cur_time );
                obj->setLastEvent( LastEvent::END_OF_PROGRAM_EVENT );
                // Unset the death site.
                obj->setDeathContextSiteName( progend, 1 );
                obj->setDeathContextSiteName( progend, 2 );
            }
            else {
                // TODO
                if (obj->getDeathTime() > cur_time) {
                    cerr << "Object ID[" << obj->getId() << "] has later death time["
                         << obj->getDeathTime() << "than final time: " << cur_time << endl;
                }
            }
        }
        // Do the count of heap vs stack loss here.
        this->update_death_counters(obj);

        // Save method death site to map
        Method *dsite = this->get_method_death_site( obj );

        // Process the death sites
        if (dsite) {
            DeathSitesMap::iterator it = this->m_death_sites_map.find(dsite);
            if (it == this->m_death_sites_map.end()) {
                this->m_death_sites_map[dsite] = new set<string>; 
            }
            this->m_death_sites_map[dsite]->insert(obj->getType());
        } else {
            this->m_no_dsites_count++;
            // TODO if (obj->getDiedByHeapFlag()) {
            //     // We couldn't find a deathsite for something that died by heap.
            //     // TODO ?????? TODO
            // } else if (obj->getDiedByStackFlag()) {
            //     // ?
            // } else {
            // }
        }
    }
}

// TODO Documentation :)
inline void HeapState::set_candidate(unsigned int objId)
{
    this->m_candidate_map[objId] = true;
}

// TODO Documentation :)
inline void HeapState::unset_candidate(unsigned int objId)
{
    this->m_candidate_map[objId] = false;
}

// TODO Documentation :)
void HeapState::set_reason_for_cycles( deque< deque<int> >& cycles )
{
    for ( deque< deque<int> >::iterator it = cycles.begin();
          it != cycles.end();
          ++it ) {
        Reason reason = Reason::UNKNOWN_REASON;
        unsigned int last_action_time = 0;
        for ( deque<int>::iterator objit = it->begin();
              objit != it->end();
              ++objit ) {
            Object* object = this->getObject(*objit);
            unsigned int objtime = object->getLastActionTime();
            if (objtime > last_action_time) {
                reason = object->getReason();
                last_action_time = objtime;
            }
        }
        for ( deque<int>::iterator objit = it->begin();
              objit != it->end();
              ++objit ) {
            Object* object = this->getObject(*objit);
            object->setReason( reason, last_action_time );
        }
    }
}

// TODO Documentation :)
deque< deque<int> > HeapState::scan_queue( EdgeList& edgelist )
{
    deque< deque<int> > result;
    cout << "Queue size: " << this->m_candidate_map.size() << endl;
    for ( auto i = this->m_candidate_map.begin();
          i != this->m_candidate_map.end();
          ++i ) {
        int objId = i->first;
        bool flag = i->second;
        if (flag) {
            Object* object = this->getObject(objId);
            if (object) {
                if (object->getColor() == Color::BLACK) {
                    object->mark_red();
                    object->scan();
                    deque<int> cycle = object->collect_blue( edgelist );
                    if (cycle.size() > 0) {
                        result.push_back( cycle );
                    }
                }
            }
        }
    }
    this->set_reason_for_cycles( result );
    return result;
}

NodeId_t HeapState::getNodeId( ObjectId_t objId, bimap< ObjectId_t, NodeId_t >& bmap ) {
    bimap< ObjectId_t, NodeId_t >::left_map::const_iterator liter = bmap.left.find(objId);
    if (liter == bmap.left.end()) {
        // Haven't mapped a NodeId yet to this ObjectId
        NodeId_t nodeId = bmap.size();
        bmap.insert( bimap< ObjectId_t, NodeId_t >::value_type( objId, nodeId ) );
        return nodeId;
    } else {
        // We have a NodeId
        return liter->second;
    }
}

// TODO Documentation :)
void HeapState::scan_queue2( EdgeList &edgelist,
                             std::map<unsigned int, bool> &not_candidate_map )
{
    // Types
    typedef std::set< std::pair< ObjectId_t, unsigned int >, compclass > CandidateSet_t;
    //      TODO: what is the unsigned int part of the pair?
    typedef std::map< ObjectId_t, unsigned int > Object2Utime_t;
    //      TODO: what is the unsigned int value part of the map?
    //                  Utime == update time?

    CandidateSet_t candSet;
    Object2Utime_t utimeMap;

    unsigned int hit_total;
    unsigned int miss_total;
    ObjectPtrMap_t &whereis = this->m_whereis;
    KeySet_t &keyset = this->m_keyset;
    // keyset contains:
    //   key-object objects as keys
    //      ->   sets of objects that depend on key objects
    cout << "Queue size: " << this->m_candidate_map.size() << endl;
    // TODO
    // 1. Convert m_candidate_map to a Boost Graph Library
    // 2. Run SCC algorithm (or Weakly connected?)
    // 3. Run reachability from the SCCs to the rest
    // 4. ??? That's it?
    //
    // TODO: Add bimap
    //    objId <-> graph ID
    //
    // Get all the candidate objects and sort according to last update time.
    for ( auto i = this->m_candidate_map.begin();
          i != this->m_candidate_map.end();
          ++i ) {
        ObjectId_t objId = i->first;
        bool flag = i->second;
        if (flag) {
            // Is a candidate
            Object *obj = this->getObject(objId);
            if ( obj && (obj->getRefCount() > 0) ) {
                // Object exists and refcount is greater than 0.
                unsigned int uptime = obj->getLastActionTime();
                // DEBUG: Compare to getDeathTime
                // Insert (objId, update time) pair into candidate set
                candSet.insert( std::make_pair( objId, uptime ) );
                utimeMap[objId] = uptime;
                // Copy all edges from source 'obj'
                for ( auto p = obj->getEdgeMapBegin();
                      p != obj->getEdgeMapEnd();
                      ++p ) {
                    auto target_edge = p->second; // Edge pointer
                    if (target_edge) {
                        auto fieldId = target_edge->getSourceField(); // Field id type (unsigned int)
                        auto tgtObj = target_edge->getTarget(); // Object pointer
                        if (tgtObj) {
                            auto tgtId = tgtObj->getId(); // Object id
                            GEdge_t e(objId, tgtId);
                            edgelist.push_back(e);
                        }
                    }
                }
            } else {
                assert(obj);
                // Refcount is 0. Check to see that it is in whereis. TODO
            }
        } // if (flag)
    }
    cout << "Before whereis size: " << whereis.size() << endl;
    // Anything seen in this loop has a reference count (RefCount) greater than zero.
    // The 'whereis' maps an object to its key object (both Object pointers)
    while (!(candSet.empty())) {
        auto it = candSet.begin();
        if (it != candSet.end()) {
            auto rootId = it->first; // Object id
            auto uptime = it->second; // Unsigned int
            Object *root;
            Object *object = this->getObject(rootId);
            // DFS work stack - can't use 'stack' as a variable name
            std::deque< Object * > work;
            // The discovered set of objects.
            std::set< Object * > discovered;
            // Root goes in first.
            work.push_back(object);
            // Check to see if the object is already in there?
            auto itmap = whereis.find(object);
            if ( (itmap == whereis.end()) || 
                 (object == whereis[object]) ) {
                // NOT FOUND so...it's a root...for now.
                root = object;
                if (itmap == whereis.end()) {
                    // Haven't saved object in whereis yet.
                    whereis[object] = root;
                }
                auto keysetit = keyset.find(root);
                if (keysetit == keyset.end()) {
                    // Not found:
                    keyset[root] = new std::set< Object * >();
                }
                root->setKeyTypeIfNotKnown( KeyType::CYCLEKEY ); // Note: root == object
            } else {
                // So-called root isn't one because we have an entry in 'whereis'
                // and root != whereis[object]
                object->setKeyType( KeyType::CYCLE ); // object is a CYCLE object
                root = whereis[object]; // My real root so far
                auto obj_it = keyset.find(object);
                if (obj_it != keyset.end()) {
                    // So we found that object is not a root but has an entry
                    // in keyset. We need to:
                    //    1. Remove from keyset
                    auto *sptr = obj_it->second; // set of Object pointers
                    keyset.erase(obj_it);
                    //    2. Add root if root is not there.
                    keyset[root] = sptr;
                    // TODO Delete these?
                    // TODO: keyset[root] = new std::set< Object * >(*sptr);
                    // TODO: delete sptr;
                } else {
                    // Well, this shouldn't be possible? TODO
                    // TODO: Should we assert(false)?
                    assert(false);
                    // Create an empty set for root in keyset
                    keyset[root] = new std::set< Object * >();
                }
                // Add object to root's set
                keyset[root]->insert(object);
            }
            assert( root != NULL );
            // Depth First Search
            while (!work.empty()) {
                auto cur = work.back(); // Object pointer
                auto curId = cur->getId(); // Object id
                work.pop_back();
                // Look in whereis
                auto itwhere = whereis.find(cur);
                // Look in discovered
                auto itdisc = discovered.find(cur);
                // Look in candidate
                unsigned int uptime = utimeMap[curId];
                auto itcand = candSet.find( std::make_pair( curId, uptime ) );
                if (itcand != candSet.end()) {
                    // Found in candidate set so remove it.
                    candSet.erase(itcand);
                }
                assert(cur);
                if (itdisc == discovered.end()) {
                    // Not yet seen by DFS.
                    discovered.insert(cur);
                    auto other_root = whereis[cur]; // Object pointer
                    if (!other_root) {
                        // 'cur' not found in 'whereis'
                        keyset[root]->insert(cur);
                        whereis[cur] = root;
                    } else {
                        auto other_time = other_root->getDeathTime();
                        auto root_time =  root->getDeathTime();
                        auto curtime = cur->getDeathTime();
                        if (itwhere != whereis.end()) {
                            // So we visit 'cur' but it has been put into whereis.
                            // We will be using the root that died LATER.
                            if (other_root != root) {
                                // DEBUG cout << "WARNING: Multiple keys[ " << other_root->getType()
                                //            << " - " << root->getType() << " ]" << endl;
                                Object *older_ptr, *newer_ptr;
                                unsigned int older_time, newer_time;
                                // TODO TODO TODO 
                                //    if (root_time < other_time) {
                                if (root_time > other_time) {
                                    older_ptr = root;
                                    older_time = root_time;
                                    newer_ptr = other_root;
                                    newer_time = other_time;
                                } else {
                                    older_ptr = other_root;
                                    older_time = other_time;
                                    newer_ptr = root;
                                    newer_time = root_time;
                                }
                                if (curtime <= newer_time) {
                                    if (cur) {
                                        keyset[newer_ptr]->insert(cur);
                                        whereis[cur] = older_ptr;
                                    }
                                } else {
                                    // Else it belongs to the root that died later.
                                    if (cur) {
                                        keyset[older_ptr]->insert(cur);
                                        whereis[cur] = older_ptr;
                                    }
                                }
                            } // else {
                                // No need to do anything since other_root is the SAME as root
                            // }
                        } else {
                            if (cur) {
                                keyset[root]->insert(cur);
                                whereis[cur] = root;
                            }
                        }
                    }
                    for ( EdgeMap::iterator p = cur->getEdgeMapBegin();
                          p != cur->getEdgeMapEnd();
                          ++p ) {
                        Edge* target_edge = p->second;
                        if (target_edge) {
                            Object *tgtObj = target_edge->getTarget();
                            work.push_back(tgtObj);
                        }
                    }
                } // if (itdisc == discovered.end())
            } // while (!work.empty())
        } // if (it != candSet.end())
    }
    cout << "After  whereis size: " << whereis.size() << endl;
    cout << endl;
    // cout << "  MISSES: " << miss_total << "   HITS: " << hit_total << endl;
}


void HeapState::save_output_edge( Edge *edge,
                                  EdgeState estate )
{
    // save_output_edge can only be called with the *NOT_SAVED edge states
    assert( (estate == EdgeState::DEAD_BY_OBJECT_DEATH_NOT_SAVED) ||
            (estate == EdgeState::DEAD_BY_PROGRAM_END_NOT_SAVED) );
    VTime_t ctime = edge->getCreateTime();
    std::pair<Edge *, VTime_t> epair = std::make_pair(edge, ctime);
    auto iter = this->m_estate_map.find(epair);
    if (iter == this->m_estate_map.end()) {
        // Not found in the edgestate_map
        // Save the edge.
        this->m_estate_map[epair] = estate;
    } else {
        // Found in the edgestate_map. This may be a problem.
        if (this->m_estate_map[epair] != estate) {
            cerr << "ERROR src[" << edge->getSource()->getId() << "] tgt["
                  << edge->getTarget()->getId()  << "]: Mismatch in edgestate in map[ "
                 << static_cast<int>(this->m_estate_map[epair]) << " ] vs [ "
                 << static_cast<int>(estate) << " ]" << endl;
            this->m_estate_map[epair] = estate;
        }
                 
    }
    // NEED TODO:
    // * output edges can now simply output these edges
}

// -- Return a string with some information
string Object::info() {
    stringstream ss;
    ss << "OBJ 0x"
       << hex
       << m_id
       << dec
       << "("
       << m_type << " "
       << (m_site != 0 ? m_site->info() : "<NONE>")
       << " @"
       << m_createTime
       << ")";
    return ss.str();
}

string Object::info2() {
    stringstream ss;
    ss << "OBJ 0x"
       << hex
       << m_id
       << dec
       << "("
       << m_type << " "
       << (m_site != 0 ? m_site->info() : "<NONE>")
       << " @"
       << m_createTime
       << ")"
       << " : " << (m_deathTime - m_createTime);
    return ss.str();
}

// Save the edge
void Object::updateField( Edge *edge,
                          FieldId_t fieldId,
                          unsigned int cur_time,
                          Method *method,
                          Reason reason,
                          Object *death_root,
                          LastEvent last_event,
                          EdgeState estate,
                          ofstream &eifile )

{
   this->__updateField( edge,
                        fieldId,
                        cur_time,
                        method,
                        reason,
                        death_root,
                        last_event,
                        estate,
                        &eifile,
                        true );
}

void Object::updateField_nosave( Edge *edge,
                                 FieldId_t fieldId,
                                 unsigned int cur_time,
                                 Method *method,
                                 Reason reason,
                                 Object *death_root,
                                 LastEvent last_event,
                                 EdgeState estate )
{
   this->__updateField( edge,
                        fieldId,
                        cur_time,
                        method,
                        reason,
                        death_root,
                        last_event,
                        estate,
                        NULL,
                        false );
}

// Doesn't output to the edgefile.
void Object::__updateField( Edge *edge,
                            FieldId_t fieldId,
                            unsigned int cur_time,
                            Method *method,
                            Reason reason,
                            Object *death_root,
                            LastEvent last_event,
                            EdgeState estate,
                            ofstream *eifile_ptr,
                            bool save_edge_flag )
{
    if (save_edge_flag) {
        assert(eifile_ptr);
    }
    if (edge) {
        edge->setEdgeState( EdgeState::LIVE );
    }
    auto p = this->m_fields.find(fieldId);
    if (p != this->m_fields.end()) {
        // -- Old edge
        auto old_edge = p->second;
        if (old_edge) {
            if (old_edge->getEdgeState() == EdgeState::LIVE) {
                // We only do the following if the edge is still alive.
                // If we need to modify the EndTime/DeathTime or EdgeState,
                // we'll do it somewhere else.
                old_edge->setEdgeState( estate );
                old_edge->setEndTime(cur_time);
                if (save_edge_flag) {
                    output_edge( old_edge,
                                 cur_time,
                                 estate,
                                 *eifile_ptr );
                }
            }
            // -- Now we know the end time
            auto old_target = old_edge->getTarget(); // pointer to old target object
            if (old_target) {
                if (reason == Reason::HEAP) {
                    old_target->setHeapReason( cur_time );
                } else if (reason == Reason::STACK) {
                    old_target->setStackReason( cur_time );
                } else {
                    cerr << "Invalid reason." << endl;
                    assert( false );
                }
                old_target->decrementRefCountReal( cur_time,
                                                   method,
                                                   reason,
                                                   death_root,
                                                   last_event,
                                                   eifile_ptr );
            }
        }
    }
    // -- Do store
    this->m_fields[fieldId] = edge;

    Object *target = NULL;
    if (edge) {
        target = edge->getTarget();
        if (target) {
            // -- Increment new ref
            target->incrementRefCountReal();
            // TODO: An increment of the refcount means this isn't a candidate root
            //       for a garbage cycle.
        }
    }

    if (HeapState::debug) {
        cout << "Update "
             << m_id << "." << fieldId
             << " --> " << (target ? target->m_id : 0)
             << " (" << (target ? target->getRefCount() : 0) << ")"
             << endl;
    }
}

void Object::mark_red()
{
    if ( (this->m_color == Color::GREEN) || (this->m_color == Color::BLACK) ) {
        // Only recolor if object is GREEN or BLACK.
        // Ignore if already RED or BLUE.
        this->recolor( Color::RED );
        for ( EdgeMap::iterator p = this->m_fields.begin();
              p != this->m_fields.end();
              p++ ) {
            Edge* edge = p->second;
            if (edge) {
                Object* target = edge->getTarget();
                target->mark_red();
            }
        }
    }
}

void Object::scan()
{
    if (this->m_color == Color::RED) {
        if (this->m_refCount > 0) {
            this->scan_green();
        } else {
            this->recolor( Color::BLUE );
            // -- Visit all edges
            for ( EdgeMap::iterator p = this->m_fields.begin();
                  p != this->m_fields.end();
                  p++ ) {
                Edge* target_edge = p->second;
                if (target_edge) {
                    Object* next_target_object = target_edge->getTarget();
                    if (next_target_object) {
                        next_target_object->scan();
                    }
                }
            }
        }
    }
}

void Object::scan_green()
{
    this->recolor( Color::GREEN );
    for ( EdgeMap::iterator p = this->m_fields.begin();
          p != this->m_fields.end();
          p++ ) {
        Edge* target_edge = p->second;
        if (target_edge) {
            Object* next_target_object = target_edge->getTarget();
            if (next_target_object) {
                if (next_target_object->getColor() != Color::GREEN) {
                    next_target_object->scan_green();
                }
            }
        }
    }
}

deque<int> Object::collect_blue( EdgeList& edgelist )
{
    deque<int> result;
    if (this->getColor() == Color::BLUE) {
        this->recolor( Color::GREEN );
        result.push_back( this->getId() );
        for ( EdgeMap::iterator p = this->m_fields.begin();
              p != this->m_fields.end();
              p++ ) {
            Edge* target_edge = p->second;
            if (target_edge) {
                Object* next_target_object = target_edge->getTarget();
                if (next_target_object) {
                    deque<int> new_result = next_target_object->collect_blue(edgelist);
                    if (new_result.size() > 0) {
                        for_each( new_result.begin(),
                                  new_result.end(),
                                  [&result] (int& n) { result.push_back(n); } );
                    }
                    pair<int,int> newedge(this->getId(), next_target_object->getId());
                    edgelist.push_back( newedge );
                    // NOTE: this may add an edge that isn't in the cyclic garbage.
                    // These invalid edges will be filtered out later when
                    // we know for sure what the cyclic component is.
                }
            }
        }
    }
    return result;
}

void Object::makeDead( unsigned int death_time,
                       unsigned int death_time_alloc,
                       EdgeState estate,
                       ofstream &edge_info_file,
                       Reason newreason )
{
    this->__makeDead( death_time,
                      death_time_alloc,
                      estate,
                      &edge_info_file,
                      newreason,
                      true );
}

void Object::makeDead_nosave( unsigned int death_time,
                              unsigned int death_time_alloc,
                              EdgeState estate,
                              ofstream &edge_info_file,
                              Reason newreason )
{
    this->__makeDead( death_time,
                      death_time_alloc,
                      estate,
                      NULL,
                      newreason,
                      false );
}

void Object::__makeDead( unsigned int death_time,
                         unsigned int death_time_alloc,
                         EdgeState estate,
                         ofstream *edge_info_file_ptr,
                         Reason newreason,
                         bool save_edge_flag )
{
    // -- Record the death time
    this->m_deathTime = death_time;
    this->m_deathTime_alloc = death_time_alloc;
    // -- Record the refcount at death
    this->m_refCount_at_death = this->getRefCount();
    // -- Set up reasons
    this->setReason( newreason, death_time );
    if (this->m_deadFlag) {
        cerr << "Object[ " << this->getId() << " ] : double Death event." << endl;
    } else {
        this->m_deadFlag = true;
    }

    // Set the DiedBy******* status
    if (newreason == Reason::STACK) {
        this->setDiedByStackFlag();
    } else if (newreason == Reason::HEAP) {
        this->setDiedByHeapFlag();
    } else if (newreason == Reason::GLOBAL) {
        this->setDiedByGlobalFlag();
    } else if (newreason == Reason::END_OF_PROGRAM_REASON) {
        this->setDiedAtEndFlag();
    } else if (newreason == Reason::UNKNOWN_REASON) {
        this->setDiedByHeapFlag();
    }
    // -- Visit all edges
    for ( EdgeMap::iterator p = this->m_fields.begin();
          p != this->m_fields.end();
          p++ ) {
        Edge* edge = p->second;

        if ( edge &&
             (edge->getEdgeState() == EdgeState::LIVE) ) {
            // -- Edge dies now
            edge->setEdgeState( estate );
            edge->setEndTime( death_time );
            if ((estate == EdgeState::DEAD_BY_OBJECT_DEATH_NOT_SAVED) ||
                (estate == EdgeState::DEAD_BY_PROGRAM_END_NOT_SAVED)) {
                this->m_heapptr->save_output_edge( edge,
                                                   estate );
            } else {
                output_edge( edge,
                             death_time,
                             estate,
                             *edge_info_file_ptr );
            }
        }
    }

    if (HeapState::debug) {
        cout << "Dead object " << m_id << " of type " << m_type << endl;
    }
}

void Object::recolor( Color newColor )
{
    // Maintain the invariant that the reference count of a node is
    // the number of GREEN or BLACK pointers to it.
    for ( EdgeMap::iterator p = this->m_fields.begin();
          p != this->m_fields.end();
          p++ ) {
        Edge* edge = p->second;
        if (edge) {
            Object* target = edge->getTarget();
            if (target) {
                if ( ((this->m_color == Color::GREEN) || (this->m_color == Color::BLACK)) &&
                     ((newColor != Color::GREEN) && (newColor != Color::BLACK)) ) {
                    // decrement reference count of target
                    target->decrementRefCount();
                } else if ( ((this->m_color != Color::GREEN) && (this->m_color != Color::BLACK)) &&
                            ((newColor == Color::GREEN) || (newColor == Color::BLACK)) ) {
                    // increment reference count of target
                    target->incrementRefCountReal();
                }
            }
        }
    }
    this->m_color = newColor;
}

void Object::decrementRefCountReal( unsigned int cur_time,
                                    Method *method,
                                    Reason reason,
                                    Object *death_root,
                                    LastEvent lastevent,
                                    ofstream *eifile_ptr )
{
    this->decrementRefCount();
    this->m_lastMethodDecRC = method;
    // NOW: Our reason is clearly because of the DECRC.
    this->setLastEvent( lastevent );
    if (this->m_refCount == 0) {
        // TODO A: ObjectPtrMap_t& whereis = this->m_heapptr->get_whereis();
        // TODO A: KeySet_t &keyset = this->m_heapptr->get_keyset();
        // TODO Should we even bother with this check?
        //      Maybe just set it to true.
        if (!m_decToZero) {
            m_decToZero = true;
            m_methodRCtoZero = method;
            this->g_counter++;
        }
        if (this->wasRoot() || (reason == Reason::STACK)) {
            this->setDiedByStackFlag();
        } else {
            this->setDiedByHeapFlag();
        }
        // TODO B: // -- Visit all edges
        // TODO B: this->recolor(Color::GREEN);

        // -- Who's my key object?
        if (death_root != NULL) {
            this->setDeathRoot( death_root );
        } else {
            this->setDeathRoot( this );
        }
        // TODO A: Object *my_death_root = this->getDeathRoot();
        // TODO A: unsigned int drootId = my_death_root->getId();
        // TODO A: assert(my_death_root);
        // TODO A: whereis[this] = my_death_root;

        // TODO A: auto itset = keyset.find(my_death_root);
        // TODO A: if (itset == keyset.end()) {
        // TODO A:     keyset[my_death_root] = new std::set< Object * >();
        // TODO A:     keyset[my_death_root]->insert( my_death_root );
        // TODO A: }
        // TODO A: keyset[my_death_root]->insert( this );

        LastEvent newevent;
        // Set key type based on last event
        if ( (lastevent == LastEvent::UPDATE_AWAY_TO_NULL) ||
             (lastevent == LastEvent::UPDATE_AWAY_TO_VALID) ||
             (lastevent == LastEvent::UPDATE_UNKNOWN) ||
             (lastevent == LastEvent::ROOT) ) {
            // This is a DAGKEY
            this->setKeyType(KeyType::DAGKEY);
            newevent = ((lastevent == LastEvent::ROOT) ? LastEvent::OBJECT_DEATH_AFTER_ROOT_DECRC
                                                       : LastEvent::OBJECT_DEATH_AFTER_UPDATE_DECRC);
        } else if ( (lastevent == LastEvent::DECRC) ||
                    is_object_death(lastevent) ||
                    (lastevent == LastEvent::END_OF_PROGRAM_EVENT) ||
                    (lastevent == LastEvent::UNKNOWN_EVENT) ) {
            // This is a DAG
            this->setKeyType(KeyType::DAG);
            newevent = lastevent;
        } else {
            // This isn't possible.
            cerr << "DEBUG: DECRC ERROR. Continuing." << endl;
            this->setKeyType(KeyType::DAG);
            newevent = lastevent;
        }
        // Edges are now dead.
        for ( auto p = this->m_fields.begin();
              p != this->m_fields.end();
              ++p ) {
            auto target_edge = (Edge *) p->second;
            if (target_edge) {
                // TODO unsigned int fieldId = target_edge->getSourceField();
                auto target_obj = target_edge->getTarget();
                if (target_obj) {
                    target_obj->decrementRefCountReal( cur_time,
                                                       method,
                                                       reason,
                                                       death_root,
                                                       lastevent,
                                                       eifile_ptr );
                }
            }
        }
        // DEBUG
        // if (Object::g_counter % 1000 == 1) {
        // cout << ".";
        // }
    } // TODO B: else {
    // TODO B: auto color = this->getColor();
    // TODO B: if (color != Color::BLACK) {
    // TODO B:     auto objId = this->getId();
    // TODO B:     this->recolor( Color::BLACK );
    // TODO B:     this->m_heapptr->set_candidate(objId);
    // TODO B: }
    // TODO B: }
}

void Object::incrementRefCountReal()
{
    if ((this->m_refCount == 0) && this->m_decToZero) {
        this->m_incFromZero = true;
        this->m_methodRCtoZero = NULL;
    }
    this->incrementRefCount();
    this->m_maxRefCount = std::max( (m_refCount > 0 ? (unsigned int) m_refCount
                                                    : 0),
                                    m_maxRefCount );
    // Take out of candidate set
    this->m_heapptr->unset_candidate( this->m_id );
    // TODO
    // {
    //     Color color = this->getColor();
    //     if (color != Color::BLACK) {
    //         unsigned int objId = this->getId();
    //         this->recolor(Color::BLACK);
    //         this->m_heapptr->set_candidate(objId);
    //     }
    // }
}

void Object::setDeathTime( VTime_t new_deathtime )
{
    this->m_deathTime = new_deathtime;
}
