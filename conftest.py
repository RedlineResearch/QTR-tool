# Configure the command line parameter for the pytest tests.
def pytest_addoption(parser):
    parser.addoption( "--java_path",
                      action = "store",
                      help = "Java 8 path" )

def pytest_generate_tests(metafunc):
    # This is called for every test. Only get/set command line arguments
    # if the argument is specified in the list of test "fixturenames".
    option_value = metafunc.config.option.java_path
    if "java_path" in metafunc.fixturenames and option_value is not None:
        metafunc.parametrize("java_path", [option_value])
    else:
        assert False
