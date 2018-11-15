#!/bin/sh
git rev-parse HEAD | awk ' BEGIN {print "#include \"version.hpp\""} {print "const char *build_git_sha = \"" $0"\";"} END {}'
date | awk 'BEGIN {} {print "const char *build_git_time = \""$0"\";"} END {} '
