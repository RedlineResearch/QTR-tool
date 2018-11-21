#include "classinfo.hpp"
#include "tokenizer.h"

#include <stdlib.h>

// -- Contents of the names file
ClassMap ClassInfo::TheClasses;

// -- All methods (also in the classes)
MethodMap ClassInfo::TheMethods;

// -- All fields (also in the classes)
FieldMap ClassInfo::TheFields;

// -- Allocation sites
AllocSiteMap ClassInfo::TheAllocSites;

// -- Debug flag
bool ClassInfo::debug_names = false;

// Should we check for main method? Default is true.
bool ClassInfo::main_flag = true;

// Main method
Method * ClassInfo::_main_method = NULL;

// -- Read in the names file with main
void ClassInfo::read_names_file( const char *filename,
                                 string main_class,
                                 string main_function )
{
    ClassInfo::set_main_flag();
    ClassInfo::__read_names_file( filename,
                                  main_class,
                                  main_function );
}

// -- Read in the names file with main
void ClassInfo::read_names_file_no_mainfunc( const char *filename )
{
    ClassInfo::unset_main_flag();
    string main_class("--NOMAIN--");
    ClassInfo::__read_names_file( filename,
                                  main_class,
                                  main_class ); // reuse the string
}

// -- Read in the names file
void ClassInfo::__read_names_file( const char *filename,
                                   string main_class,
                                   string main_function )
{
    FILE* f = fopen(filename, "r");
    if ( ! f) {
        cout << "ERROR: Could not open " << filename << endl;
        exit(-1);
    }

    unsigned int maxId = 0;
    Tokenizer t(f);
    while ( ! t.isDone() ) {
        t.getLine();
        if (t.isDone()) {
            break;
        }

        switch (t.getChar(0)) {
            case 'C':
            case 'I':
                {
                    // -- Class and interface entries
                    // C/I <id> <name> <other stuff>
                    // -- Lookup or create the class info...
                    Class* cls = 0;
                    ClassMap::iterator p = TheClasses.find(t.getInt(1));
                    if (p == TheClasses.end()) {
                        // -- Not found, make a new one
                        cls = new Class(t.getInt(1), t.getString(2), (t.getChar(0) == 'I'));
                        TheClasses[cls->getId()] = cls;
                    } else {
                        cls = p->second;
                    }

                    if (debug_names)
                        cout << "CLASS " << cls->getName() << " id = " << cls->getId() << endl;

                    // Superclass ID and interfaces are optional
                    if (t.numTokens() > 3)
                        cls->setSuperclassId(t.getInt(3));

                    // -- For now, ignore the rest (interfaces implemented)
                }
                break;

            case 'N': 
                {
                    // N <id> <classid> <classname> <methodname> <descriptor> <flags S|I +N>
                    // 0  1       2          3           4            5             6
                    Class *cls = TheClasses[t.getInt(2)];
                    string classname( t.getString(3) );
                    string methodname( t.getString(4) );
                    unsigned int my_id = t.getInt(1);
                    Method *m = new Method(my_id, cls, t.getString(4), t.getString(5), t.getString(6));
                    maxId = std::max(my_id, maxId);
                    // DEBUG
                    // cout << classname << " | " <<  methodname << endl;
                    if (ClassInfo::main_flag) {
                        if ( (classname.compare(main_class) == 0) &&
                             (methodname.compare(main_function) == 0) ) {
                            // this is the main_package
                            ClassInfo::_main_method = m;
                        }
                    }
                    TheMethods[m->getId()] = m;
                    cls->addMethod(m);
                    if (debug_names) {
                        cout << "   + METHOD " << m->info() << endl;
                    }
                }
                break;

            case 'F':
                {
                    // F S/I <id> <name> <classid> <classname> <descriptor>
                    // 0  1   2     3        4          5           6
                    Class* cls = TheClasses[t.getInt(4)];
                    Field* fld = new Field( t.getInt(2),
                                            cls,
                                            t.getString(3),
                                            t.getString(6),
                                            (t.getChar(1) == 'S') );
                    TheFields[fld->getId()] = fld;
                    cls->addField(fld);
                    if (debug_names) {
                        cout << "   + FIELD" << fld->getName()
                             << " id = " << fld->getId()
                             << " in class " << cls->getName()
                             << endl;
                    }
                }
                break;

            case 'S':
                {
                    // S <methodid> <classid> <id> <type> <dims>
                    // 0    1           2      3     4      5
                    Method* meth = TheMethods[t.getInt(1)];
                    unsigned int my_id = t.getInt(3);
                    maxId = std::max(my_id, maxId);
                    AllocSite* as = new AllocSite( my_id, // Id
                                                   meth, // Method pointer
                                                   t.getString(3), // Maybe a hack? Use the ID as name.
                                                   t.getString(4), // descriptor which in this case means type
                                                   t.getInt(5) );  // Dimensions
                    TheAllocSites[as->getId()] = as;
                    meth->addAllocSite(as);
                    if (debug_names) {
                        cout << "> ALLOC   : " << as->getType() << endl
                             << "    id    = " << as->getId() << endl
                             << " in method: " << meth->info()
                             << endl;
                    }
                }
                break;

            case 'E':
                // -- No need to process this entry (end of a class)
                break;

            default:
                {
                    cout << "ERROR: Unknown char " << t.getChar(0) << endl;
                }
                break;
        }
    }
    
    // TODO: Method *m = new Method(my_id, cls, t.getString(4), t.getString(5), t.getString(6));
}


// ----------------------------------------------------------------------
//  Info methods (for debugging)
string Method::info()
{
    stringstream ss;
    ss << m_class->info() << "." << m_name << " ~ " << m_descriptor;
    return ss.str();
}

string Method::getName()
{
    stringstream ss;
    if (this->m_class) {
        ss << this->m_class->info() << "." << m_name;
    } else {
        ss << "NOTYPE" << "." << m_name;
    }
    return ss.str();
}

string AllocSite::info()
{
    stringstream ss;
    ss << m_method->info() << ":" << m_id;
    return ss.str();
}

