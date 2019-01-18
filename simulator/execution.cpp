// ----------------------------------------------------------------------
//  Representation of runtime execution
//  stacks, threads, time

#include "execution.hpp"

unsigned int CCNode::m_ccnode_nextid = 1;

// ----------------------------------------------------------------------
//   Calling context tree

CCNode* CCNode::Call(Method* m)
{
    CCNode* result = 0;
    auto p = m_callees.find(m->getId());
    // TODO auto p = m_callees.find(m);
    if (p == m_callees.end()) {
        result = new CCNode( this, // parent
                             m );    // method
        m_callees[m->getId()] = result;
        // TODO m_callees[m] = result;
    } else {
        result = (*p).second;
    }
    NodeId_t parent = this->get_node_id();
    NodeId_t child = result->get_node_id();
    return result;
}

CCNode* CCNode::Return(Method* m)
{
    if (m_method != m) {
        cout << "WEIRD: Returning from the wrong method " << m->info() << endl;
        cout << "WEIRD:    should be " << m_method->info() << endl;
    }
    return m_parent;
}

string CCNode::info()
{
    stringstream ss;
    if (m_method) {
        ss << m_method->info(); //  << "@" << m_startTime;
    }
    else {
        ss << "ROOT";
    }
    return ss.str();
}

string CCNode::stacktrace()
{
    stringstream ss;
    CCNode* cur = this;
    while (cur) {
        ss << cur->info() << endl;
        cur = cur->getParent();
    }
    return ss.str();
}

DequeId_t CCNode::stacktrace_using_id()
{
    DequeId_t result;
    CCNode* cur = this;
    while (cur) {
        result.push_front(this->getMethodId());
        cur = cur->getParent();
    }
    return  result;
}

// A version that returns a list/vector of
// - method pointers
deque<Method *> CCNode::simple_stacktrace()
{
    if (this->isDone()) {
        return this->m_simptrace;
    }
    deque<Method *> result;
    // Relies on the fact that Method * are unique even
    // if the CCNodes are different as long as they refer
    // to the same method. A different CCNode will mean
    // a different invocation.
    Method *mptr = this->getMethod();
    assert(mptr);
    CCNode *parent_ptr = this->getParent();
    if (parent_ptr) {
        result = parent_ptr->simple_stacktrace();
    }
    result.push_front(mptr);
    this->setDone();
    this->m_simptrace = result;
    return result;
}

bool CCNode::simple_cc_equal( CCNode &other )
{
    // Relies on the fact that Method * are unique even
    // if the CCNodes are different as long as they refer
    // to the same method. A different CCNode will mean
    // a different invocation.
    if (this->getMethod() != other.getMethod()) {
        return false;
    }
    CCNode *self_ptr = this->getParent();
    CCNode *other_ptr = other.getParent();
    if ((self_ptr == NULL || other_ptr == NULL)) {
        return (self_ptr == other_ptr);
    }
    return (self_ptr->simple_cc_equal(*other_ptr));
}


// ----------------------------------------------------------------------
//   Thread representation
//   (Essentially, the stack)

// -- Call method m
void Thread::Call(Method *m)
{
    if (this->m_kind == ExecMode::Full) {
        CCNode* cur = this->TopCC();
        this->m_curcc = cur->Call(m);
        // this->nodefile << child << "," << m->getName() << endl;
    }

    if (this->m_kind == ExecMode::StackOnly) {
        // Save (old_top, new_top) of m_methods
        // NOT SURE IF THIS IS STILL NEEDED:
        ContextPair cpair;
        if (m_methods.size() > 0) {
            cpair = std::make_pair( m_methods.back(), m );
        } else {
            cpair = std::make_pair( (Method *) NULL, m );
        }
        this->setContextPair( cpair, CPairType::CP_Call );
        // TODO this->debug_cpair( this->getContextPair(), "call" );
        // END NOT NEEDED NOTE
        // -------------------
        m_methods.push_back(m);
        // m_methods, m_locals, and m_deadlocals must be synched in pushing
        // TODO: Do we need to check for m existing in map?
        // Ideally no, but not really sure what is possible in Elephant 
        // Tracks.
        m_locals.push_back(new LocalVarSet());
        m_deadlocals.push_back(new LocalVarSet());
        // Count 2 level contexts
        MethodDeque top2meth = this->top_N_methods(2);
        this->m_exec.IncCallContext( top2meth );
    }
}

// -- Return from method m
void Thread::Return(Method* m)
{
    if (this->m_kind == ExecMode::Full) {
        CCNode* cur = this->TopCC();
        if (cur->getMethod()) {
            this->m_curcc = cur->Return(m);
        } else {
            cout << "WARNING: Return from " << m->info() << " at top context" << endl;
            m_curcc = cur;
        }
    }

    if (this->m_kind == ExecMode::StackOnly) {
        if ( !m_methods.empty()) {
            Method *cur = m_methods.back();
            m_methods.pop_back();
            // if (cur != m) {
            //     cerr << "WARNING: Return from method " << m->info() << " does not match stack top " << cur->info() << endl;
            // }
            // m_methods, m_locals, and m_deadlocals must be synched in popping

            // NOTE: Maybe refactor. See same code in Thread::Call
            // Save (old_top, new_top) of m_methods
            ContextPair cpair;
            if (m_methods.size() > 0) {
                // TODO: What if m != cur?
                // It seems reasonable to simply use the m that's passed to us rather than
                // rely on the call stack being correct. TODO: Verify.
                cpair = std::make_pair( cur, m_methods.back() );
            } else {
                cpair = std::make_pair( cur, (Method *) NULL );
            }
            this->setContextPair( cpair, CPairType::CP_Return );
            // TODO this->debug_cpair( this->getContextPair(), "return" );
            // TODO TODO: Save type (Call vs Return) -- See similar code above.
            // Locals
            LocalVarSet *localvars = m_locals.back();
            m_locals.pop_back();
            LocalVarSet *deadvars = m_deadlocals.back();
            m_deadlocals.pop_back();
            delete localvars;
            delete deadvars;
        } else {
            cout << "ERROR: Stack empty at return " << m->info() << endl;
        }
    }
}

// -- Get current CC
CCNode* Thread::TopCC()
{
    if (this->m_kind == ExecMode::Full) {
        assert(m_curcc);
        // TODO // -- Create a root context if necessary
        // TODO if (m_curcc == 0) {
        // TODO     m_curcc = new CCNode();
        // TODO }
        return m_curcc;
    }

    if (this->m_kind == ExecMode::StackOnly) {
        cout << "ERROR: Asking for calling context in stack mode" << endl;
        return 0;
    }

    cout << "ERROR: Unkown mode " << this->m_kind << endl;
    return 0;
}

// -- Get current method
Method *Thread::TopMethod()
{
    if (this->m_kind == ExecMode::Full) {
        return TopCC()->getMethod();
    }

    if (this->m_kind == ExecMode::StackOnly) {
        if ( ! m_methods.empty()) {
            return m_methods.back();
        } else {
            // cout << "ERROR: Asking for top of empty stack" << endl;
            return NULL;
        }
    }

    cout << "ERROR: Unkown mode " << m_kind << endl;
    return NULL;
}

// -- Get method 'num' from the top (zero-based)
Method *Thread::TopMethod( unsigned int num )
{
    if (this->m_kind == ExecMode::Full) {
        assert(false);
        return TopCC()->getMethod();
    }

    if (this->m_kind == ExecMode::StackOnly) {
        unsigned int mysize = this->m_methods.size();
        if (mysize > num) {
            // return this->m_methods[num + 1];
            return this->m_methods[mysize - num - 1];
        } else {
            // cout << "ERROR: Asking for " << num
            //      << "-th from top but stack is smaller." << endl;
            return NULL;
        }
    }

    cout << "ERROR: Unkown mode " << m_kind << endl;
    return NULL;
}

// -- Get N current methods. Only makes sense if N > 1.
//    For N == 1, use TopMethod()
MethodDeque Thread::top_N_methods(unsigned int N)
{
    assert( N > 1 );
    MethodDeque result;
    if (this->m_kind == ExecMode::Full) {
        // TODO: NOT IMPLEMENTED YET FOR FULL MODE
        assert(false);
        // TODO DELETE return TopCC()->getMethod();
    }
    else if (this->m_kind == ExecMode::StackOnly) {
        unsigned int count = 0;
        unsigned int stacksize = this->m_methods.size();
        while ((count < N) && count < stacksize) {
            result.push_back(this->TopMethod(count));
            count++;
        }
        while (count < N ) {
            result.push_back(NULL);
            count++;
        }
    } else {
        cout << "ERROR: Unkown mode " << m_kind << endl;
        assert(result.size() == 0);
    }
    return result;
    // NOTHING GOES HERE.
}

// -- Get full context of methods.
MethodDeque Thread::full_method_stack()
{
    MethodDeque result;
    if (this->m_kind == ExecMode::Full) {
        // TODO: NOT IMPLEMENTED YET FOR FULL MODE
        //       Maybe should be called at all from FULL mode.
        assert(false);
    } else if (this->m_kind == ExecMode::StackOnly) {
        unsigned int count = 0;
        unsigned int stacksize = this->m_methods.size();
        while (count < stacksize) {
            result.push_back(this->TopMethod(count));
            count++;
        }
    } else {
        cout << "ERROR: Unkown mode " << m_kind << endl;
        assert(result.size() == 0);
    }
    return result;
    // NOTHING GOES HERE.
}

//-----------------------------------------------------------------------------
// Private helper function for determining whether
// a function is a Java library.
// If the function name starts with:
// -   'java/'
// -   'sun/'
// -   'com/sun/'
// -   'com/ibm/'
// then it's a Java library function.
bool is_javalib_method( string &methname )
{
    return  ( (methname.substr(0,5) == "java/") ||
              (methname.substr(0,4) == "sun/") ||
              (methname.substr(0,8) == "com/sun/") ||
              (methname.substr(0,8) == "com/ibm/") );
}
//-----------------------------------------------------------------------------

// -- Get all Java library methods from top of call stack +
//    first non Java method
MethodDeque Thread::top_javalib_methods()
{
    MethodDeque result;
    if (this->m_kind == ExecMode::Full) {
        // TODO: NOT IMPLEMENTED YET FOR FULL MODE
        assert(false);
    } else if (this->m_kind == ExecMode::StackOnly) {
        // TODO Some testing code. Should move these out into a unit testing
        //      framework.
        // string test1("java/lang/Shutdown.shutdown");
        // string test2("SHOULD FAIL");
        // string test3("");
        // string test4("sun/some/method");
        // string test5("com/sun/some/method");
        // string test6("com/ibm/some/method");
        // assert( is_javalib_method(test1) ); 
        // assert( !is_javalib_method(test2) ); 
        // assert( !is_javalib_method(test3) ); 
        // assert( is_javalib_method(test4) ); 
        // assert( is_javalib_method(test5) ); 
        // assert( is_javalib_method(test6) ); 
        unsigned int count = 0;
        unsigned int stacksize = this->m_methods.size();
        while (count < stacksize) {
            Method *mptr = this->TopMethod(count);
            string mname = (mptr ? mptr->getName() : "NULL_METHOD");
            result.push_back(mptr);
            count++;
            if (!is_javalib_method(mname)) {
                // This check and break statement is placed after
                // the push onto the result deque. This means that
                // at least one non Java library method may be included
                // in the result.
                break;
            }
        }
        if (result.size() == 0) {
            result.push_back(NULL);
        }
    } else {
        cout << "ERROR: Unkown mode " << m_kind << endl;
        assert(result.size() == 0);
    }
    return result;
    // NOTHING GOES HERE.
}

// -- Get current dead locals
LocalVarSet * Thread::TopLocalVarSet()
{
    if (this->m_kind == ExecMode::Full) {
        // TODO
        return NULL;
    }
    else if (this->m_kind == ExecMode::StackOnly) {
        return ((!m_deadlocals.empty()) ? m_deadlocals.back() : NULL);
    }
}

// -- Get a stack trace
string Thread::stacktrace()
{
    if (this->m_kind == ExecMode::Full) {
        return TopCC()->stacktrace();
    }

    if (this->m_kind == ExecMode::StackOnly) {
        if ( ! m_methods.empty()) {
            stringstream ss;
            for ( auto p = m_methods.begin();
                  p != m_methods.end();
                  p++ ) {
                Method* m =*p;
                ss << m->info() << endl;
            }
            return ss.str();
        } else {
            return "<empty>";
        }
    }

    cout << "ERROR: Unkown mode " << m_kind << endl;
    return "ERROR";
}

// -- Get a stack trace in method ids
DequeId_t Thread::stacktrace_using_id()
{
    if (this->m_kind == ExecMode::Full) {
        return TopCC()->stacktrace_using_id();
    }

    if (this->m_kind == ExecMode::StackOnly) {
        assert(false);
        // TODO  if ( ! m_methods.empty()) {
        // TODO      stringstream ss;
        // TODO      for ( auto p = m_methods.begin();
        // TODO            p != m_methods.end();
        // TODO            p++ ) {
        // TODO          Method* m =*p;
        // TODO          ss << m->info() << endl;
        // TODO      }
        // TODO      return ss.str();
        // TODO  } else {
        // TODO      return "<empty>";
        // TODO  }
        DequeId_t result;
        return result;
    }
    assert(false);
}

// -- Object is a root
void Thread::objectRoot(Object *object)
{
    if (!m_locals.empty()) {
        LocalVarSet *localvars = m_locals.back();
        localvars->insert(object);
    } else {
        cout << "[objectRoot] ERROR: Stack empty at ROOT event." << endl;
    }
}

// -- Check dead object if root
bool Thread::isLocalVariable(Object *object)
{
    if (!m_locals.empty()) {
        LocalVarSet *localvars = m_locals.back();
        auto it = localvars->find(object);
        return (it != localvars->end());
    } else {
        cout << "[isLocalVariable] ERROR: Stack empty at ROOT event." << endl;
        return false;
    }
}

// ----------------------------------------------------------------------
//   Execution state

// -- Look up or create a thread
Thread* ExecState::getThread(unsigned int threadid)
{
    Thread *result = NULL;
    auto p = m_threads.find(threadid);
    if (p == m_threads.end()) {
        // -- Not there, make a new one
        result = new Thread( threadid,
                             this->m_kind,
                             this->m_allocCountmap,
                             this->m_deathPairCountMap,
                             *this );
        assert(result);
        m_threads[threadid] = result;
    } else {
        result = (*p).second;
    }

    return result;
}

// -- Call method m in thread t
void ExecState::Call(Method* m, unsigned int threadid)
{
    this->IncMethodTime();
    Thread *t = getThread(threadid);
    if (t) {
        t->Call(m);
        this->m_thread_stack.push_back(t);
    }
}

// -- Return from method m in thread t
void ExecState::Return(Method* m, unsigned int threadid)
{
    this->IncMethodExitTime();
    if (this->m_thread_stack.size() > 0) {
        this->m_thread_stack.pop_back();
    }
    getThread(threadid)->Return(m);
}

// -- Get the top method in thread t
Method* ExecState::TopMethod(unsigned int threadid)
{
    Thread *thread = getThread(threadid);
    if (thread) {
        return thread->TopMethod();
    } else {
        return NULL;
    }
}

// -- Get the top method in thread t
Method *ExecState::TopMethod( unsigned int threadid,
                              unsigned int num )
{
    Thread *thread = getThread(threadid);
    if (thread) {
        return thread->TopMethod(num);
    } else {
        return NULL;
    }
}

// -- Get the top calling context in thread t
CCNode* ExecState::TopCC(unsigned int threadid)
{
    return getThread(threadid)->TopCC();
}

// Update the Object pointer to simple Death context pair map
void ExecState::UpdateObj2DeathContext( Object *obj,
                                        MethodDeque &methdeque )
{
    unsigned int count = 0;
    bool nonjava_flag = false;
    string nonjava_method;
    MethodDeque top_2_methods;
    while ( (count < 2) &&
            (methdeque.size() > 0) ) {
        Method *next_method = methdeque.front();
        methdeque.pop_front();
        string next_name;
        count += 1;
        next_name = (next_method ? next_method->getName() : "NULL_METHOD");
        obj->setDeathSite(next_method, count);
        obj->setDeathContextSiteName(next_name, count);
        if (count == 1) {
            this->m_objDeath2cmap[obj] = next_name;
            top_2_methods.push_back(next_method);
        } else if (count == 2) {
            top_2_methods.push_back(next_method);
        }
        if (!nonjava_flag && !is_javalib_method(next_name)) {
            obj->set_nonJavaLib_death_context(next_name);
            nonjava_flag = true;
            break;
        }
    }
    if (!nonjava_flag && (count < 2)) {
        while (count < 2) {
            string next_name("NULL_METHOD");
            count += 1;
            obj->setDeathSite(NULL, count);
            obj->setDeathContextSiteName(next_name, count);
            if (count == 1) {
                this->m_objDeath2cmap[obj] = next_name;
                top_2_methods.push_back(NULL);
            } else if (count == 2) {
                top_2_methods.push_back(NULL);
            }
            if (!nonjava_flag && !is_javalib_method(next_name)) {
                obj->set_nonJavaLib_death_context(next_name);
                nonjava_flag = true;
                break;
            }
        }
    }
    if (!nonjava_flag) {
        string last_name;
        if (methdeque.size() > 0) {
            Method *last_method = methdeque.back();
            last_name = ( last_method ? last_method->getName() : "NULL_METHOD" );
        } else {
            last_name = "NULL_METHOD";
        }
        obj->set_nonJavaLib_death_context(last_name);
        nonjava_flag = true; // probably don't need to set the flag, but just in
                             // case someone adds some code later on.
    }
    ContextPair cpair = std::make_pair( top_2_methods[0], top_2_methods[1] );
    auto iter = this->m_deathPairCountMap.find(cpair);
    if (iter == this->m_deathPairCountMap.end()) {
        this->m_deathPairCountMap[cpair] = 0;
    }
    this->m_deathPairCountMap[cpair]++;

}


// -
// Utility files
//
std::ostream& operator << ( std::ostream &out, const ExecMode &value )
{
    switch(value)
    {
        case ExecMode::Full:
            out << "Full";
            break;
        case ExecMode::StackOnly:
            out << "StackOnly";
            break;
        default:
            out << "<UNKNOWN>";
    }

    return out;
}

