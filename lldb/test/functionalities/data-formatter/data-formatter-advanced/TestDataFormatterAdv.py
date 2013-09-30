"""
Test lldb data formatter subsystem.
"""

import os, time
import unittest2
import lldb
from lldbtest import *
import lldbutil

class AdvDataFormatterTestCase(TestBase):

    mydir = os.path.join("functionalities", "data-formatter", "data-formatter-advanced")

    @unittest2.skipUnless(sys.platform.startswith("darwin"), "requires Darwin")
    @dsym_test
    def test_with_dsym_and_run_command(self):
        """Test data formatter commands."""
        self.buildDsym()
        self.data_formatter_commands()

    @dwarf_test
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

        lldbutil.run_break_set_by_file_and_line (self, "main.cpp", self.line, num_expected_locations=1, loc_exact=True)

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
            self.runCmd("settings set target.max-children-count 256", check=False)


        # Execute the cleanup function during test case tear down.
        self.addTearDownHook(cleanup)

        self.runCmd("type summary add --summary-string \"pippo\" \"i_am_cool\"")

        self.runCmd("type summary add --summary-string \"pluto\" -x \"i_am_cool[a-z]*\"")

        self.expect("frame variable cool_boy",
            substrs = ['pippo'])

        self.expect("frame variable cooler_boy",
            substrs = ['pluto'])

        self.runCmd("type summary delete i_am_cool")
        
        self.expect("frame variable cool_boy",
            substrs = ['pluto'])

        self.runCmd("type summary clear")
        
        self.runCmd("type summary add --summary-string \"${var[]}\" -x \"int \\[[0-9]\\]")

        self.expect("frame variable int_array",
            substrs = ['1,2,3,4,5'])

        # this will fail if we don't do [] as regex correctly
        self.runCmd('type summary add --summary-string "${var[].integer}" "i_am_cool[]')
        
        self.expect("frame variable cool_array",
            substrs = ['1,1,1,1,6'])

        self.runCmd("type summary clear")
            
        self.runCmd("type summary add --summary-string \"${var[1-0]%x}\" \"int\"")
        
        self.expect("frame variable iAmInt",
            substrs = ['01'])
                
        self.runCmd("type summary add --summary-string \"${var[0-1]%x}\" \"int\"")
        
        self.expect("frame variable iAmInt",
            substrs = ['01'])

        self.runCmd("type summary clear")

        self.runCmd("type summary add --summary-string \"${var[0-1]%x}\" int")
        self.runCmd("type summary add --summary-string \"${var[0-31]%x}\" float")
                    
        self.expect("frame variable *pointer",
            substrs = ['0x',
                       '2'])

        # check fix for <rdar://problem/11338654> LLDB crashes when using a "type summary" that uses bitfields with no format
        self.runCmd("type summary add --summary-string \"${var[0-1]}\" int")
        self.expect("frame variable iAmInt",
            substrs = ['9 1'])

        self.expect("frame variable cool_array[3].floating",
            substrs = ['0x'])
                    
        self.runCmd("type summary add --summary-string \"low bits are ${*var[0-1]} tgt is ${*var}\" \"int *\"")

        self.expect("frame variable pointer",
            substrs = ['low bits are',
                       'tgt is 6'])

        self.expect("frame variable int_array --summary-string \"${*var[0-1]}\"",
            substrs = ['3'])

        self.runCmd("type summary clear")
            
        self.runCmd('type summary add --summary-string \"${var[0-1]}\" -x \"int \[[0-9]\]\"')

        self.expect("frame variable int_array",
            substrs = ['1,2'])

        self.runCmd('type summary add --summary-string \"${var[0-1]}\" "int []"')

        self.expect("frame variable int_array",
            substrs = ['1,2'])

        self.runCmd("type summary clear")

        self.runCmd("type summary add -c -x \"i_am_cool \[[0-9]\]\"")
        self.runCmd("type summary add -c i_am_cool")

        self.expect("frame variable cool_array",
            substrs = ['[0]',
                       '[1]',
                       '[2]',
                       '[3]',
                       '[4]',
                       'integer',
                       'character',
                       'floating'])

        self.runCmd("type summary add --summary-string \"int = ${*var.int_pointer}, float = ${*var.float_pointer}\" IWrapPointers")

        self.expect("frame variable wrapper",
            substrs = ['int = 4',
                       'float = 1.1'])

        self.runCmd("type summary add --summary-string \"low bits = ${*var.int_pointer[2]}\" IWrapPointers -p")
        
        self.expect("frame variable wrapper",
            substrs = ['low bits = 1'])
        
        self.expect("frame variable *wrap_pointer",
            substrs = ['low bits = 1'])

        self.runCmd("type summary clear")

        self.expect("frame variable int_array --summary-string \"${var[0][0-2]%hex}\"",
            substrs = ['0x',
                       '7'])

        self.runCmd("type summary clear")

        self.runCmd("type summary add --summary-string \"${*var[].x[0-3]%hex} is a bitfield on a set of integers\" -x \"SimpleWithPointers \[[0-9]\]\"")

        self.expect("frame variable couple --summary-string \"${*var.sp.x[0-2]} are low bits of integer ${*var.sp.x}. If I pretend it is an array I get ${var.sp.x[0-5]}\"",
            substrs = ['1 are low bits of integer 9.',
                       'If I pretend it is an array I get [9,'])

        # if the summary has an error, we still display the value
        self.expect("frame variable couple --summary-string \"${*var.sp.foo[0-2]\"",
            substrs = ['(Couple) couple = {','sp = {','z =','"X"'])


        self.runCmd("type summary add --summary-string \"${*var.sp.x[0-2]} are low bits of integer ${*var.sp.x}. If I pretend it is an array I get ${var.sp.x[0-5]}\" Couple")

        self.expect("frame variable sparray",
            substrs = ['[0x0000000f,0x0000000c,0x00000009]'])
        
        # check that we can format a variable in a summary even if a format is defined for its datatype
        self.runCmd("type format add -f hex int")
        self.runCmd("type summary add --summary-string \"x=${var.x%d}\" Simple")

        self.expect("frame variable a_simple_object",
            substrs = ['x=3'])

        self.expect("frame variable a_simple_object", matching=False,
                    substrs = ['0x0'])

        # now check that the default is applied if we do not hand out a format
        self.runCmd("type summary add --summary-string \"x=${var.x}\" Simple")

        self.expect("frame variable a_simple_object", matching=False,
                    substrs = ['x=3'])

        self.expect("frame variable a_simple_object", matching=True,
                    substrs = ['x=0x00000003'])

        # check that we can correctly cap the number of children shown
        self.runCmd("settings set target.max-children-count 5")

        self.expect('frame variable a_long_guy', matching=True,
            substrs = ['a_1',
                       'b_1',
                       'c_1',
                       'd_1',
                       'e_1',
                       '...'])

        # check that no further stuff is printed (not ALL values are checked!)
        self.expect('frame variable a_long_guy', matching=False,
                    substrs = ['f_1',
                               'g_1',
                               'h_1',
                               'i_1',
                               'j_1',
                               'q_1',
                               'a_2',
                               'f_2',
                               't_2',
                               'w_2'])

        self.runCmd("settings set target.max-children-count 1")
        self.expect('frame variable a_long_guy', matching=True,
                    substrs = ['a_1',
                               '...'])
        self.expect('frame variable a_long_guy', matching=False,
                    substrs = ['b_1',
                               'c_1',
                               'd_1',
                               'e_1'])
        self.expect('frame variable a_long_guy', matching=False,
                    substrs = ['f_1',
                               'g_1',
                               'h_1',
                               'i_1',
                               'j_1',
                               'q_1',
                               'a_2',
                               'f_2',
                               't_2',
                               'w_2'])

        self.runCmd("settings set target.max-children-count 30")
        self.expect('frame variable a_long_guy', matching=True,
                    substrs = ['a_1',
                               'b_1',
                               'c_1',
                               'd_1',
                               'e_1',
                               'z_1',
                               'a_2',
                               'b_2',
                               'c_2',
                               'd_2',
                               '...'])
        self.expect('frame variable a_long_guy', matching=False,
                    substrs = ['e_2',
                               'n_2',
                               'r_2',
                               'i_2',
                               'k_2',
                               'o_2'])

        # override the cap 
        self.expect('frame variable a_long_guy --show-all-children', matching=True,
                    substrs = ['a_1',
                               'b_1',
                               'c_1',
                               'd_1',
                               'e_1',
                               'z_1',
                               'a_2',
                               'b_2',
                               'c_2',
                               'd_2'])
        self.expect('frame variable a_long_guy --show-all-children', matching=True,
                    substrs = ['e_2',
                               'n_2',
                               'r_2',
                               'i_2',
                               'k_2',
                               'o_2'])
        self.expect('frame variable a_long_guy --show-all-children', matching=False,
                    substrs = ['...'])


if __name__ == '__main__':
    import atexit
    lldb.SBDebugger.Initialize()
    atexit.register(lambda: lldb.SBDebugger.Terminate())
    unittest2.main()
