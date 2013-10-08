"""
Example data formatters for strings represented as (pointer,length) pairs
encoded in UTF8/16/32 for use with the LLDB debugger

To use in your projects, tweak the children names as appropriate for your data structures
and use as summaries for your data types

part of The LLVM Compiler Infrastructure
This file is distributed under the University of Illinois Open Source
License. See LICENSE.TXT for details.
"""

import lldb
def utf8_summary(value,unused):
	pointer = value.GetChildMemberWithName("first").GetValueAsUnsigned(0)
	length = value.GetChildMemberWithName("second").GetValueAsUnsigned(0)
	if pointer == 0:
		return False
	if length == 0:
		return '""'
	error = lldb.SBError()
	string_data = value.process.ReadMemory(pointer, length, error)
	return '"%s"' % (string_data) # utf8 is safe to emit as-is on OSX

def utf16_summary(value,unused):
	pointer = value.GetChildMemberWithName("first").GetValueAsUnsigned(0)
	length = value.GetChildMemberWithName("second").GetValueAsUnsigned(0)
	# assume length is in bytes - if in UTF16 chars, just multiply by 2
	if pointer == 0:
		return False
	if length == 0:
		return '""'
	error = lldb.SBError()
	string_data = value.process.ReadMemory(pointer, length, error)
	return '"%s"' % (string_data.decode('utf-16').encode('utf-8')) # utf8 is safe to emit as-is on OSX

def utf32_summary(value,unused):
	pointer = value.GetChildMemberWithName("first").GetValueAsUnsigned(0)
	length = value.GetChildMemberWithName("second").GetValueAsUnsigned(0)
	# assume length is in bytes - if in UTF32 chars, just multiply by 4
	if pointer == 0:
		return False
	if length == 0:
		return '""'
	error = lldb.SBError()
	string_data = value.process.ReadMemory(pointer, length, error)
	return '"%s"' % (string_data.decode('utf-32').encode('utf-8')) # utf8 is safe to emit as-is on OSX

