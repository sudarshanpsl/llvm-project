"""
LLDB module which provides the abstract base class of lldb test case.

The concrete subclass can override lldbtest.TesBase in order to inherit the
common behavior for unitest.TestCase.setUp/tearDown implemented in this file.

The subclass should override the attribute mydir in order for the python runtime
to locate the individual test cases when running as part of a large test suite
or when running each test case as a separate python invocation.

./dotest.py provides a test driver which sets up the environment to run the
entire test suite.  Users who want to run a test case on its own can specify the
LLDB_TEST and PYTHONPATH environment variables, for example:

$ export LLDB_TEST=$PWD
$ export PYTHONPATH=/Volumes/data/lldb/svn/trunk/build/Debug/LLDB.framework/Resources/Python:$LLDB_TEST
$ echo $LLDB_TEST
/Volumes/data/lldb/svn/trunk/test
$ echo $PYTHONPATH
/Volumes/data/lldb/svn/trunk/build/Debug/LLDB.framework/Resources/Python:/Volumes/data/lldb/svn/trunk/test
$ python function_types/TestFunctionTypes.py
.
----------------------------------------------------------------------
Ran 1 test in 0.363s

OK
$ LLDB_COMMAND_TRACE=YES python array_types/TestArrayTypes.py
LLDB_COMMAND_TRACE=YES python array_types/TestArrayTypes.py
runCmd: file /Volumes/data/lldb/svn/trunk/test/array_types/a.out
output: Current executable set to '/Volumes/data/lldb/svn/trunk/test/array_types/a.out' (x86_64).

runCmd: breakpoint set -f main.c -l 42
output: Breakpoint created: 1: file ='main.c', line = 42, locations = 1

runCmd: run
output: Launching '/Volumes/data/lldb/svn/trunk/test/array_types/a.out'  (x86_64)

runCmd: thread list
output: Process 24987 state is Stopped
  thread #1: tid = 0x2e03, pc = 0x0000000100000df4, where = a.out`main + 612 at /Volumes/data/lldb/svn/trunk/test/array_types/main.c:45, stop reason = breakpoint 1.1, queue = com.apple.main-thread

runCmd: breakpoint list
output: Current breakpoints:
1: file ='main.c', line = 42, locations = 1, resolved = 1
  1.1: where = a.out`main + 612 at /Volumes/data/lldb/svn/trunk/test/array_types/main.c:45, address = 0x0000000100000df4, resolved, hit count = 1 


runCmd: variable list strings
output: (char *[4]) strings = {
  (char *) strings[0] = 0x0000000100000f0c "Hello",
  (char *) strings[1] = 0x0000000100000f12 "Hola",
  (char *) strings[2] = 0x0000000100000f17 "Bonjour",
  (char *) strings[3] = 0x0000000100000f1f "Guten Tag"
}

runCmd: variable list char_16
output: (char [16]) char_16 = {
  (char) char_16[0] = 'H',
  (char) char_16[1] = 'e',
  (char) char_16[2] = 'l',
  (char) char_16[3] = 'l',
  (char) char_16[4] = 'o',
  (char) char_16[5] = ' ',
  (char) char_16[6] = 'W',
  (char) char_16[7] = 'o',
  (char) char_16[8] = 'r',
  (char) char_16[9] = 'l',
  (char) char_16[10] = 'd',
  (char) char_16[11] = '\n',
  (char) char_16[12] = '\0',
  (char) char_16[13] = '\0',
  (char) char_16[14] = '\0',
  (char) char_16[15] = '\0'
}

runCmd: variable list ushort_matrix
output: (unsigned short [2][3]) ushort_matrix = {
  (unsigned short [3]) ushort_matrix[0] = {
    (unsigned short) ushort_matrix[0][0] = 0x0001,
    (unsigned short) ushort_matrix[0][1] = 0x0002,
    (unsigned short) ushort_matrix[0][2] = 0x0003
  },
  (unsigned short [3]) ushort_matrix[1] = {
    (unsigned short) ushort_matrix[1][0] = 0x000b,
    (unsigned short) ushort_matrix[1][1] = 0x0016,
    (unsigned short) ushort_matrix[1][2] = 0x0021
  }
}

runCmd: variable list long_6
output: (long [6]) long_6 = {
  (long) long_6[0] = 1,
  (long) long_6[1] = 2,
  (long) long_6[2] = 3,
  (long) long_6[3] = 4,
  (long) long_6[4] = 5,
  (long) long_6[5] = 6
}

.
----------------------------------------------------------------------
Ran 1 test in 0.349s

OK
$ 
"""

import os
import sys
import time
import unittest2
import lldb

#
# Some commonly used assert messages.
#

CURRENT_EXECUTABLE_SET = "Current executable set successfully"

RUN_STOPPED = "Process is launched and then stopped successfully"

RUN_COMPLETED = "Process exited successfully"

BREAKPOINT_CREATED = "Breakpoint created successfully"

BREAKPOINT_PENDING_CREATED = "Pending breakpoint created successfully"

BREAKPOINT_HIT_ONCE = "Breakpoint resolved with hit cout = 1"

STOPPED_DUE_TO_BREAKPOINT = "Process state is stopped due to breakpoint"

STOPPED_DUE_TO_STEP_IN = "Process state is stopped due to step in"

DATA_TYPES_DISPLAYED_CORRECTLY = "Data type(s) displayed correctly"

VARIABLES_DISPLAYED_CORRECTLY = "Variable(s) displayed correctly"

#
# And a generic "Command '%s' returns successfully" message generator.
#
def CMD_MSG(command):
    return "Command '%s' returns successfully" % (command)


class TestBase(unittest2.TestCase):
    """This LLDB abstract base class is meant to be subclassed."""

    # The concrete subclass should override this attribute.
    mydir = None

    # State pertaining to the inferior process, if any.
    runStarted = False

    # Maximum allowed attempts when launching the inferior process.
    # Can be overridden by the LLDB_MAX_LAUNCH_COUNT environment variable.
    maxLaunchCount = 3;

    # Time to wait before the next launching attempt in second(s).
    # Can be overridden by the LLDB_TIME_WAIT environment variable.
    timeWait = 1.0;

    # os.environ["LLDB_COMMAND_TRACE"], if set to "YES", will turn on this flag.
    traceAlways = False;

    def setUp(self):
        #import traceback
        #traceback.print_stack()

        # Fail fast if 'mydir' attribute is not overridden.
        if not self.mydir or len(self.mydir) == 0:
            raise Exception("Subclasses must override the 'mydir' attribute.")
        # Save old working directory.
        self.oldcwd = os.getcwd()

        # Change current working directory if ${LLDB_TEST} is defined.
        # See also dotest.py which sets up ${LLDB_TEST}.
        if ("LLDB_TEST" in os.environ):
            os.chdir(os.path.join(os.environ["LLDB_TEST"], self.mydir));

        if "LLDB_MAX_LAUNCH_COUNT" in os.environ:
            self.maxLaunchCount = int(os.environ["LLDB_MAX_LAUNCH_COUNT"])

        if "LLDB_TIME_WAIT" in os.environ:
            self.timeWait = float(os.environ["LLDB_TIME_WAIT"])

        if ("LLDB_COMMAND_TRACE" in os.environ and
            os.environ["LLDB_COMMAND_TRACE"] == "YES"):
            self.traceAlways = True

        # Create the debugger instance if necessary.
        try:
            self.dbg = lldb.DBG
        except AttributeError:
            self.dbg = lldb.SBDebugger.Create()

        if not self.dbg.IsValid():
            raise Exception('Invalid debugger instance')

        # We want our debugger to be synchronous.
        self.dbg.SetAsync(False)

        # Retrieve the associated command interpreter instance.
        self.ci = self.dbg.GetCommandInterpreter()
        if not self.ci:
            raise Exception('Could not get the command interpreter')

        # And the result object.
        self.res = lldb.SBCommandReturnObject()

    def tearDown(self):
        # Finish the inferior process, if it was "run" previously.
        if self.runStarted:
            self.ci.HandleCommand("continue", self.res)

        del self.dbg

        # Restore old working directory.
        os.chdir(self.oldcwd)

    def runCmd(self, cmd, msg=None, check=True, trace=False):
        """
        Ask the command interpreter to handle the command and then check its
        return status.
        """
        # Fail fast if 'cmd' is not meaningful.
        if not cmd or len(cmd) == 0:
            raise Exception("Bad 'cmd' parameter encountered")

        trace = (True if self.traceAlways else trace)

        self.runStarted = (cmd.startswith("run") or
                           cmd.startswith("process launch"))

        for i in range(self.maxLaunchCount if self.runStarted else 1):
            self.ci.HandleCommand(cmd, self.res)

            if trace:
                print >> sys.stderr, "runCmd:", cmd
                if self.res.Succeeded():
                    print >> sys.stderr, "output:", self.res.GetOutput()
                else:
                    print >> sys.stderr, self.res.GetError()

            if self.res.Succeeded():
                break
            else:
                if self.runStarted:
                    # Process launch failed, wait some time before the next try.
                    time.sleep(self.timeWait)

        if check:
            self.assertTrue(self.res.Succeeded(),
                            msg if msg else CMD_MSG(cmd))

    def expect(self, cmd, msg=None, startstr=None, substrs=None, trace=False):
        """
        Similar to runCmd; with additional expect style output matching ability.

        Ask the command interpreter to handle the command and then check its
        return status.  The 'msg' parameter specifies an informational assert
        message.  We expect the output from running the command to start with
        'startstr' and matches the substrings contained in 'substrs'.
        """

        trace = (True if self.traceAlways else trace)

        # First run the command.
        self.runCmd(cmd, trace = (True if trace else False))

        # Then compare the output against expected strings.
        output = self.res.GetOutput()
        matched = output.startswith(startstr) if startstr else True

        if startstr and trace:
            print >> sys.stderr, "Expecting start string:", startstr
            print >> sys.stderr, "Matched" if matched else "Not matched"
            print >> sys.stderr

        if substrs and matched:
            for str in substrs:
                matched = output.find(str) > 0
                if trace:
                    print >> sys.stderr, "Expecting sub string:", str
                    print >> sys.stderr, "Matched" if matched else "Not matched"
                if not matched:
                    break
            if trace:
                print >> sys.stderr

        self.assertTrue(matched, msg if msg else CMD_MSG(cmd))

