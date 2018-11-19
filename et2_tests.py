# TODO: Document this pytest functional test
import os
import subprocess
import sys

from shutil import copyfile

# Runs java on the class using the ET2 agent.
def run_java( java_path,
              agent_path,
              java_class,
              tmp_path ):
    # Copy libet2.so file
    et2_filename = "libet2.so"
    src_path = os.path.join(agent_path, et2_filename)
    tgt_path = tmp_path.joinpath(et2_filename)
    copyfile( src_path,
              tgt_path.absolute().as_posix() )
    # Copy class file
    # For now, the classpath is hardcoded:
    classpath = "java"
    java_class_filename = java_class + ".class"
    class_src_path = os.path.join(classpath, java_class_filename)
    class_tgt_path = tmp_path.joinpath(java_class_filename)
    copyfile( class_src_path,
              class_tgt_path.absolute().as_posix() )
    # Setup environment variables
    tmp_path_name = tmp_path.absolute().as_posix()
    my_env = os.environ.copy()
    my_env["LD_LIBRARY_PATH"] = tmp_path_name + my_env["LD_LIBRARY_PATH"]
    # Setup working directory
    if not os.path.isfile( java_path ):
        # Java file not found
        raise FileNotFoundError("java %s not found." % java_path)
    cmd = [ java_path,
            "-cp", tmp_path_name,
            "-agentpath:" + os.path.join(tmp_path_name, et2_filename),
            java_class ]
    fp = subprocess.Popen( cmd,
                           stdout = subprocess.PIPE,
                           stderr = subprocess.STDOUT,
                           cwd = tmp_path_name,
                           env = my_env ).stdout
    return fp

# Design questions:
# 1. Do we build the et2 library here?
# 2. Do we compile the target java programs here?
def test_hello_world( capsys,
                      java_path,
                      agent_path,
                      tmp_path ):
    # Either check that HelloWorld.class is there.
    # Or compile it here.
    # Check to see that HelloWorld runs
    # HelloWorld.java is in `java/`
    found = False
    with run_java( java_path = java_path,
                   agent_path = agent_path,
                   tmp_path = tmp_path,
                   java_class = "HelloWorld" ) as fp:
        for x in fp:
            xstr = x.decode('utf-8')
            if xstr.find("Hello world.") != -1:
                found = True
                break
        assert found

def test_methods( capsys,
                  java_path,
                  agent_path,
                  tmp_path ):
    # Either check that Methods.class is there.
    # Or compile it here.
    # Methods.java is in `java/`
    with run_java( java_path = java_path,
                   agent_path = agent_path,
                   tmp_path = tmp_path,
                   java_class = "Methods" ) as fp:
        for x in fp:
            sys.stdout.write(x.decode('utf-8'))
    out, err = capsys.readouterr()
    sys.stdout.write(out)
    sys.stderr.write(err)
    # TODO:
    # - Need to run with ET2
