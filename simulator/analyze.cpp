using namespace std;

#include "classinfo.hpp"
#include "execution.hpp"
#include "heap.hpp"
#include "tokenizer.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <map>
#include <set>
#include <vector>


extern HeapState Heap;
extern ExecState Exec;

class Object;
class CCNode;

// -- Union/find over objects

class DisjointSets
{
    private:
        map<Object*, Object*> m_parent;
        map<Object*, int> m_rank;
        map<Object*, int> m_set_size;

    public:
        DisjointSets() {}

        bool is_root(Object* x)
        {
            map<Object*, Object*>::iterator p = m_parent.find(x);
            if (p != m_parent.end()) {
                return p->second == x;
            }

            return false;
        }

        void make_set(Object* x)
        {
            if (m_parent.find(x) == m_parent.end()) {
                m_parent[x] = x;
                m_rank[x] = 0;
                m_set_size[x] = 1;
            }
        }

        Object* find(Object* x)
        {
            Object* p = m_parent[x];
            if (p == x) {
                return x;
            }
            else {
                Object* root = find(p);
                m_parent[x] = root;
                return root;
            }
        }

        void setunion(Object* A, Object* B)
        {
            Object* A_root = find(A);
            Object* B_root = find(B);
            if (A_root == B_root) {
                return;
            }

            // -- Tree with the smaller rank becomes the child
            int A_rank = m_rank[A_root];
            int B_rank = m_rank[B_root];
            if (A_rank < B_rank) {
                // -- A is smaller; B becomes root
                m_parent[A_root] = B_root;
                m_set_size[B_root] += m_set_size[A_root];
            } else {
                if (A_rank > B_rank) {
                    // -- B is smaller; A becomes root
                    m_parent[B_root] = A_root;
                    m_set_size[A_root] += m_set_size[B_root];
                } else {
                    // -- Tie; choose A as root
                    m_parent[B_root] = A_root;
                    m_rank[A_root]++;
                    m_set_size[A_root] += m_set_size[B_root];
                }
            }
        }

        void report()
        {
            for ( map<Object*, int>::iterator i = m_set_size.begin();
                  i != m_set_size.end();
                  ++i ) {
                pair<Object*, int> entry =*i;
                Object* obj = entry.first;
                if (is_root(obj)) {
                    cout << "Set size " << entry.second
                         << " root " << obj->info() 
                         << " lifetime " << (obj->getDeathTime() - obj->getCreateTime())
                         << endl;
                }
            }
        }

        void get_sets(map<Object*, set< Object*> > & the_sets)
        {
            for ( map<Object*, int>::iterator i = m_set_size.begin();
                  i != m_set_size.end();
                  ++i ) {
                pair<Object*, int> entry = *i;
                Object* obj = entry.first;
                Object* root = find(obj);
                the_sets[root].insert(obj);
            }
        }
};

double compute_overlap( unsigned int start1, unsigned int end1,
                        unsigned int start2, unsigned int end2 )
{
    unsigned int span_start = min(start1, start2);
    unsigned int span_end = max(end1, end2);
    unsigned int overlap_start = max(start1, start2);
    unsigned int overlap_end = min(end1, end2);

    double fspan = (double) (span_end - span_start);
    double foverlap = (double) (overlap_end - overlap_start);

    double frac = foverlap/fspan;

    return frac * 100.0;
} 

void analyze(unsigned int max_time)
{
    map<Object*, int> targetCount;

    double d_max_time = (double) max_time;
/*
    for ( EdgeSet::iterator i = Heap.begin_edges();
          i != Heap.end_edges();
          ++i ) {
        Edge* edge = *i;

        Object* source = edge->getSource();
        Object* target = edge->getTarget();

        unsigned int source_life = source->getDeathTime() - source->getCreateTime();
        unsigned int target_life = target->getDeathTime() - target->getCreateTime();

        double d_source = (((double) source_life)/d_max_time) * 100.0;
        double d_target = (((double) target_life)/d_max_time) * 100.0;
        double ratio_1 = d_source / d_target;
        double ratio_2 = d_target / d_source;

        // cout << d_source << "   " << d_target << "   s/t " << ratio_1 << "   t/s " << ratio_2 << endl;
        if (ratio_1 > 100.0) {
            targetCount[source]++;
        }
    }

    for ( map<Object*, int>::iterator j = targetCount.begin();
          j != targetCount.end();
          j++ ) {
        Object* src = j->first;
        int count = j->second;
        cout << "Source with " << count << " short-lived targets -- " << src->info() << endl;
    }
    */
}

void analyze2()
{
    double total_time = (double) Exec.NowUp();

    DisjointSets sets;

    map<Object*, EdgeSet> strong_edges;
    map<Object*, EdgeSet> weak_edges;
#if 0
    // This was an analysis of ALL edges. I removed the EdgeSet in heap.h/cpp
    // because this I haven't used this analysis.
    //
    for ( EdgeSet::iterator i = Heap.begin_edges();
          i != Heap.end_edges();
          ++i ) {
        Edge* edge = *i;

        Object* source = edge->getSource();
        Object* target = edge->getTarget();

        // -- Compare lifetimes
        // Invariants:
        //   CreateTime(edge) > CreateTime(source)
        //   CreateTime(edge) > CreateTime(target)
        //   EndTIme(edge) <= DeathTime(source)
        //   EndTime(edge) < DeathTime(target)

        double source_overlap = compute_overlap( source->getCreateTime(),
                                                 source->getDeathTime(),
                                                 edge->getCreateTime(),
                                                 edge->getEndTime() );

        double target_overlap = compute_overlap( source->getCreateTime(),
                                                 source->getDeathTime(),
                                                 target->getCreateTime(),
                                                 target->getDeathTime() );

        unsigned int edge_lifetime = edge->getEndTime() - edge->getCreateTime();
        double perc_lifetime = (((double) edge_lifetime)/total_time) * 100.0;

        sets.make_set(source);
        sets.make_set(target);
        if ( (source_overlap > 0.9) and
             (target_overlap > 0.9 ) ) {
            sets.setunion(source, target);
            strong_edges[source].insert(edge);
        } else {
            weak_edges[source].insert(edge);
        }

        /*
           if (perc_lifetime > 0.1)
           printf("Strength s->e %5.3f s->t %5.3f lifetime %5.5f \n",
           source_overlap, target_overlap, perc_lifetime);
           */
    }

    map<Object*, set< Object*> > the_sets;
    sets.get_sets(the_sets);

    int num_small_sets = 0;

    for ( map<Object*, set< Object*> >::iterator j = the_sets.begin();
          j != the_sets.end();
          ++j ) {
        pair<Object* const, set<Object*> > & entry = *j;
        set<Object*>& one_set = entry.second;

        set<Object*> targets;

        int out_edges = 0;
        if (one_set.size() <= 5) {
            num_small_sets++;
        }
        if (one_set.size() > 1000) {
            for ( set<Object*>::iterator k = one_set.begin();
                  k != one_set.end();
                  ++k ) {
                Object* obj = *k;

                if (weak_edges.find(obj) != weak_edges.end()) {
                    EdgeSet & weak = weak_edges[obj];
                    for ( EdgeSet::iterator m = weak.begin();
                          m != weak.end();
                          ++m ) {
                        Edge* weak_edge = *m;
                        if (one_set.find(weak_edge->getTarget()) == one_set.end()) {
                            // printf("OUT-EDGE from %s to %s\n", weak_edge->getSource()->info().c_str(), weak_edge->getTarget()->info().c_str());
                            targets.insert(sets.find(weak_edge->getTarget()));
                            out_edges++;
                        }
                    }
                }
            }
            cout << "Set with size " << one_set.size()
                 << " has " << out_edges << " outgoing edges to "
                 << targets.size() << " sets"
                 << endl;
            cout << "  Root is " << entry.first->info()
                 << " created " << entry.first->getCreateTime()
                 << " died " << entry.first->getDeathTime()
                 << endl;
            for ( set<Object*>::iterator n = targets.begin();
                  n != targets.end();
                  ++n ) {
                Object* target = *n;
                cout << "   Edge to set size " << the_sets[target].size()
                     << " root " << target->info() << " created " << target->getCreateTime()
                     << " died " << target->getDeathTime()
                     << endl;
            }
        } // if (one_set.size() > 1000)
    } // for ( map<Object*, set< Object*> >::iterator j = the_sets.begin(); ... )

    cout << "Total objects " << Heap.size() << endl;
    cout << "Total sets " << the_sets.size() << endl;
    cout << "Number small sets " << num_small_sets << endl;

    // sets.report();
#endif
}
