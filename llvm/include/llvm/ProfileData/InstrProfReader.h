//=-- InstrProfReader.h - Instrumented profiling readers ----------*- C++ -*-=//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file contains support for reading profiling data for instrumentation
// based PGO and coverage.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_PROFILEDATA_INSTRPROF_READER_H_
#define LLVM_PROFILEDATA_INSTRPROF_READER_H_

#include "llvm/ADT/ArrayRef.h"
#include "llvm/ADT/StringExtras.h"
#include "llvm/ProfileData/InstrProf.h"
#include "llvm/Support/LineIterator.h"
#include "llvm/Support/MemoryBuffer.h"
#include "llvm/Support/EndianStream.h"
#include "llvm/Support/OnDiskHashTable.h"

#include <iterator>

namespace llvm {

class InstrProfReader;

/// Profiling information for a single function.
struct InstrProfRecord {
  InstrProfRecord() {}
  InstrProfRecord(StringRef Name, uint64_t Hash, ArrayRef<uint64_t> Counts)
      : Name(Name), Hash(Hash), Counts(Counts) {}
  StringRef Name;
  uint64_t Hash;
  ArrayRef<uint64_t> Counts;
};

/// A file format agnostic iterator over profiling data.
class InstrProfIterator : public std::iterator<std::input_iterator_tag,
                                               InstrProfRecord> {
  InstrProfReader *Reader;
  InstrProfRecord Record;

  void Increment();
public:
  InstrProfIterator() : Reader(nullptr) {}
  InstrProfIterator(InstrProfReader *Reader) : Reader(Reader) { Increment(); }

  InstrProfIterator &operator++() { Increment(); return *this; }
  bool operator==(const InstrProfIterator &RHS) { return Reader == RHS.Reader; }
  bool operator!=(const InstrProfIterator &RHS) { return Reader != RHS.Reader; }
  InstrProfRecord &operator*() { return Record; }
  InstrProfRecord *operator->() { return &Record; }
};

/// Base class and interface for reading profiling data of any known instrprof
/// format. Provides an iterator over InstrProfRecords.
class InstrProfReader {
  error_code LastError;
public:
  InstrProfReader() : LastError(instrprof_error::success) {}
  virtual ~InstrProfReader() {}

  /// Read the header.  Required before reading first record.
  virtual error_code readHeader() = 0;
  /// Read a single record.
  virtual error_code readNextRecord(InstrProfRecord &Record) = 0;
  /// Iterator over profile data.
  InstrProfIterator begin() { return InstrProfIterator(this); }
  InstrProfIterator end() { return InstrProfIterator(); }

protected:
  /// Set the current error_code and return same.
  error_code error(error_code EC) {
    LastError = EC;
    return EC;
  }

  /// Clear the current error code and return a successful one.
  error_code success() { return error(instrprof_error::success); }

public:
  /// Return true if the reader has finished reading the profile data.
  bool isEOF() { return LastError == instrprof_error::eof; }
  /// Return true if the reader encountered an error reading profiling data.
  bool hasError() { return LastError && !isEOF(); }
  /// Get the current error code.
  error_code getError() { return LastError; }

  /// Factory method to create an appropriately typed reader for the given
  /// instrprof file.
  static error_code create(std::string Path,
                           std::unique_ptr<InstrProfReader> &Result);
};

/// Reader for the simple text based instrprof format.
///
/// This format is a simple text format that's suitable for test data. Records
/// are separated by one or more blank lines, and record fields are separated by
/// new lines.
///
/// Each record consists of a function name, a function hash, a number of
/// counters, and then each counter value, in that order.
class TextInstrProfReader : public InstrProfReader {
private:
  /// The profile data file contents.
  std::unique_ptr<MemoryBuffer> DataBuffer;
  /// Iterator over the profile data.
  line_iterator Line;
  /// The current set of counter values.
  std::vector<uint64_t> Counts;

  TextInstrProfReader(const TextInstrProfReader &) LLVM_DELETED_FUNCTION;
  TextInstrProfReader &operator=(const TextInstrProfReader &)
    LLVM_DELETED_FUNCTION;
public:
  TextInstrProfReader(std::unique_ptr<MemoryBuffer> DataBuffer_)
      : DataBuffer(std::move(DataBuffer_)), Line(*DataBuffer, '#') {}

  /// Read the header.
  error_code readHeader() override { return success(); }
  /// Read a single record.
  error_code readNextRecord(InstrProfRecord &Record) override;
};

/// Reader for the raw instrprof binary format from runtime.
///
/// This format is a raw memory dump of the instrumentation-baed profiling data
/// from the runtime.  It has no index.
///
/// Templated on the unsigned type whose size matches pointers on the platform
/// that wrote the profile.
template <class IntPtrT>
class RawInstrProfReader : public InstrProfReader {
private:
  /// The profile data file contents.
  std::unique_ptr<MemoryBuffer> DataBuffer;
  /// The current set of counter values.
  std::vector<uint64_t> Counts;
  struct ProfileData {
    const uint32_t NameSize;
    const uint32_t NumCounters;
    const uint64_t FuncHash;
    const IntPtrT NamePtr;
    const IntPtrT CounterPtr;
  };
  struct RawHeader {
    const uint64_t Magic;
    const uint64_t Version;
    const uint64_t DataSize;
    const uint64_t CountersSize;
    const uint64_t NamesSize;
    const uint64_t CountersDelta;
    const uint64_t NamesDelta;
  };

  bool ShouldSwapBytes;
  uint64_t CountersDelta;
  uint64_t NamesDelta;
  const ProfileData *Data;
  const ProfileData *DataEnd;
  const uint64_t *CountersStart;
  const char *NamesStart;

  RawInstrProfReader(const TextInstrProfReader &) LLVM_DELETED_FUNCTION;
  RawInstrProfReader &operator=(const TextInstrProfReader &)
    LLVM_DELETED_FUNCTION;
public:
  RawInstrProfReader(std::unique_ptr<MemoryBuffer> DataBuffer)
      : DataBuffer(std::move(DataBuffer)) { }

  static bool hasFormat(const MemoryBuffer &DataBuffer);
  error_code readHeader() override;
  error_code readNextRecord(InstrProfRecord &Record) override;

private:
  error_code readHeader(const RawHeader &Header);
  template <class IntT>
  IntT swap(IntT Int) const {
    return ShouldSwapBytes ? sys::SwapByteOrder(Int) : Int;
  }
  const uint64_t *getCounter(IntPtrT CounterPtr) const {
    ptrdiff_t Offset = (swap(CounterPtr) - CountersDelta) / sizeof(uint64_t);
    return CountersStart + Offset;
  }
  const char *getName(IntPtrT NamePtr) const {
    ptrdiff_t Offset = (swap(NamePtr) - NamesDelta) / sizeof(char);
    return NamesStart + Offset;
  }
};

typedef RawInstrProfReader<uint32_t> RawInstrProfReader32;
typedef RawInstrProfReader<uint64_t> RawInstrProfReader64;

namespace IndexedInstrProf {
enum class HashT : uint32_t;
uint64_t ComputeHash(HashT Type, StringRef K);
}

/// Trait for lookups into the on-disk hash table for the binary instrprof
/// format.
class InstrProfLookupTrait {
  std::vector<uint64_t> CountBuffer;
  IndexedInstrProf::HashT HashType;
public:
  InstrProfLookupTrait(IndexedInstrProf::HashT HashType) : HashType(HashType) {}

  typedef InstrProfRecord data_type;
  typedef StringRef internal_key_type;
  typedef StringRef external_key_type;
  typedef uint64_t hash_value_type;
  typedef uint64_t offset_type;

  static bool EqualKey(StringRef A, StringRef B) { return A == B; }
  static StringRef GetInternalKey(StringRef K) { return K; }

  hash_value_type ComputeHash(StringRef K) {
    return IndexedInstrProf::ComputeHash(HashType, K);
  }

  static std::pair<offset_type, offset_type>
  ReadKeyDataLength(const unsigned char *&D) {
    using namespace support;
    return std::make_pair(endian::readNext<offset_type, little, unaligned>(D),
                          endian::readNext<offset_type, little, unaligned>(D));
  }

  StringRef ReadKey(const unsigned char *D, unsigned N) {
    return StringRef((const char *)D, N);
  }

  InstrProfRecord ReadData(StringRef K, const unsigned char *D, unsigned N) {
    if (N < 2 * sizeof(uint64_t) || N % sizeof(uint64_t)) {
      // The data is corrupt, don't try to read it.
      CountBuffer.clear();
      return InstrProfRecord("", 0, CountBuffer);
    }

    using namespace support;

    // The first stored value is the hash.
    uint64_t Hash = endian::readNext<uint64_t, little, unaligned>(D);
    // Each counter follows.
    unsigned NumCounters = N / sizeof(uint64_t) - 1;
    CountBuffer.clear();
    CountBuffer.reserve(NumCounters - 1);
    for (unsigned I = 0; I < NumCounters; ++I)
      CountBuffer.push_back(endian::readNext<uint64_t, little, unaligned>(D));

    return InstrProfRecord(K, Hash, CountBuffer);
  }
};
typedef OnDiskIterableChainedHashTable<InstrProfLookupTrait>
    InstrProfReaderIndex;

/// Reader for the indexed binary instrprof format.
class IndexedInstrProfReader : public InstrProfReader {
private:
  /// The profile data file contents.
  std::unique_ptr<MemoryBuffer> DataBuffer;
  /// The index into the profile data.
  std::unique_ptr<InstrProfReaderIndex> Index;
  /// Iterator over the profile data.
  InstrProfReaderIndex::data_iterator RecordIterator;
  /// The maximal execution count among all fucntions.
  uint64_t MaxFunctionCount;

  IndexedInstrProfReader(const IndexedInstrProfReader &) LLVM_DELETED_FUNCTION;
  IndexedInstrProfReader &operator=(const IndexedInstrProfReader &)
    LLVM_DELETED_FUNCTION;
public:
  IndexedInstrProfReader(std::unique_ptr<MemoryBuffer> DataBuffer)
      : DataBuffer(std::move(DataBuffer)), Index(nullptr),
        RecordIterator(InstrProfReaderIndex::data_iterator()) {}

  /// Return true if the given buffer is in an indexed instrprof format.
  static bool hasFormat(const MemoryBuffer &DataBuffer);

  /// Read the file header.
  error_code readHeader() override;
  /// Read a single record.
  error_code readNextRecord(InstrProfRecord &Record) override;

  /// Fill Counts with the profile data for the given function name.
  error_code getFunctionCounts(StringRef FuncName, uint64_t &FuncHash,
                               std::vector<uint64_t> &Counts);
  /// Return the maximum of all known function counts.
  uint64_t getMaximumFunctionCount() { return MaxFunctionCount; }

  /// Factory method to create an indexed reader.
  static error_code create(std::string Path,
                           std::unique_ptr<IndexedInstrProfReader> &Result);
};

} // end namespace llvm

#endif // LLVM_PROFILEDATA_INSTRPROF_READER_H_
