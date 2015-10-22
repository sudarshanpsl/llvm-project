"""
Test some SBValue APIs.
"""

import lldb_shared

import os, time
import re
import lldb, lldbutil
from lldbtest import *

class ValueAPITestCase(TestBase):

    mydir = TestBase.compute_mydir(__file__)

    def setUp(self):
        # Call super's setUp().
        TestBase.setUp(self)
        # We'll use the test method name as the exe_name.
        self.exe_name = self.testMethodName
        # Find the line number to of function 'c'.
        self.line = line_number('main.c', '// Break at this line')

    @expectedFailureWindows("llvm.org/pr24772")
    @python_api_test
    def test(self):
        """Exercise some SBValue APIs."""
        d = {'EXE': self.exe_name}
        self.build(dictionary=d)
        self.setTearDownCleanup(dictionary=d)
        exe = os.path.join(os.getcwd(), self.exe_name)

        # Create a target by the debugger.
        target = self.dbg.CreateTarget(exe)
        self.assertTrue(target, VALID_TARGET)

        # Create the breakpoint inside function 'main'.
        breakpoint = target.BreakpointCreateByLocation('main.c', self.line)
        self.assertTrue(breakpoint, VALID_BREAKPOINT)

        # Now launch the process, and do not stop at entry point.
        process = target.LaunchSimple (None, None, self.get_process_working_directory())
        self.assertTrue(process, PROCESS_IS_VALID)

        # Get Frame #0.
        self.assertTrue(process.GetState() == lldb.eStateStopped)
        thread = lldbutil.get_stopped_thread(process, lldb.eStopReasonBreakpoint)
        self.assertTrue(thread.IsValid(), "There should be a thread stopped due to breakpoint condition")
        frame0 = thread.GetFrameAtIndex(0)

        # Get global variable 'days_of_week'.
        list = target.FindGlobalVariables('days_of_week', 1)
        days_of_week = list.GetValueAtIndex(0)
        self.assertTrue(days_of_week, VALID_VARIABLE)
        self.assertTrue(days_of_week.GetNumChildren() == 7, VALID_VARIABLE)
        self.DebugSBValue(days_of_week)

        # Get global variable 'weekdays'.
        list = target.FindGlobalVariables('weekdays', 1)
        weekdays = list.GetValueAtIndex(0)
        self.assertTrue(weekdays, VALID_VARIABLE)
        self.assertTrue(weekdays.GetNumChildren() == 5, VALID_VARIABLE)
        self.DebugSBValue(weekdays)

        # Get global variable 'g_table'.
        list = target.FindGlobalVariables('g_table', 1)
        g_table = list.GetValueAtIndex(0)
        self.assertTrue(g_table, VALID_VARIABLE)
        self.assertTrue(g_table.GetNumChildren() == 2, VALID_VARIABLE)
        self.DebugSBValue(g_table)

        fmt = lldbutil.BasicFormatter()
        cvf = lldbutil.ChildVisitingFormatter(indent_child=2)
        rdf = lldbutil.RecursiveDecentFormatter(indent_child=2)
        if self.TraceOn():
            print fmt.format(days_of_week)
            print cvf.format(days_of_week)
            print cvf.format(weekdays)
            print rdf.format(g_table)

        # Get variable 'my_int_ptr'.
        value = frame0.FindVariable('my_int_ptr')
        self.assertTrue(value, VALID_VARIABLE)
        self.DebugSBValue(value)

        # Get what 'my_int_ptr' points to.
        pointed = value.GetChildAtIndex(0)
        self.assertTrue(pointed, VALID_VARIABLE)
        self.DebugSBValue(pointed)

        # While we are at it, verify that 'my_int_ptr' points to 'g_my_int'.
        symbol = target.ResolveLoadAddress(int(pointed.GetLocation(), 0)).GetSymbol()
        self.assertTrue(symbol)
        self.expect(symbol.GetName(), exe=False,
            startstr = 'g_my_int')

        # Get variable 'str_ptr'.
        value = frame0.FindVariable('str_ptr')
        self.assertTrue(value, VALID_VARIABLE)
        self.DebugSBValue(value)

        # SBValue::TypeIsPointerType() should return true.
        self.assertTrue(value.TypeIsPointerType())

        # Verify the SBValue::GetByteSize() API is working correctly.
        arch = self.getArchitecture()
        if arch == 'i386':
            self.assertTrue(value.GetByteSize() == 4)
        elif arch == 'x86_64':
            self.assertTrue(value.GetByteSize() == 8)

        # Get child at index 5 => 'Friday'.
        child = value.GetChildAtIndex(5, lldb.eNoDynamicValues, True)
        self.assertTrue(child, VALID_VARIABLE)
        self.DebugSBValue(child)

        self.expect(child.GetSummary(), exe=False,
            substrs = ['Friday'])

        # Now try to get at the same variable using GetValueForExpressionPath().
        # These two SBValue objects should have the same value.
        val2 = value.GetValueForExpressionPath('[5]')
        self.assertTrue(val2, VALID_VARIABLE)
        self.DebugSBValue(val2)
        self.assertTrue(child.GetValue() == val2.GetValue() and
                        child.GetSummary() == val2.GetSummary())

        val_i = target.EvaluateExpression('i')
        val_s = target.EvaluateExpression('s')
        val_a = target.EvaluateExpression('a')
        self.assertTrue(val_s.GetChildMemberWithName('a').AddressOf(), VALID_VARIABLE)
        self.assertTrue(val_a.Cast(val_i.GetType()).AddressOf(), VALID_VARIABLE)
