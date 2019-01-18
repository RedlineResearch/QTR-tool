#include "heap.hpp"

// -- Global flags
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


// =================================================================

Object* HeapState::allocate( unsigned int id,
                             unsigned int size,
                             char kind,
                             string type,
                             AllocSite *site, 
                             string &nonjavalib_site_name,
                             unsigned int els,
                             Thread *thread,
                             bool new_flag,
                             unsigned int create_time )
{
    // Design decision: allocation time isn't 0 based.
    this->m_alloc_time += size;
    Object *obj = NULL;
    obj = new Object( id,
                      size,
                      kind,
                      type,
                      site,
                      nonjavalib_site_name,
                      els,
                      thread,
                      create_time,
                      new_flag,
                      this );
    // Add to object map
    this->m_objects[obj->getId()] = obj;
    // Add to live set. Given that it's a set, duplicates are not a problem.
    this->m_liveset.insert(obj);

    unsigned long int temp = this->m_liveSize + obj->getSize();
    // Max live size calculation
    this->m_liveSize = ( (temp < this->m_liveSize) ? ULONG_MAX : temp );
    if (this->m_maxLiveSize < this->m_liveSize) {
        this->m_maxLiveSize = this->m_liveSize;
    }
    // cerr << "ALLOC: " << id << " - " << obj << endl;
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
    return ((p != m_objects.end()) ? (*p).second
                                   : NULL);
}

Edge * HeapState::make_edge( Object *source,
                             FieldId_t field_id,
                             Object *target,
                             unsigned int cur_time )
{
    // cerr << "EDGE: [" << source << "]< " << source->getId() << " > ::"
    //      << field_id << " -> " << target << " -- at " << cur_time << endl;
    Edge *new_edge = new Edge( source, field_id,
                               target, cur_time );
    // TODO target->setPointedAtByHeap();
    return new_edge;
}

Edge *
HeapState::make_edge( Edge *edgeptr )
{
    // Edge* new_edge = new Edge( source, field_id,
    //                            target, cur_time );
    Object *target = edgeptr->getTarget();
    assert(target != NULL);
    // TODO target->setPointedAtByHeap();

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
                       *eifile_ptr,
                       objreason );
    }
}

// TODO Documentation :)
void HeapState::update_death_counters( Object *obj )
{
    unsigned int obj_size = obj->getSize();
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
    
}

Method * HeapState::get_method_death_site( Object *obj )
{
    return obj->getDeathSite();
    /*
     * TODO: This not needed? TODO RLV - 2018-1212
    if (obj->getDiedByHeapFlag()) {
        // DIED BY HEAP
        if (!dsite) {
            if (obj->wasDecrementedToZero()) {
                // So died by heap but no saved death site. First alternative is
                // to look for the a site that decremented to 0.
                dsite = obj->getMethodDecToZero();
            } else {
                // TODO: No dsite here yet
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
    */
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
                                   *eifile_ptr,
                                   Reason::END_OF_PROGRAM_REASON );
                } else {
                    obj->makeDead_nosave( cur_time,
                                          this->m_alloc_time,
                                          EdgeState::DEAD_BY_PROGRAM_END,
                                          *eifile_ptr,
                                          Reason::END_OF_PROGRAM_REASON );
                }
                obj->setLastTimestamp( cur_time );
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
// ET2 version:
void Object::updateField( Edge *edge, FieldId_t fieldId )
{
    Edge *old_edge = this->m_fields[fieldId];
    if (old_edge) {
        delete old_edge;
    }
    this->m_fields[fieldId] = edge;
}

void Object::makeDead( unsigned int death_time,
                       unsigned int death_time_alloc,
                       ofstream &edge_info_file,
                       Reason newreason )
{
    this->__makeDead( death_time,
                      death_time_alloc,
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
                      NULL,
                      newreason,
                      false );
}

void Object::__makeDead( unsigned int death_time,
                         unsigned int death_time_alloc,
                         ofstream *edge_info_file_ptr,
                         Reason newreason,
                         bool save_edge_flag )
{
    // -- Record the death time
    this->m_deathTime = death_time;
    this->m_deathTime_alloc = death_time_alloc;
    // -- Record the refcount at death
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

    if (HeapState::debug) {
        cout << "Dead object " << m_id << " of type " << m_type << endl;
    }
}


void Object::setDeathTime( VTime_t new_deathtime )
{
    this->m_deathTime = new_deathtime;
}


Edge * Object::getEdge(FieldId_t fieldId) const
{
    auto iter = m_fields.find(fieldId);
    return (iter != m_fields.end() ? iter->second : NULL);
}
