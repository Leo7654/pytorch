#pragma once

#include <torch/csrc/jit/ir/ir.h>
#include <torch/csrc/jit/runtime/static/impl.h>

namespace torch {
namespace jit {
enum class Strategy {
  NAIVE = 0,
  LINEAR_SCAN,
  GREEDY_BY_SIZE,
  GREEDY_BY_SIZE_WITH_FIRST_GAP,
  GREEDY_BY_LONGEST_AND_SIZE,
  GREEDY_BY_BREADTH,
};

inline const char* toString(Strategy s) {
  switch (s) {
    case Strategy::NAIVE:
      return "NAIVE";
    case Strategy::LINEAR_SCAN:
      return "LINEAR_SCAN";
    case Strategy::GREEDY_BY_SIZE:
      return "GREEDY_BY_SIZE";
    case Strategy::GREEDY_BY_SIZE_WITH_FIRST_GAP:
      return "GREEDY_BY_SIZE_WITH_FIRST_GAP";
    case Strategy::GREEDY_BY_LONGEST_AND_SIZE:
      return "GREEDY_BY_LONGEST_AND_SIZE";
    case Strategy::GREEDY_BY_BREADTH:
      return "GREEDY_BY_BREADTH";
    default:
      return "UNKNOWN STRATEGY";
  }
}

inline std::ostream& operator<<(std::ostream& str, Strategy rhs) {
  return str << toString(rhs);
}

typedef struct MemRegion {
  size_t offset;
  size_t size;
} MemRegion;

inline std::ostream& operator<<(std::ostream& str, MemRegion reg) {
  return str << "{offset: " << reg.offset << ", size: " << reg.size << "}";
}

inline bool operator==(const MemRegion& lhs, const MemRegion& rhs) {
  return lhs.offset == rhs.offset && lhs.size == rhs.size;
}

struct regionSizeCmp {
  bool operator()(const MemRegion& reg1, const MemRegion& reg2) const {
    return reg1.size == reg2.size ? reg1.offset < reg2.offset
                                  : reg1.size < reg2.size;
  }
};

struct regionOffsetCmp {
  bool operator()(const MemRegion& reg1, const MemRegion& reg2) const {
    return reg1.offset == reg2.offset ? reg1.size < reg2.size
                                      : reg1.offset < reg2.offset;
  }
};

bool overlapMemRegion(const MemRegion& reg1, const MemRegion& reg2);

struct UniqueLiveRange {
  LiveRange lvr;
  std::string id;
};

bool overlapLiveRange(
    const UniqueLiveRange& ulvr1,
    const UniqueLiveRange& ulvr2);

inline std::ostream& operator<<(std::ostream& str, UniqueLiveRange rhs) {
  return str << "{id: " << rhs.id << ", lvr: " << rhs.lvr << "}";
}

inline bool operator==(const UniqueLiveRange lhs, const UniqueLiveRange rhs) {
  return lhs.lvr == rhs.lvr && lhs.id == rhs.id;
}

struct liveRangeStartCmp {
  bool operator()(const UniqueLiveRange& u1, const UniqueLiveRange& u2) const {
    return u1.lvr.begin == u2.lvr.begin
        ? (u1.lvr.end == u2.lvr.end ? u1.id < u2.id : u1.lvr.end < u2.lvr.end)
        : u1.lvr.begin < u2.lvr.begin;
  }
};

struct liveRangeEndCmp {
  bool operator()(const UniqueLiveRange& u1, const UniqueLiveRange& u2) const {
    return u1.lvr.end == u2.lvr.end
        ? (u1.lvr.begin == u2.lvr.begin ? u1.id < u2.id
                                        : u1.lvr.begin < u2.lvr.begin)
        : u1.lvr.end < u2.lvr.end;
  }
};

template <typename Value>
using SortedLiveRangeMap = std::map<UniqueLiveRange, Value, liveRangeStartCmp>;
struct TORCH_API MemAllocation {
  UniqueLiveRange ulvr;
  MemRegion reg;
};

inline std::ostream& operator<<(std::ostream& str, MemAllocation rhs) {
  return str << rhs.ulvr << ", " << rhs.reg;
}

inline bool operator==(const MemAllocation lhs, const MemAllocation rhs) {
  return lhs.ulvr == rhs.ulvr && lhs.reg == rhs.reg;
}

inline bool valid_add(size_t a, size_t b) {
  size_t _carry = 0;
  return !__builtin_add_overflow(a, b, &_carry);
}

inline bool valid_sub(size_t a, size_t b) {
  size_t _carry = 0;
  return !__builtin_sub_overflow(a, b, &_carry);
}

c10::optional<size_t> computeStorageSize(const Value& value);

TORCH_API bool hasOutVariant(Node* node);

TORCH_API void planMemory(const std::shared_ptr<Graph>&, Strategy);

} // namespace jit
} // namespace torch

namespace std {
template <>
struct hash<torch::jit::MemRegion> {
  size_t operator()(torch::jit::MemRegion const& reg) const {
    return std::hash<size_t>()(reg.offset) ^
        (std::hash<size_t>()(reg.size) << 1);
  }
};

template <>
struct hash<torch::jit::UniqueLiveRange> {
  size_t operator()(torch::jit::UniqueLiveRange const& ulvr) const {
    return std::hash<torch::jit::LiveRange>()(ulvr.lvr) ^
        (std::hash<string>()(ulvr.id));
  }
};

} // namespace std
