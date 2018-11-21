#ifndef REFSTATE_H
#define REFSTATE_H

// ----------------------------------------------------------------------
//  Representation of object's in-reference state

// TODO: Not sure if we need the classes in classinfo
//       * Maybe Method?
//       - RLV - 2016-0114
// #include "classinfo.hpp"

#include <iostream>

// using namespace std;

// ----------------------------------------------------------------------
//   Ref state

enum class RefState {
    Start,
    Heap,
    HeapRoot,
    HeapRootFinal,
    HeapOnly,
    Root,
    RootOnly,
    Error = 999
};

class ObjectRefState
{
    // See diagram:
    // function Update --> On event, change state to new state.
    // function getstate
    // --? set state?
public:

    ObjectRefState()
        : m_state(RefState::Start) {
    }

    RefState getState() { return this->m_state; }

private:
    RefState m_state;
};

#endif
