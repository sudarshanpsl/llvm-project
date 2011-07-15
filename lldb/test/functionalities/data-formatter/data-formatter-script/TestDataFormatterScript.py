"""
Test lldb data formatter subsystem.
"""

import os, time
import unittest2
import lldb
from lldbtest import *

class DataFormatterTestCase(TestBase):

    mydir = os.path.join("functionalities", "data-formatter", "data-formatter-script")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    def test_with_dsym_and_run_command(self):
        """Test data formatter commands."""
        self.buildDsym()
        self.data_formatter_commands()

    def test_with_dwarf_and_run_command(self):
        """Test data formatter commands."""
        self.buildDwarf()
        self.data_formatter_commands()

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # Find the line number to break at.
        self.line = line_number('main.cpp', '// Set break point at this line.')

    def data_formatter_commands(self):
        """Test that that file and class static variables display correctly."""
        self.runCmd("file a.out", CURRENT_EXECUTABLE_SET)

        self.expect("breakpoint set -f main.cpp -l %d" % self.line,
                    BREAKPOINT_CREATED,
            startstr = "Breakpoint created: 1: file ='main.cpp', line = %d, locations = 1" %
                        self.line)

        self.runCmd("run", RUN_SUCCEEDED)

        # The stop reason of the thread should be breakpoint.
        self.expect("thread list", STOPPED_DUE_TO_BREAKPOINT,
            substrs = ['stopped',
                       'stop reason = breakpoint'])

        # This is the function to remove the custom formats in order to have a
        # clean slate for the next test case.
        def cleanup():
            self.runCmd('type format clear', check=False)
            self.runCmd('type summary clear', check=False)

        # Execute the cleanup function during test case tear down.
        self.addTearDownHook(cleanup)

        # Set the script here to ease the formatting
        script = 'a = valobj.GetChildMemberWithName(\'integer\'); a_val = a.GetValue(); str = \'Hello from Python, \' + a_val + \' time\'; return str + (\'!\' if a_val == \'1\' else \'s!\');'

        self.runCmd("type summary add add i_am_cool -s \"%s\"" % script)

        self.expect("frame variable one",
            substrs = ['Hello from Python',
                       '1 time!'])

        self.expect("frame variable two",
            substrs = ['Hello from Python',
                       '4 times!'])
        
        self.runCmd("n"); # skip ahead to make values change

        self.expect("frame variable three",
            substrs = ['Hello from Python, 10 times!',
                       'Hello from Python, 4 times!'])

        self.runCmd("n"); # skip ahead to make values change
    
        self.expect("frame variable two",
            substrs = ['Hello from Python',
                       '1 time!'])

        script = 'a = valobj.GetChildMemberWithName(\'integer\'); a_val = a.GetValue(); str = \'int says \' + a_val; return str;'

        # Check that changes in the script are immediately reflected
        self.runCmd("type summary add i_am_cool -s \"%s\"" % script)

        self.expect("frame variable two",
                    substrs = ['int says 1'])

        # Change the summary
        self.runCmd("type summary add -f \"int says ${var.integer}, and float says ${var.floating}\" i_am_cool")

        self.expect("frame variable two",
                    substrs = ['int says 1',
                               'and float says 2.71'])
        # Try it for pointers
        self.expect("frame variable twoptr",
                    substrs = ['int says 1',
                               'and float says 2.71'])

        # Force a failure for pointers
        self.runCmd("type summary add i_am_cool -p -s \"%s\"" % script)

        self.expect("frame variable twoptr", matching=False,
                    substrs = ['and float says 2.71'])

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
