"""
Test SBprocess and SBThread APIs with printing of the stack traces using lldbutil.
"""

import os, time
import re
import unittest2
import lldb
from lldbtest import *

class ThreadsStackTracesTestCase(TestBase):

    mydir = "python_api/lldbutil"

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break inside main().
        self.line = line_number('main.cpp', '// Set break point at this line.')

    def test_stack_traces(self):
        """Test SBprocess and SBThread APIs with printing of the stack traces."""
        self.buildDefault()
        self.break_and_print_stacktraces()

    def break_and_print_stacktraces(self):
        """Break at main.cpp:68 and do a threads dump"""
        exe = os.path.join(os.getcwd(), "a.out")

        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target.IsValid(), VALID_TARGET)

        breakpoint = target.BreakpointCreateByLocation("main.cpp", self.line)
        self.assertTrue(breakpoint.IsValid(), VALID_BREAKPOINT)

        # Now launch the process, and do not stop at entry point.
        rc = lldb.SBError()
        self.process = target.Launch (self.dbg.GetListener(), None, None, os.ctermid(), os.ctermid(), os.ctermid(), None, 0, False, rc)

        if not rc.Success() or not self.process.IsValid():
            self.fail("SBTarget.LaunchProcess() failed")

        import lldbutil
        if self.process.GetState() != lldb.eStateStopped:
            self.fail("Process should be in the 'stopped' state, "
                      "instead the actual state is: '%s'" %
                      lldbutil.state_type_to_str(self.process.GetState()))

        if self.TraceOn():
            lldbutil.print_stacktraces(self.process)


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
