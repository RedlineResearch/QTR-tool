# Configure the command line parameter for the pytest tests.
def pytest_addoption(parser):
    parser.addoption( "--java_path",
                      action = "store",
                      help = "Java 8 path" )
    parser.addoption( "--agent_path",
                      action = "store",
                      help = "ET2 agent path" )

def pytest_generate_tests(metafunc):
    # This is called for every test. Only get/set command line arguments
    # if the argument is specified in the list of test "fixturenames".
    # Get the java_path
    java_path = metafunc.config.option.java_path
    if "java_path" in metafunc.fixturenames and java_path is not None:
        metafunc.parametrize("java_path", [java_path])
    else:
        sys.stderr.write("Please provide path to Java 1.8 binary via --java_path")
    # Get the agent_path
    agent_path = metafunc.config.option.agent_path
    if "agent_path" in metafunc.fixturenames and agent_path is not None:
        metafunc.parametrize("agent_path", [agent_path])
    else:
        sys.stderr.write("Please provide path to Elephant Tracks directory via --agent_path")
