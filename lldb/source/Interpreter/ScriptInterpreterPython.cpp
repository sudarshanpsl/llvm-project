//===-- ScriptInterpreterPython.cpp -----------------------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//

// In order to guarantee correct working with Python, Python.h *MUST* be
// the *FIRST* header file included:

#if defined (__APPLE__)
#include <Python/Python.h>
#else
#include <Python.h>
#endif

#include "lldb/Interpreter/ScriptInterpreterPython.h"


#include <sys/ioctl.h>
#include <termios.h>
#include <stdlib.h>
#include <stdio.h>

#include <string>

#include "lldb/API/SBFrame.h"
#include "lldb/API/SBBreakpointLocation.h"
#include "lldb/Breakpoint/Breakpoint.h"
#include "lldb/Breakpoint/BreakpointLocation.h"
#include "lldb/Breakpoint/StoppointCallbackContext.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Core/FileSpec.h"
#include "lldb/Core/InputReader.h"
#include "lldb/Core/Stream.h"
#include "lldb/Core/StreamString.h"
#include "lldb/Core/Timer.h"
#include "lldb/Host/Host.h"
#include "lldb/Interpreter/CommandInterpreter.h"
#include "lldb/Interpreter/CommandReturnObject.h"
#include "lldb/Core/Debugger.h"
#include "lldb/Target/Process.h"
#include "lldb/Target/Thread.h"

// This function is in the C++ output file generated by SWIG after it is 
// run on all of the headers in "lldb/API/SB*.h"
extern "C" void init_lldb (void);

extern "C" bool
LLDBSWIGPythonBreakpointCallbackFunction 
(
    const char *python_function_name,
    lldb::SBFrame& sb_frame, 
    lldb::SBBreakpointLocation& sb_bp_loc
);

using namespace lldb;
using namespace lldb_private;

const char embedded_interpreter_string[] =
"import readline\n\
import code\n\
import sys\n\
import traceback\n\
\n\
class SimpleREPL(code.InteractiveConsole):\n\
   def __init__(self, prompt, dict):\n\
       code.InteractiveConsole.__init__(self,dict)\n\
       self.prompt = prompt\n\
       self.loop_exit = False\n\
       self.dict = dict\n\
\n\
   def interact(self):\n\
       try:\n\
           sys.ps1\n\
       except AttributeError:\n\
           sys.ps1 = \">>> \"\n\
       try:\n\
           sys.ps2\n\
       except AttributeError:\n\
           sys.ps2 = \"... \"\n\
\n\
       while not self.loop_exit:\n\
           try:\n\
               self.read_py_command()\n\
           except (SystemExit, EOFError):\n\
               # EOF while in Python just breaks out to top level.\n\
               self.write('\\n')\n\
               self.loop_exit = True\n\
               break\n\
           except KeyboardInterrupt:\n\
               self.write(\"\\nKeyboardInterrupt\\n\")\n\
               self.resetbuffer()\n\
               more = 0\n\
           except:\n\
               traceback.print_exc()\n\
\n\
   def process_input (self, in_str):\n\
      # Canonicalize the format of the input string\n\
      temp_str = in_str\n\
      temp_str.strip(' \t')\n\
      words = temp_str.split()\n\
      temp_str = ('').join(words)\n\
\n\
      # Check the input string to see if it was the quit\n\
      # command.  If so, intercept it, so that it doesn't\n\
      # close stdin on us!\n\
      if (temp_str.lower() == \"quit()\" or temp_str.lower() == \"exit()\"):\n\
         self.loop_exit = True\n\
         in_str = \"raise SystemExit \"\n\
      return in_str\n\
\n\
   def my_raw_input (self, prompt):\n\
      stream = sys.stdout\n\
      stream.write (prompt)\n\
      stream.flush ()\n\
      try:\n\
         line = sys.stdin.readline()\n\
      except KeyboardInterrupt:\n\
         line = \" \\n\"\n\
      except (SystemExit, EOFError):\n\
         line = \"quit()\\n\"\n\
      if not line:\n\
         raise EOFError\n\
      if line[-1] == '\\n':\n\
         line = line[:-1]\n\
      return line\n\
\n\
   def read_py_command(self):\n\
       # Read off a complete Python command.\n\
       more = 0\n\
       while 1:\n\
           if more:\n\
               prompt = sys.ps2\n\
           else:\n\
               prompt = sys.ps1\n\
           line = self.my_raw_input(prompt)\n\
           # Can be None if sys.stdin was redefined\n\
           encoding = getattr(sys.stdin, \"encoding\", None)\n\
           if encoding and not isinstance(line, unicode):\n\
               line = line.decode(encoding)\n\
           line = self.process_input (line)\n\
           more = self.push(line)\n\
           if not more:\n\
               break\n\
\n\
def run_python_interpreter (dict):\n\
   # Pass in the dictionary, for continuity from one session to the next.\n\
   repl = SimpleREPL('>>> ', dict)\n\
   repl.interact()\n";

static int
_check_and_flush (FILE *stream)
{
  int prev_fail = ferror (stream);
  return fflush (stream) || prev_fail ? EOF : 0;
}

ScriptInterpreterPython::ScriptInterpreterPython (CommandInterpreter &interpreter) :
    ScriptInterpreter (interpreter, eScriptLanguagePython),
    m_compiled_module (NULL),
    m_termios (),
    m_termios_valid (false),
    m_embedded_python_pty (),
    m_embedded_thread_input_reader_sp ()
{

    Timer scoped_timer (__PRETTY_FUNCTION__, __PRETTY_FUNCTION__);

    // Save terminal settings if we can
    int input_fd;
    FILE *input_fh = m_interpreter.GetDebugger().GetInputFileHandle();
    if (input_fh != NULL)
        input_fd = ::fileno (input_fh);
    else
        input_fd = STDIN_FILENO;
    
    m_termios_valid = ::tcgetattr (input_fd, &m_termios) == 0;
            
    // Find the module that owns this code and use that path we get to
    // set the PYTHONPATH appropriately.

    FileSpec file_spec;
    char python_dir_path[PATH_MAX];
    if (Host::GetLLDBPath (ePathTypePythonDir, file_spec))
    {
        std::string python_path;
        const char *curr_python_path = ::getenv ("PYTHONPATH");
        if (curr_python_path)
        {
            // We have a current value for PYTHONPATH, so lets append to it
            python_path.append (curr_python_path);
        }

        if (file_spec.GetPath(python_dir_path, sizeof (python_dir_path)))
        {
            if (!python_path.empty())
                python_path.append (1, ':');
            python_path.append (python_dir_path);
        }
        
        if (Host::GetLLDBPath (ePathTypeLLDBShlibDir, file_spec))
        {
            if (file_spec.GetPath(python_dir_path, sizeof (python_dir_path)))
            {
                if (!python_path.empty())
                    python_path.append (1, ':');
                python_path.append (python_dir_path);
            }
        }
        const char *pathon_path_env_cstr = python_path.c_str();
        ::setenv ("PYTHONPATH", pathon_path_env_cstr, 1);
    }

    Py_Initialize ();

    PyObject *compiled_module = Py_CompileString (embedded_interpreter_string, 
                                                  "embedded_interpreter.py",
                                                  Py_file_input);

    m_compiled_module = static_cast<void*>(compiled_module);

    // This function is in the C++ output file generated by SWIG after it is 
    // run on all of the headers in "lldb/API/SB*.h"
    init_lldb ();

    // Update the path python uses to search for modules to include the current directory.

    int success = PyRun_SimpleString ("import sys");
    success = PyRun_SimpleString ("sys.path.append ('.')");
    if (success == 0)
    {
        // Import the Script Bridge module.
        success =  PyRun_SimpleString ("import lldb");
    }

    const char *pty_slave_name = GetScriptInterpreterPtyName ();
    FILE *out_fh = interpreter.GetDebugger().GetOutputFileHandle();
    
    PyObject *pmod = PyImport_ExecCodeModule (const_cast<char*> ("embedded_interpreter"),
                                              static_cast<PyObject*>(m_compiled_module));

    if (pmod != NULL)
    {
        PyRun_SimpleString ("ConsoleDict = locals()");
        PyRun_SimpleString ("from embedded_interpreter import run_python_interpreter");
        PyRun_SimpleString ("import sys");
        PyRun_SimpleString ("from termios import *");
      
        if (out_fh != NULL)
        {
            PyObject *new_sysout = PyFile_FromFile (out_fh, (char *) "", (char *) "w", 
                                                        _check_and_flush);
            PyObject *sysmod = PyImport_AddModule ("sys");
            PyObject *sysdict = PyModule_GetDict (sysmod);

            if ((new_sysout != NULL)
                && (sysmod != NULL)
                && (sysdict != NULL))
            {
                PyDict_SetItemString (sysdict, "stdout", new_sysout);
            }

            if (PyErr_Occurred())
                PyErr_Clear();
        }

        StreamString run_string;
        run_string.Printf ("new_stdin = open('%s', 'r')", pty_slave_name);
        PyRun_SimpleString (run_string.GetData());
        PyRun_SimpleString ("sys.stdin = new_stdin");

        run_string.Clear();
        run_string.Printf ("lldb.debugger_unique_id = %d", interpreter.GetDebugger().GetID());
        PyRun_SimpleString (run_string.GetData());
    }


    // Restore terminal settings if they were validly saved
    if (m_termios_valid)
    {
        ::tcsetattr (input_fd, TCSANOW, &m_termios);
    }
}

ScriptInterpreterPython::~ScriptInterpreterPython ()
{
    Py_Finalize ();
}

bool
ScriptInterpreterPython::ExecuteOneLine (const char *command, CommandReturnObject *result)
{
    if (command)
    {
        int success;

        success = PyRun_SimpleString (command);
        if (success == 0)
            return true;

        // The one-liner failed.  Append the error message.
        if (result)
            result->AppendErrorWithFormat ("python failed attempting to evaluate '%s'\n", command);
        return false;
    }

    if (result)
        result->AppendError ("empty command passed to python\n");
    return false;
}



size_t
ScriptInterpreterPython::InputReaderCallback
(
    void *baton, 
    InputReader &reader, 
    InputReaderAction notification,
    const char *bytes, 
    size_t bytes_len
)
{
    lldb::thread_t embedded_interpreter_thread;
    LogSP log (lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_SCRIPT));

    if (baton == NULL)
        return 0;

    ScriptInterpreterPython *script_interpreter = (ScriptInterpreterPython *) baton;            
    switch (notification)
    {
    case eInputReaderActivate:
        {
            // Save terminal settings if we can
            int input_fd;
            FILE *input_fh = reader.GetDebugger().GetInputFileHandle();
            if (input_fh != NULL)
                input_fd = ::fileno (input_fh);
            else
                input_fd = STDIN_FILENO;

            script_interpreter->m_termios_valid = ::tcgetattr (input_fd, &script_interpreter->m_termios) == 0;
            
            char error_str[1024];
            if (script_interpreter->m_embedded_python_pty.OpenFirstAvailableMaster (O_RDWR|O_NOCTTY, error_str, 
                                                                                    sizeof(error_str)))
            {
                if (log)
                    log->Printf ("ScriptInterpreterPython::InputReaderCallback, Activate, succeeded in opening master pty (fd = %d).",
                                  script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor());
                embedded_interpreter_thread = Host::ThreadCreate ("<lldb.script-interpreter.embedded-python-loop>",
                                                                  ScriptInterpreterPython::RunEmbeddedPythonInterpreter,
                                                                  script_interpreter, NULL);
                if (embedded_interpreter_thread != LLDB_INVALID_HOST_THREAD)
                {
                    if (log)
                        log->Printf ("ScriptInterpreterPython::InputReaderCallback, Activate, succeeded in creating thread (thread = %d)", embedded_interpreter_thread);
                    Error detach_error;
                    Host::ThreadDetach (embedded_interpreter_thread, &detach_error);
                }
                else
                {
                    if (log)
                        log->Printf ("ScriptInterpreterPython::InputReaderCallback, Activate, failed in creating thread");
                    reader.SetIsDone (true);
                }
            }
            else
            {
                if (log)
                    log->Printf ("ScriptInterpreterPython::InputReaderCallback, Activate, failed to open master pty ");
                reader.SetIsDone (true);
            }

        }
        break;

    case eInputReaderDeactivate:
        break;

    case eInputReaderReactivate:
        break;
        
    case eInputReaderInterrupt:
        ::write (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor(), "raise KeyboardInterrupt\n", 24);
        break;
        
    case eInputReaderEndOfFile:
        ::write (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor(), "quit()\n", 7);
        break;

    case eInputReaderGotToken:
        if (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor() != -1)
        {
            if (log)
                log->Printf ("ScriptInterpreterPython::InputReaderCallback, GotToken, bytes='%s', byte_len = %d", bytes,
                             bytes_len);
            if (bytes && bytes_len)
            {
                if ((int) bytes[0] == 4)
                    ::write (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor(), "quit()", 6);
                else
                    ::write (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor(), bytes, bytes_len);
            }
            ::write (script_interpreter->m_embedded_python_pty.GetMasterFileDescriptor(), "\n", 1);
        }
        else
        {
            if (log)
                log->Printf ("ScriptInterpreterPython::InputReaderCallback, GotToken, bytes='%s', byte_len = %d, Master File Descriptor is bad.", 
                             bytes,
                             bytes_len);
            reader.SetIsDone (true);
        }

        break;
        
    case eInputReaderDone:
        // Send a control D to the script interpreter
        //::write (interpreter->GetMasterFileDescriptor(), "\nquit()\n", strlen("\nquit()\n"));
        // Write a newline out to the reader output
        //::fwrite ("\n", 1, 1, out_fh);
        // Restore terminal settings if they were validly saved
        if (log)
            log->Printf ("ScriptInterpreterPython::InputReaderCallback, Done, closing down input reader.");
        if (script_interpreter->m_termios_valid)
        {
            int input_fd;
            FILE *input_fh = reader.GetDebugger().GetInputFileHandle();
            if (input_fh != NULL)
                input_fd = ::fileno (input_fh);
            else
                input_fd = STDIN_FILENO;
            
            ::tcsetattr (input_fd, TCSANOW, &script_interpreter->m_termios);
        }
        script_interpreter->m_embedded_python_pty.CloseMasterFileDescriptor();
        break;
    }

    return bytes_len;
}


void
ScriptInterpreterPython::ExecuteInterpreterLoop ()
{
    Timer scoped_timer (__PRETTY_FUNCTION__, __PRETTY_FUNCTION__);

    Debugger &debugger = m_interpreter.GetDebugger();

    // At the moment, the only time the debugger does not have an input file handle is when this is called
    // directly from Python, in which case it is both dangerous and unnecessary (not to mention confusing) to
    // try to embed a running interpreter loop inside the already running Python interpreter loop, so we won't
    // do it.

    if (debugger.GetInputFileHandle() == NULL)
        return;

    InputReaderSP reader_sp (new InputReader(debugger));
    if (reader_sp)
    {
        Error error (reader_sp->Initialize (ScriptInterpreterPython::InputReaderCallback,
                                            this,                         // baton
                                            eInputReaderGranularityLine,  // token size, to pass to callback function
                                            NULL,                         // end token
                                            NULL,                         // prompt
                                            true));                       // echo input
     
        if (error.Success())
        {
            debugger.PushInputReader (reader_sp);
            m_embedded_thread_input_reader_sp = reader_sp;
        }
    }
}

bool
ScriptInterpreterPython::ExecuteOneLineWithReturn (const char *in_string,
                                                   ScriptInterpreter::ReturnType return_type,
                                                   void *ret_value)
{
    PyObject *py_return = NULL;
    PyObject *mainmod = PyImport_AddModule ("__main__");
    PyObject *globals = PyModule_GetDict (mainmod);
    PyObject *locals = globals;
    PyObject *py_error = NULL;
    bool ret_success;
    int success;

    if (in_string != NULL)
    {
        py_return = PyRun_String (in_string, Py_eval_input, globals, locals);
        if (py_return == NULL)
        {
            py_error = PyErr_Occurred ();
            if (py_error != NULL)
                PyErr_Clear ();

            py_return = PyRun_String (in_string, Py_single_input, globals, locals);
        }

        if (py_return != NULL)
        {
            switch (return_type)
            {
                case eCharPtr: // "char *"
                {
                    const char format[3] = "s#";
                    success = PyArg_Parse (py_return, format, (char **) &ret_value);
                    break;
                }
                case eBool:
                {
                    const char format[2] = "b";
                    success = PyArg_Parse (py_return, format, (bool *) ret_value);
                    break;
                }
                case eShortInt:
                {
                    const char format[2] = "h";
                    success = PyArg_Parse (py_return, format, (short *) ret_value);
                    break;
                }
                case eShortIntUnsigned:
                {
                    const char format[2] = "H";
                    success = PyArg_Parse (py_return, format, (unsigned short *) ret_value);
                    break;
                }
                case eInt:
                {
                    const char format[2] = "i";
                    success = PyArg_Parse (py_return, format, (int *) ret_value);
                    break;
                }
                case eIntUnsigned:
                {
                    const char format[2] = "I";
                    success = PyArg_Parse (py_return, format, (unsigned int *) ret_value);
                    break;
                }
                case eLongInt:
                {
                    const char format[2] = "l";
                    success = PyArg_Parse (py_return, format, (long *) ret_value);
                    break;
                }
                case eLongIntUnsigned:
                {
                    const char format[2] = "k";
                    success = PyArg_Parse (py_return, format, (unsigned long *) ret_value);
                    break;
                }
                case eLongLong:
                {
                    const char format[2] = "L";
                    success = PyArg_Parse (py_return, format, (long long *) ret_value);
                    break;
                }
                case eLongLongUnsigned:
                {
                    const char format[2] = "K";
                    success = PyArg_Parse (py_return, format, (unsigned long long *) ret_value);
                    break;
                }
                case eFloat:
                {
                    const char format[2] = "f";
                    success = PyArg_Parse (py_return, format, (float *) ret_value);
                    break;
                }
                case eDouble:
                {
                    const char format[2] = "d";
                    success = PyArg_Parse (py_return, format, (double *) ret_value);
                    break;
                }
                case eChar:
                {
                    const char format[2] = "c";
                    success = PyArg_Parse (py_return, format, (char *) ret_value);
                    break;
                }
                default:
                  {}
            }
            Py_DECREF (py_return);
            if (success)
                ret_success = true;
            else
                ret_success = false;
        }
    }

    py_error = PyErr_Occurred();
    if (py_error != NULL)
    {
        if (PyErr_GivenExceptionMatches (py_error, PyExc_SyntaxError))
            PyErr_Print ();
        PyErr_Clear();
        ret_success = false;
    }

    return ret_success;
}

bool
ScriptInterpreterPython::ExecuteMultipleLines (const char *in_string)
{
    bool success = false;
    PyObject *py_return = NULL;
    PyObject *mainmod = PyImport_AddModule ("__main__");
    PyObject *globals = PyModule_GetDict (mainmod);
    PyObject *locals = globals;
    PyObject *py_error = NULL;

    if (in_string != NULL)
    {
        struct _node *compiled_node = PyParser_SimpleParseString (in_string, Py_file_input);
        if (compiled_node)
        {
            PyCodeObject *compiled_code = PyNode_Compile (compiled_node, "temp.py");
            if (compiled_code)
            {
                py_return = PyEval_EvalCode (compiled_code, globals, locals);
                if (py_return != NULL)
                {
                    success = true;
                    Py_DECREF (py_return);
                }
            }
        }
    }

    py_error = PyErr_Occurred ();
    if (py_error != NULL)
    {
        if (PyErr_GivenExceptionMatches (py_error, PyExc_SyntaxError))
            PyErr_Print ();
        PyErr_Clear();
        success = false;
    }

    return success;
}

static const char *g_reader_instructions = "Enter your Python command(s). Type 'DONE' to end.";

size_t
ScriptInterpreterPython::GenerateBreakpointOptionsCommandCallback
(
    void *baton, 
    InputReader &reader, 
    InputReaderAction notification,
    const char *bytes, 
    size_t bytes_len
)
{
  static StringList commands_in_progress;

    FILE *out_fh = reader.GetDebugger().GetOutputFileHandle();
    if (out_fh == NULL)
        out_fh = stdout;

    switch (notification)
    {
    case eInputReaderActivate:
        {
            commands_in_progress.Clear();
            if (out_fh)
            {
                ::fprintf (out_fh, "%s\n", g_reader_instructions);
                if (reader.GetPrompt())
                    ::fprintf (out_fh, "%s", reader.GetPrompt());
                ::fflush (out_fh);
            }
        }
        break;

    case eInputReaderDeactivate:
        break;

    case eInputReaderReactivate:
        if (reader.GetPrompt() && out_fh)
        {
            ::fprintf (out_fh, "%s", reader.GetPrompt());
            ::fflush (out_fh);
        }
        break;

    case eInputReaderGotToken:
        {
            std::string temp_string (bytes, bytes_len);
            commands_in_progress.AppendString (temp_string.c_str());
            if (out_fh && !reader.IsDone() && reader.GetPrompt())
            {
                ::fprintf (out_fh, "%s", reader.GetPrompt());
                ::fflush (out_fh);
            }
        }
        break;

    case eInputReaderEndOfFile:
    case eInputReaderInterrupt:
        // Control-c (SIGINT) & control-d both mean finish & exit.
        reader.SetIsDone(true);
        
        // Control-c (SIGINT) ALSO means cancel; do NOT create a breakpoint command.
        if (notification == eInputReaderInterrupt)
            commands_in_progress.Clear();  
        
        // Fall through here...

    case eInputReaderDone:
        {
            BreakpointOptions *bp_options = (BreakpointOptions *)baton;
            std::auto_ptr<BreakpointOptions::CommandData> data_ap(new BreakpointOptions::CommandData());
            data_ap->user_source.AppendList (commands_in_progress);
            if (data_ap.get())
            {
                ScriptInterpreter *interpreter = reader.GetDebugger().GetCommandInterpreter().GetScriptInterpreter();
                if (interpreter)
                {
                    if (interpreter->GenerateBreakpointCommandCallbackData (data_ap->user_source, 
                                                                            data_ap->script_source))
                    {
                        if (data_ap->script_source.GetSize() == 1)
                        {
                            BatonSP baton_sp (new BreakpointOptions::CommandBaton (data_ap.release()));
                            bp_options->SetCallback (ScriptInterpreterPython::BreakpointCallbackFunction, baton_sp);
                        }
                    }
                    else
                        ::fprintf (out_fh, "Warning: No command attached to breakpoint.\n");
                }
                else
                {
                    // FIXME:  Error processing.
                }
            }
        }
        break;
        
    }

    return bytes_len;
}

void
ScriptInterpreterPython::CollectDataForBreakpointCommandCallback (BreakpointOptions *bp_options,
                                                                  CommandReturnObject &result)
{
    Debugger &debugger = m_interpreter.GetDebugger();
    InputReaderSP reader_sp (new InputReader (debugger));

    if (reader_sp)
    {
        Error err = reader_sp->Initialize (
                ScriptInterpreterPython::GenerateBreakpointOptionsCommandCallback,
                bp_options,                 // baton
                eInputReaderGranularityLine, // token size, for feeding data to callback function
                "DONE",                     // end token
                "> ",                       // prompt
                true);                      // echo input
    
        if (err.Success())
            debugger.PushInputReader (reader_sp);
        else
        {
            result.AppendError (err.AsCString());
            result.SetStatus (eReturnStatusFailed);
        }
    }
    else
    {
        result.AppendError("out of memory");
        result.SetStatus (eReturnStatusFailed);
    }
}

// Set a Python one-liner as the callback for the breakpoint.
void
ScriptInterpreterPython::SetBreakpointCommandCallback (BreakpointOptions *bp_options,
                                                       const char *oneliner)
{
    std::auto_ptr<BreakpointOptions::CommandData> data_ap(new BreakpointOptions::CommandData());

    // It's necessary to set both user_source and script_source to the oneliner.
    // The former is used to generate callback description (as in breakpoint command list)
    // while the latter is used for Python to interpret during the actual callback.

    data_ap->user_source.AppendString (oneliner);

    if (GenerateBreakpointCommandCallbackData (data_ap->user_source, data_ap->script_source))
    {
        if (data_ap->script_source.GetSize() == 1)
        {
            BatonSP baton_sp (new BreakpointOptions::CommandBaton (data_ap.release()));
            bp_options->SetCallback (ScriptInterpreterPython::BreakpointCallbackFunction, baton_sp);
        }
    }
    
    return;
}

bool
ScriptInterpreterPython::ExportFunctionDefinitionToInterpreter (StringList &function_def)
{
    // Convert StringList to one long, newline delimited, const char *.
    std::string function_def_string;

    int num_lines = function_def.GetSize();

    for (int i = 0; i < num_lines; ++i)
    {
        function_def_string.append (function_def.GetStringAtIndex(i));
        if (function_def_string.at (function_def_string.length() - 1) != '\n')
            function_def_string.append ("\n");

    }

    return ExecuteMultipleLines (function_def_string.c_str());
}

bool
ScriptInterpreterPython::GenerateBreakpointCommandCallbackData (StringList &user_input, StringList &callback_data)
{
    static int num_created_functions = 0;
    user_input.RemoveBlankLines ();
    int num_lines = user_input.GetSize ();
    StreamString sstr;

    // Check to see if we have any data; if not, just return.
    if (user_input.GetSize() == 0)
        return false;

    // Take what the user wrote, wrap it all up inside one big auto-generated Python function, passing in the
    // frame and breakpoint location as parameters to the function.


    sstr.Printf ("lldb_autogen_python_bp_callback_func_%d", num_created_functions);
    ++num_created_functions;
    std::string auto_generated_function_name = sstr.GetData();

    sstr.Clear();
    StringList auto_generated_function;

    // Create the function name & definition string.

    sstr.Printf ("def %s (frame, bp_loc):", auto_generated_function_name.c_str());
    auto_generated_function.AppendString (sstr.GetData());

    // Wrap everything up inside the function, increasing the indentation.

    for (int i = 0; i < num_lines; ++i)
    {
        sstr.Clear ();
        sstr.Printf ("     %s", user_input.GetStringAtIndex (i));
        auto_generated_function.AppendString (sstr.GetData());
    }

    // Verify that the results are valid Python.

    if (!ExportFunctionDefinitionToInterpreter (auto_generated_function))
    {
        return false;
    }

    // Store the name of the auto-generated function to be called.

    callback_data.AppendString (auto_generated_function_name.c_str());
    return true;
}

bool
ScriptInterpreterPython::BreakpointCallbackFunction 
(
    void *baton,
    StoppointCallbackContext *context,
    user_id_t break_id,
    user_id_t break_loc_id
)
{
    BreakpointOptions::CommandData *bp_option_data = (BreakpointOptions::CommandData *) baton;
    const char *python_function_name = bp_option_data->script_source.GetStringAtIndex (0);
    
    if (python_function_name != NULL 
        && python_function_name[0] != '\0')
    {
        Thread *thread = context->exe_ctx.thread;
        Target *target = context->exe_ctx.target;
        const StackFrameSP stop_frame_sp = thread->GetStackFrameSPForStackFramePtr (context->exe_ctx.frame);
        BreakpointSP breakpoint_sp = target->GetBreakpointByID (break_id);
        const BreakpointLocationSP bp_loc_sp = breakpoint_sp->FindLocationByID (break_loc_id);
        
        SBFrame sb_frame (stop_frame_sp);
        SBBreakpointLocation sb_bp_loc (bp_loc_sp);
        
        if (sb_bp_loc.IsValid() || sb_frame.IsValid())
            return LLDBSWIGPythonBreakpointCallbackFunction (python_function_name, sb_frame, sb_bp_loc);
    }
    // We currently always true so we stop in case anything goes wrong when
    // trying to call the script function
    return true;
}

lldb::thread_result_t
ScriptInterpreterPython::RunEmbeddedPythonInterpreter (lldb::thread_arg_t baton)
{
    ScriptInterpreterPython *script_interpreter = (ScriptInterpreterPython *) baton;
    
    LogSP log (lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_SCRIPT));
    
    if (log)
        log->Printf ("%p ScriptInterpreterPython::RunEmbeddedPythonInterpreter () thread starting...", baton);
    
    char error_str[1024];
    const char *pty_slave_name = script_interpreter->m_embedded_python_pty.GetSlaveName (error_str, sizeof (error_str));
    if (pty_slave_name != NULL)
    {
        StreamString run_string;
        PyRun_SimpleString ("save_stderr = sys.stderr");
        PyRun_SimpleString ("sys.stderr = sys.stdout");
        PyRun_SimpleString ("save_stdin = sys.stdin");
        run_string.Printf ("sys.stdin = open ('%s', 'r')", pty_slave_name);
        PyRun_SimpleString (run_string.GetData());
        
	    // The following call drops into the embedded interpreter loop and stays there until the
	    // user chooses to exit from the Python interpreter.
        script_interpreter->ExecuteOneLine ("run_python_interpreter(ConsoleDict)", NULL);
        
        PyRun_SimpleString ("sys.stdin = save_stdin");
        PyRun_SimpleString ("sys.stderr = save_stderr");
    }
    
    if (script_interpreter->m_embedded_thread_input_reader_sp)
        script_interpreter->m_embedded_thread_input_reader_sp->SetIsDone (true);
    
    script_interpreter->m_embedded_python_pty.CloseSlaveFileDescriptor();
    
    log = lldb_private::GetLogIfAllCategoriesSet (LIBLLDB_LOG_SCRIPT);
    if (log)
        log->Printf ("%p ScriptInterpreterPython::RunEmbeddedPythonInterpreter () thread exiting...", baton);
    

	// Clean up the input reader and make the debugger pop it off the stack.    
    Debugger &debugger = script_interpreter->m_interpreter.GetDebugger();
    const InputReaderSP reader_sp = script_interpreter->m_embedded_thread_input_reader_sp;
    script_interpreter->m_embedded_thread_input_reader_sp.reset();
    debugger.PopInputReader (reader_sp);
        
    return NULL;
}



