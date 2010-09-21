"""
Test lldb breakpoint command add/list/remove.
"""

import os, time
import unittest2
import lldb
from lldbtest import *

class BreakpointCommandTestCase(TestBase):

    mydir = "breakpoint_command"

    @classmethod
    def classCleanup(cls):
        system(["/bin/sh", "-c", "rm output.txt"])

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    def test_with_dsym(self):
        """Test a sequence of breakpoint command add, list, and remove."""
        self.buildDsym()
        self.breakpoint_command_sequence()

    def test_with_dwarf(self):
        """Test a sequence of breakpoint command add, list, and remove."""
        self.buildDwarf()
        self.breakpoint_command_sequence()

    def breakpoint_command_sequence(self):
        """Test a sequence of breakpoint command add, list, and remove."""
        exe = os.path.join(os.getcwd(), "a.out")
        self.runCmd("file " + exe, CURRENT_EXECUTABLE_SET)

        # Add two breakpoints on the same line.
        self.expect("breakpoint set -f main.c -l 12", BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='main.c', line = 12, locations = 1")
        self.expect("breakpoint set -f main.c -l 12", BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 2: file ='main.c', line = 12, locations = 1")

        # Now add callbacks for the breakpoints just created.
        self.runCmd("breakpoint command add -c -o 'frame variable -s' 1")
        self.runCmd("breakpoint command add -p -o 'here = open(\"output.txt\", \"w\"); print >> here, \"lldb\"; here.close()' 2")

        # Check that the breakpoint commands are correctly set.

        # The breakpoint list now only contains breakpoint 1.
        self.expect("breakpoint list", "Breakpoints 1 & 2 created",
            substrs = ["1: file ='main.c', line = 12, locations = 1",
                       "2: file ='main.c', line = 12, locations = 1"],
            patterns = ["1.1: .+at main.c:12, .+unresolved, hit count = 0",
                        "2.1: .+at main.c:12, .+unresolved, hit count = 0"])

        self.expect("breakpoint command list 1", "Breakpoint 1 command ok",
            substrs = ["Breakpoint commands:",
                          "frame variable -s"])
        self.expect("breakpoint command list 2", "Breakpoint 2 command ok",
            substrs = ["Breakpoint commands:",
                          "here = open",
                          "print >> here",
                          "here.close()"])

        # Run the program.
        self.runCmd("run", RUN_SUCCEEDED)

        # Check that the file 'output.txt' exists and contains the string "lldb".

        # Read the output file produced by running the program.
        output = open('output.txt', 'r').read()

        self.assertTrue(output.startswith("lldb"),
                        "File 'output.txt' and the content matches")

        # Finish the program.
        self.runCmd("process continue")

        # Remove the breakpoint command associated with breakpoint 1.
        self.runCmd("breakpoint command remove 1")

        # Remove breakpoint 2.
        self.runCmd("breakpoint delete 2")

        self.expect("breakpoint command list 1",
            startstr = "Breakpoint 1 does not have an associated command.")
        self.expect("breakpoint command list 2", error=True,
            startstr = "error: '2' is not a currently valid breakpoint id.")

        # The breakpoint list now only contains breakpoint 1.
        self.expect("breakpoint list", "Breakpoint 1 exists",
            substrs = ["1: file ='main.c', line = 12, locations = 1, resolved = 1",
                       "hit count = 1"])

        # Not breakpoint 2.
        self.expect("breakpoint list", "No more breakpoint 2", matching=False,
            substrs = ["2: file ='main.c', line = 12, locations = 1, resolved = 1"])

        # Run the program again, with breakpoint 1 remaining.
        self.runCmd("run", RUN_SUCCEEDED)

        # We should be stopped again due to breakpoint 1.

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['state is Stopped',
                       'stop reason = breakpoint'])

        # The breakpoint should have a hit count of 2.
        self.expect("breakpoint list", BREAKPOINT_HIT_ONCE,
            substrs = ['resolved, hit count = 2'])


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
