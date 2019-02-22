import argparse
import os
import pprint
import re
import sys
# from operator import itemgetter
# import itertools
# from tempfile import NamedTemporaryFile


#
# Main processing
#

def save_method_in_dict( meth_dict = dict(),
                         rec = "" ):
    meth_id, class_name, meth_name = rec.split(b",")
    meth_id = int(re.sub(b' +', b'', meth_id))
    class_name = re.sub(b' +', b'', class_name)
    meth_name = re.sub(b' +', b'', meth_name)
    assert meth_id not in meth_dict
    meth_dict[meth_id] = { "class" : class_name, "method" : meth_name }

def process_methods( methods_file ):
    # DEBUG: pp = pprint.PrettyPrinter( indent = 4 )
    meth_dict = {} # Format:   meth_id -> { "class" : class_name, "method" : meth_name }
    with open(methods_file, 'rb') as methods:
        num = 1
        for line in methods:
            line = line.rstrip()
            save_method_in_dict( meth_dict = meth_dict,
                                 rec = line )
            # DEBUG: print("%d = %d : %s" % (num, len(line), line))
            # DEBUG: num += 1
    # DEBUG: pp.pprint(meth_dict)
    return meth_dict

def process_trace( trace_file ):
    pass

def main():
    # set up arg parser
    parser = argparse.ArgumentParser()
    parser.add_argument( "--trace",
                         help = "Absolute trace file path.",
                         action = "store",
                         required = True )
    parser.add_argument( "--methods",
                         help = "Absolute methods.ist file path.",
                         action = "store",
                         required = True )
    args = parser.parse_args()
    #
    # Main processing
    #
    meth_dict = process_methods( methods_file = args.methods )
    pp = pprint.PrettyPrinter( indent = 4 )
    pp.pprint(meth_dict)
    # TODO: process_trace()

__all__ = [ "process_methods", ]

if __name__ == "__main__":
    main()
