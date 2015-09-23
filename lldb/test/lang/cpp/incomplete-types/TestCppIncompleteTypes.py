import lldb
from lldbtest import *
import lldbutil

class TestCppIncompleteTypes(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    @dwarf_test
    @skipIfGcc
    def test_with_dwarf_limit_debug_info(self):
        self.buildDwarf()
        frame = self.get_test_frame('limit')

        value_f = frame.EvaluateExpression("f")
        self.assertTrue(value_f.IsValid(), "'expr f' results in a valid SBValue object")
        self.assertFalse(value_f.GetError().Success(), "'expr f' results in an error, but LLDB does not crash")

        value_s = frame.EvaluateExpression("s")
        self.assertTrue(value_s.IsValid(), "'expr s' results in a valid SBValue object")
        self.assertFalse(value_s.GetError().Success(), "'expr s' results in an error, but LLDB does not crash")

    @dwarf_test
    @skipIfGcc
    def test_with_dwarf_partial_limit_debug_info(self):
        self.buildDwarf()
        frame = self.get_test_frame('nolimit')

        value_f = frame.EvaluateExpression("f")
        self.assertTrue(value_f.IsValid(), "'expr f' results in a valid SBValue object")
        self.assertTrue(value_f.GetError().Success(), "'expr f' is successful")

        value_s = frame.EvaluateExpression("s")
        self.assertTrue(value_s.IsValid(), "'expr s' results in a valid SBValue object")
        self.assertTrue(value_s.GetError().Success(), "'expr s' is successful")

    def setUp(self):
        TestBase.setUp(self)

    def get_test_frame(self, exe):
        # Get main source file
        src_file = "main.cpp"
        src_file_spec = lldb.SBFileSpec(src_file)
        self.assertTrue(src_file_spec.IsValid(), "Main source file")

        # Get the path of the executable
        cwd = os.getcwd()
        exe_path  = os.path.join(cwd, exe)

        # Load the executable
        target = self.dbg.CreateTarget(exe_path)
        self.assertTrue(target.IsValid(), VALID_TARGET)

        # Break on main function
        main_breakpoint = target.BreakpointCreateBySourceRegex("break here", src_file_spec)
        self.assertTrue(main_breakpoint.IsValid() and main_breakpoint.GetNumLocations() >= 1, VALID_BREAKPOINT)

        # Launch the process
        args = None
        env = None
        process = target.LaunchSimple(args, env, self.get_process_working_directory())
        self.assertTrue(process.IsValid(), PROCESS_IS_VALID)

        # Get the thread of the process
        self.assertTrue(process.GetState() == lldb.eStateStopped, PROCESS_STOPPED)
        thread = lldbutil.get_stopped_thread(process, lldb.eStopReasonBreakpoint)

        # Get frame for current thread
        return thread.GetSelectedFrame()

if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
