# TODO: Document this pytest functional test
import os
import subprocess
import sys

def run_java( java8_path = None, java_class = None ):
    if not os.path.isfile( java8_path ):
        # Java file not found
        raise FileNotFoundError("java %s not found." % java8_path)
    fp = subprocess.Popen( [ java8_path, "-cp", "java", java_class ],
                           stdout = subprocess.PIPE,
                           stderr = subprocess.PIPE ).stdout
    return fp

# Design questions:
# 1. Do we build the et2 library here?
# 2. Do we compile the target java programs here?
def test_hello_world(capsys):
    # Check to see that HelloWorld runs
    # HelloWorld.java is in `java/`
    # TODO: Pass in the Java path.
    with run_java( "/etc/alternatives/java_sdk_1.8.0/bin/java",
                   "HelloWorld" ) as fp:
        for x in fp:
            print(x)
    out, err = capsys.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)
    assert out.find("Hello world.")
