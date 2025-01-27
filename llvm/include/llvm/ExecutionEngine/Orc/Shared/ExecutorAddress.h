//===------ ExecutorAddress.h - Executing process address -------*- C++ -*-===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//
//
// Represents an address in the executing program.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_EXECUTIONENGINE_ORC_SHARED_EXECUTORADDRESS_H
#define LLVM_EXECUTIONENGINE_ORC_SHARED_EXECUTORADDRESS_H

#include "llvm/ExecutionEngine/Orc/Shared/SimplePackedSerialization.h"

#include <cassert>
#include <type_traits>

namespace llvm {
namespace orc {

/// Represents the difference between two addresses in the executor process.
class ExecutorAddrDiff {
public:
  ExecutorAddrDiff() = default;
  explicit ExecutorAddrDiff(uint64_t Value) : Value(Value) {}

  uint64_t getValue() const { return Value; }

private:
  int64_t Value = 0;
};

/// Represents an address in the executor process.
class ExecutorAddr {
public:
  ExecutorAddr() = default;
  explicit ExecutorAddr(uint64_t Addr) : Addr(Addr) {}

  /// Create an ExecutorAddr from the given pointer.
  /// Warning: This should only be used when JITing in-process.
  template <typename T> static ExecutorAddr fromPtr(T *Value) {
    return ExecutorAddr(
        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(Value)));
  }

  /// Cast this ExecutorAddr to a pointer of the given type.
  /// Warning: This should only be used when JITing in-process.
  template <typename T> T toPtr() const {
    static_assert(std::is_pointer<T>::value, "T must be a pointer type");
    uintptr_t IntPtr = static_cast<uintptr_t>(Addr);
    assert(IntPtr == Addr && "ExecutorAddr value out of range for uintptr_t");
    return reinterpret_cast<T>(IntPtr);
  }

  uint64_t getValue() const { return Addr; }
  void setValue(uint64_t Addr) { this->Addr = Addr; }
  bool isNull() const { return Addr == 0; }

  explicit operator bool() const { return Addr != 0; }

  friend bool operator==(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr == RHS.Addr;
  }

  friend bool operator!=(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr != RHS.Addr;
  }

  friend bool operator<(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr < RHS.Addr;
  }

  friend bool operator<=(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr <= RHS.Addr;
  }

  friend bool operator>(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr > RHS.Addr;
  }

  friend bool operator>=(const ExecutorAddr &LHS, const ExecutorAddr &RHS) {
    return LHS.Addr >= RHS.Addr;
  }

  ExecutorAddr &operator++() {
    ++Addr;
    return *this;
  }
  ExecutorAddr &operator--() {
    --Addr;
    return *this;
  }
  ExecutorAddr operator++(int) { return ExecutorAddr(Addr++); }
  ExecutorAddr operator--(int) { return ExecutorAddr(Addr++); }

  ExecutorAddr &operator+=(const ExecutorAddrDiff Delta) {
    Addr += Delta.getValue();
    return *this;
  }

  ExecutorAddr &operator-=(const ExecutorAddrDiff Delta) {
    Addr -= Delta.getValue();
    return *this;
  }

private:
  uint64_t Addr = 0;
};

/// Subtracting two addresses yields an offset.
inline ExecutorAddrDiff operator-(const ExecutorAddr &LHS,
                                  const ExecutorAddr &RHS) {
  return ExecutorAddrDiff(LHS.getValue() - RHS.getValue());
}

/// Adding an offset and an address yields an address.
inline ExecutorAddr operator+(const ExecutorAddr &LHS,
                              const ExecutorAddrDiff &RHS) {
  return ExecutorAddr(LHS.getValue() + RHS.getValue());
}

/// Adding an address and an offset yields an address.
inline ExecutorAddr operator+(const ExecutorAddrDiff &LHS,
                              const ExecutorAddr &RHS) {
  return ExecutorAddr(LHS.getValue() + RHS.getValue());
}

/// Represents an address range in the exceutor process.
struct ExecutorAddrRange {
  ExecutorAddrRange() = default;
  ExecutorAddrRange(ExecutorAddr Start, ExecutorAddr End)
      : Start(Start), End(End) {}

  bool empty() const { return Start == End; }
  ExecutorAddrDiff size() const { return End - Start; }

  ExecutorAddr Start;
  ExecutorAddr End;
};

namespace shared {

/// SPS serializatior for ExecutorAddr.
template <> class SPSSerializationTraits<SPSExecutorAddr, ExecutorAddr> {
public:
  static size_t size(const ExecutorAddr &EA) {
    return SPSArgList<uint64_t>::size(EA.getValue());
  }

  static bool serialize(SPSOutputBuffer &BOB, const ExecutorAddr &EA) {
    return SPSArgList<uint64_t>::serialize(BOB, EA.getValue());
  }

  static bool deserialize(SPSInputBuffer &BIB, ExecutorAddr &EA) {
    uint64_t Tmp;
    if (!SPSArgList<uint64_t>::deserialize(BIB, Tmp))
      return false;
    EA = ExecutorAddr(Tmp);
    return true;
  }
};

using SPSExecutorAddrRange = SPSTuple<SPSExecutorAddr, SPSExecutorAddr>;

/// Serialization traits for address ranges.
template <>
class SPSSerializationTraits<SPSExecutorAddrRange, ExecutorAddrRange> {
public:
  static size_t size(const ExecutorAddrRange &Value) {
    return SPSArgList<SPSExecutorAddr, SPSExecutorAddr>::size(Value.Start,
                                                              Value.End);
  }

  static bool serialize(SPSOutputBuffer &BOB, const ExecutorAddrRange &Value) {
    return SPSArgList<SPSExecutorAddr, SPSExecutorAddr>::serialize(
        BOB, Value.Start, Value.End);
  }

  static bool deserialize(SPSInputBuffer &BIB, ExecutorAddrRange &Value) {
    return SPSArgList<SPSExecutorAddr, SPSExecutorAddr>::deserialize(
        BIB, Value.Start, Value.End);
  }
};

using SPSExecutorAddrRangeSequence = SPSSequence<SPSExecutorAddrRange>;

} // End namespace shared.
} // End namespace orc.
} // End namespace llvm.

#endif // LLVM_EXECUTIONENGINE_ORC_SHARED_EXECUTORADDRESS_H
