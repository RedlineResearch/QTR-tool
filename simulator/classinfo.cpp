#include "classinfo.hpp"
#include "tokenizer.hpp"

#include <algorithm>
#include <assert.h>
#include <deque>
#include <map>
#include <stdlib.h>
#include <utility>

#include "et2utility.hpp"

// -- Contents of the names file
ClassMap ClassInfo::TheClasses;
RevClassMap ClassInfo::rev_map;
//     Max class id
TypeId_t ClassInfo::max_class_id = 0;

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

/********************************************************************************
 *
 *  Elephant Tracks Version 1
 *
 ********************************************************************************/
// -- Read in the names file with main
void ClassInfo::read_names_file( const char *filename,
                                 string main_class,
                                 string main_function )
{
    ClassInfo::set_main_flag();
    ClassInfo::impl_read_names_file_et1( filename,
                                         main_class,
                                         main_function );
}

// -- Read in the names file with main
void ClassInfo::read_names_file_no_mainfunc( const char *filename )
{
    ClassInfo::unset_main_flag();
    string main_class("--NOMAIN--");
    ClassInfo::impl_read_names_file_et1( filename,
                                         main_class,
                                         main_class ); // reuse the string
}

// -- Read in the names file
void ClassInfo::impl_read_names_file_et1( const char *filename,
                                          string main_class,
                                          string main_function )
{
    FILE *f = fopen(filename, "r");
    if (!f) {
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

                    if (debug_names) {
                        cout << "CLASS " << cls->getName() << " id = " << cls->getId() << endl;
                    }

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
}


/********************************************************************************
 *
 *  Elephant Tracks Version 2
 *
 ********************************************************************************/

void ClassInfo::read_names_file_et2( const char *classes_filename,
                                     const char *fields_filename,
                                     const char *methods_filename )
{
    // TODO: ClassInfo::set_main_flag();
    ClassInfo::read_names_file_no_mainfunc_et2( classes_filename,
                                                fields_filename,
                                                methods_filename );
}

// -- Read in the names file with main
void ClassInfo::read_names_file_no_mainfunc_et2( const char *classes_filename,
                                                 const char *fields_filename,
                                                 const char *methods_filename )
{
    ClassInfo::unset_main_flag();
    string main_class("--NOMAIN--");
    ClassInfo::impl_read_names_file_et2( classes_filename,
                                         fields_filename,
                                         methods_filename,
                                         main_class,
                                         main_class ); // reuse the string
}

// -- Read in the names files
void ClassInfo::impl_read_names_file_et2( const char *classes_filename,
                                          const char *fields_filename,
                                          const char *methods_filename,
                                          string main_class,
                                          string main_function )
{
    ClassInfo::impl_read_classes_file_et2(classes_filename);
    ClassInfo::impl_read_fields_file_et2(fields_filename);
    ClassInfo::impl_read_methods_file_et2(methods_filename);
}


// -- Read in the classes file
void ClassInfo::impl_read_classes_file_et2( const char *classes_filename )
{
    FILE *f_classes = fopen(classes_filename, "r");
    if (!f_classes) {
        cout << "ERROR: Could not open " << classes_filename << endl;
        exit(-1);
    }

    unsigned int maxId = 0;
    Tokenizer t_classes(f_classes);
    while ( !t_classes.isDone() ) {
        t_classes.getLine();
        if (t_classes.isDone()) {
            break;
        }

        TypeId_t type_id = t_classes.getInt(0);
        string name = t_classes.getString(1);
        auto iter = rev_map.find(name);
        if (iter != rev_map.end()) {
            // Duplicate type id
            cerr << "DUPLICATE type id: " << type_id << endl;
            // TODO: Add more info.
            continue;
        }
        rev_map[name] = type_id;
        // -- Class entries:
        // -- Lookup or create the class info...
        Class* cls = NULL;
        auto iter2 = TheClasses.find(type_id);
        if (iter2 == TheClasses.end()) {
            // -- Not found, make a new one
            cls = new Class(type_id, name, false); // false because we have no info on interfaces -RLV
            max_class_id = std::max(max_class_id, type_id);
            TheClasses[cls->getId()] = cls;
        } else {
            cls = iter2->second;
            cerr << "DUPLICATE[ " << cls->getName() << " ]: This shouldn't happen." << endl;
        }

        if (debug_names) {
            cout << "CLASS " << cls->getName() << " id = " << cls->getId() << endl;
        }
    }
}

// -- Read in the fields file
std::map<FieldId_t, string>
ClassInfo::impl_read_fields_file_et2( const char *fields_filename )
{
    FILE *f_fields = fopen(fields_filename, "r");
    if (!f_fields) {
        cout << "ERROR: Could not open " << fields_filename << endl;
        exit(-1);
    }

    unsigned int maxId = 0;
    Tokenizer t_fields(f_fields);
    std::map< FieldId_t, string> field_map;
    while (!t_fields.isDone()) {
        t_fields.getLine();
        if (t_fields.isDone()) {
            break;
        }

        FieldId_t field_id = t_fields.getInt(0);
        string name = t_fields.getString(1);
        auto iter = field_map.find(field_id);
        if (iter != field_map.end()) {
            // Duplicate field id
            cerr << "DUPLICATE field id: " << field_id << endl;
            // TODO: Add more info.
            continue;
        }
        field_map[field_id] = name;
        auto classpath = split(name, '/');
        string class_name;
        string method_name;
        if (classpath.size() >= 2 ) {
            class_name = classpath[classpath.size() - 2];
            method_name = classpath[classpath.size() - 1];
        }
        // TODO: RLV
        // What if the classname isn't in TheClasses yet?
        auto reviter = rev_map.find(class_name);
        Class *cls = NULL;
        if (reviter == rev_map.end()) {
            // Class not yet in the map!
            TypeId_t type_id = max_class_id;
            max_class_id += 1;
            cls = new Class(type_id, class_name, false); // false because we have no info on interfaces -RLV
            TheClasses[cls->getId()] = cls;
        } else {
            cerr << "Class found: " << class_name << endl;
            cls = TheClasses[reviter->second];
        }
        assert(cls != NULL);
        string todo= "TODO";
        Field *fld = new Field( field_id,
                                cls,
                                name.c_str(), // Field name
                                todo.c_str(),
                                false ); // Is static? TODO: We don't have this info.
        TheFields[fld->getId()] = fld;
        cls->addField(fld);
        if (debug_names) {
            cout << "   + FIELD" << fld->getName()
                 << " id = " << fld->getId()
                 << " in class " << cls->getName()
                 << endl;
        }
    }

    return field_map;
}

// -- Read in the methods file
Et2_MethodMap
ClassInfo::impl_read_methods_file_et2( const char *methods_filename )
{
    FILE *f_methods = fopen(methods_filename, "r");
    if (!f_methods) {
        cout << "ERROR: Could not open " << methods_filename << endl;
        exit(-1);
    }

    unsigned int maxId = 0;
    Tokenizer t_methods(f_methods);
    Et2_MethodMap method_map;
    while ( !t_methods.isDone() ) {
        t_methods.getLine();
        if (t_methods.isDone()) {
            break;
        }

        MethodId_t method_id = t_methods.getInt(0);
        string class_name = t_methods.getString(1);
        string method_name = t_methods.getString(1);
        auto iter = method_map.find(method_id);
        if (iter != method_map.end()) {
            // Duplicate method id
            cerr << "DUPLICATE method id: " << method_id << endl;
            // TODO: Add more info.
            continue;
        }
    }

    return method_map;
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

