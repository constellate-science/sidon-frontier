#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <fstream>
#include <iostream>
#include <limits>
#include <random>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

constexpr std::uint32_t kDimension = 24;
constexpr std::uint32_t kUniverse = 1U << kDimension;
constexpr std::uint32_t kIndexBits = 13;
constexpr std::uint32_t kIndexMask = (1U << kIndexBits) - 1U;
constexpr std::size_t kHashCapacity = 1ULL << 26;
constexpr std::size_t kCandidateCapPerRemovedPoint = 256;

struct PairSumTable {
  std::vector<std::uint64_t> keys = std::vector<std::uint64_t>(kHashCapacity);
  std::vector<std::uint32_t> pairs = std::vector<std::uint32_t>(kHashCapacity);

  static std::uint64_t mix(std::uint64_t value) {
    value ^= value >> 30;
    value *= 0xbf58476d1ce4e5b9ULL;
    value ^= value >> 27;
    value *= 0x94d049bb133111ebULL;
    value ^= value >> 31;
    return value;
  }

  void insert(std::uint64_t code, std::uint32_t left, std::uint32_t right) {
    const std::uint64_t stored = code + 1;
    std::size_t slot = mix(stored) & (kHashCapacity - 1);
    while (keys[slot] != 0) {
      if (keys[slot] == stored) {
        throw std::runtime_error("input witness is not Sidon");
      }
      slot = (slot + 1) & (kHashCapacity - 1);
    }
    keys[slot] = stored;
    pairs[slot] = left | (right << kIndexBits);
  }

  bool find(std::uint64_t code, std::uint32_t& left, std::uint32_t& right) const {
    const std::uint64_t stored = code + 1;
    std::size_t slot = mix(stored) & (kHashCapacity - 1);
    while (keys[slot] != 0) {
      if (keys[slot] == stored) {
        left = pairs[slot] & kIndexMask;
        right = pairs[slot] >> kIndexBits;
        return true;
      }
      slot = (slot + 1) & (kHashCapacity - 1);
    }
    return false;
  }
};

std::vector<std::uint32_t> parse_points(const std::string& path) {
  std::ifstream input(path, std::ios::binary);
  if (!input) throw std::runtime_error("cannot open witness");
  const std::string bytes((std::istreambuf_iterator<char>(input)), {});
  const auto marker = bytes.find("\"points\"");
  if (marker == std::string::npos) throw std::runtime_error("points field missing");
  const auto start = bytes.find('[', marker);
  if (start == std::string::npos) throw std::runtime_error("points array missing");

  std::vector<std::uint32_t> points;
  std::uint32_t value = 0;
  std::uint32_t bits = 0;
  int depth = 0;
  for (std::size_t cursor = start; cursor < bytes.size(); ++cursor) {
    const char ch = bytes[cursor];
    if (ch == '[') {
      ++depth;
      if (depth == 2) {
        value = 0;
        bits = 0;
      }
    } else if (ch == ']') {
      if (depth == 2) {
        if (bits != kDimension) throw std::runtime_error("point has wrong dimension");
        points.push_back(value);
      }
      --depth;
      if (depth == 0) break;
    } else if (depth == 2 && (ch == '0' || ch == '1')) {
      value = (value << 1U) | static_cast<std::uint32_t>(ch - '0');
      ++bits;
    }
  }
  if (points.empty()) throw std::runtime_error("no points parsed");
  return points;
}

struct SumEncoder {
  std::array<std::uint64_t, 3> powers{};
  std::vector<std::uint16_t> byte_sums = std::vector<std::uint16_t>(1U << 16U);

  SumEncoder() {
    powers[0] = 1;
    for (std::size_t i = 1; i < powers.size(); ++i) powers[i] = powers[i - 1] * 6561ULL;
    for (std::uint32_t left = 0; left < 256; ++left) {
      for (std::uint32_t right = 0; right < 256; ++right) {
        std::uint16_t code = 0;
        std::uint16_t power = 1;
        for (std::uint32_t bit = 0; bit < 8; ++bit) {
          code += static_cast<std::uint16_t>(((left >> bit) & 1U) + ((right >> bit) & 1U)) * power;
          power *= 3;
        }
        byte_sums[(left << 8U) | right] = code;
      }
    }
  }

  std::uint64_t operator()(std::uint32_t left, std::uint32_t right) const {
    return byte_sums[((left & 255U) << 8U) | (right & 255U)]
      + powers[1] * byte_sums[((((left >> 8U) & 255U) << 8U) | ((right >> 8U) & 255U))]
      + powers[2] * byte_sums[((((left >> 16U) & 255U) << 8U) | ((right >> 16U) & 255U))];
  }
};

bool triple_contains(const std::array<std::uint32_t, 3>& triple, std::uint32_t value) {
  return triple[0] == value || triple[1] == value || triple[2] == value;
}

void write_witness(const std::string& path,
                   const std::vector<std::uint32_t>& points,
                   std::uint32_t removed_index,
                   std::uint32_t first,
                   std::uint32_t second) {
  std::ofstream output(path, std::ios::binary | std::ios::trunc);
  if (!output) throw std::runtime_error("cannot write candidate witness");
  output << "{\"kind\":\"sidon\",\"n\":24,\"points\":[";
  bool comma = false;
  auto emit = [&](std::uint32_t point) {
    if (comma) output << ',';
    comma = true;
    output << '[';
    for (int bit = 23; bit >= 0; --bit) {
      if (bit != 23) output << ',';
      output << ((point >> bit) & 1U);
    }
    output << ']';
  };
  for (std::uint32_t i = 0; i < points.size(); ++i) {
    if (i != removed_index) emit(points[i]);
  }
  emit(first);
  emit(second);
  output << "],\"claimed_size\":" << (points.size() + 1)
         << ",\"claim\":\"There exists a Sidon subset of {0,1}^24 with at least 7193 elements.\"}\n";
}

}  // namespace

int main(int argc, char** argv) {
  try {
    if (argc != 5) {
      std::cerr << "usage: sidon_exchange_benchmark WITNESS SAMPLE_COUNT SEED OUTPUT\n";
      return 2;
    }
    const std::string witness_path = argv[1];
    const std::uint64_t sample_count = std::stoull(argv[2]);
    const std::uint64_t seed = std::stoull(argv[3]);
    const std::string output_path = argv[4];
    const auto started = std::chrono::steady_clock::now();

    const auto points = parse_points(witness_path);
    if (points.size() >= (1U << kIndexBits)) throw std::runtime_error("too many points");
    std::vector<std::uint8_t> present(kUniverse);
    for (const auto point : points) {
      if (present[point]) throw std::runtime_error("duplicate input point");
      present[point] = 1;
    }

    const SumEncoder encode;
    PairSumTable sums;
    std::uint64_t pair_count = 0;
    for (std::uint32_t i = 0; i < points.size(); ++i) {
      for (std::uint32_t j = i; j < points.size(); ++j) {
        sums.insert(encode(points[i], points[j]), i, j);
        ++pair_count;
      }
    }
    const auto indexed_at = std::chrono::steady_clock::now();

    std::vector<std::vector<std::uint32_t>> candidates(points.size());
    std::mt19937_64 random(seed);
    std::uint64_t sampled = 0;
    std::uint64_t fully_scanned = 0;
    std::uint64_t single_removal_candidates = 0;
    while (sampled < sample_count) {
      const std::uint32_t candidate = static_cast<std::uint32_t>(random()) & (kUniverse - 1U);
      if (present[candidate]) continue;
      ++sampled;

      std::array<std::uint32_t, 3> intersection{};
      std::size_t intersection_size = 0;
      bool saw_collision = false;
      for (std::uint32_t a = 0; a < points.size(); ++a) {
        std::uint32_t b = 0;
        std::uint32_t c = 0;
        if (!sums.find(encode(candidate, points[a]), b, c)) continue;
        std::array<std::uint32_t, 3> triple{a, b, c};
        std::sort(triple.begin(), triple.end());
        saw_collision = true;
        if (intersection_size == 0) {
          intersection = triple;
          intersection_size = static_cast<std::size_t>(std::unique(intersection.begin(), intersection.end()) - intersection.begin());
        } else {
          std::array<std::uint32_t, 3> filtered{};
          std::size_t count = 0;
          for (std::size_t i = 0; i < intersection_size; ++i) {
            if (triple_contains(triple, intersection[i])) filtered[count++] = intersection[i];
          }
          intersection = filtered;
          intersection_size = count;
          if (intersection_size == 0) break;
        }
      }
      if (!saw_collision) throw std::runtime_error("found a direct one-point extension");
      if (intersection_size == 0) continue;
      ++fully_scanned;
      for (std::size_t i = 0; i < intersection_size; ++i) {
        auto& bucket = candidates[intersection[i]];
        if (bucket.size() < kCandidateCapPerRemovedPoint) {
          bucket.push_back(candidate);
          ++single_removal_candidates;
        }
      }
    }
    const auto sampled_at = std::chrono::steady_clock::now();

    std::uint64_t pairs_tested = 0;
    for (std::uint32_t removed = 0; removed < candidates.size(); ++removed) {
      auto& bucket = candidates[removed];
      std::sort(bucket.begin(), bucket.end());
      bucket.erase(std::unique(bucket.begin(), bucket.end()), bucket.end());
      for (std::size_t i = 0; i < bucket.size(); ++i) {
        for (std::size_t j = i + 1; j < bucket.size(); ++j) {
          ++pairs_tested;
          const std::uint32_t first = bucket[i];
          const std::uint32_t second = bucket[j];
          std::uint32_t left = 0;
          std::uint32_t right = 0;
          if (sums.find(encode(first, second), left, right) && left != removed && right != removed) continue;

          const std::uint32_t first_only = first & ~second;
          const std::uint32_t second_only = second & ~first;
          const std::uint32_t difference = first ^ second;
          bool cross_collision = false;
          for (std::uint32_t a = 0; a < points.size(); ++a) {
            if (a == removed) continue;
            const std::uint32_t point = points[a];
            if ((point & first_only) != 0 || (point & second_only) != second_only) continue;
            const std::uint32_t partner = point ^ difference;
            if (present[partner] && partner != points[removed]) {
              cross_collision = true;
              break;
            }
          }
          if (cross_collision) continue;

          write_witness(output_path, points, removed, first, second);
          const auto finished = std::chrono::steady_clock::now();
          std::cout << "status=found\n"
                    << "input_size=" << points.size() << "\n"
                    << "output_size=" << (points.size() + 1) << "\n"
                    << "removed_index=" << removed << "\n"
                    << "removed_point=" << points[removed] << "\n"
                    << "added_first=" << first << "\n"
                    << "added_second=" << second << "\n"
                    << "pair_sums=" << pair_count << "\n"
                    << "sampled=" << sampled << "\n"
                    << "fully_scanned=" << fully_scanned << "\n"
                    << "single_removal_candidates=" << single_removal_candidates << "\n"
                    << "candidate_pairs_tested=" << pairs_tested << "\n"
                    << "index_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(indexed_at - started).count() << "\n"
                    << "sample_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(sampled_at - indexed_at).count() << "\n"
                    << "total_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count() << "\n";
          return 0;
        }
      }
    }

    const auto finished = std::chrono::steady_clock::now();
    std::cout << "status=not_found\n"
              << "input_size=" << points.size() << "\n"
              << "pair_sums=" << pair_count << "\n"
              << "sampled=" << sampled << "\n"
              << "fully_scanned=" << fully_scanned << "\n"
              << "single_removal_candidates=" << single_removal_candidates << "\n"
              << "candidate_pairs_tested=" << pairs_tested << "\n"
              << "index_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(indexed_at - started).count() << "\n"
              << "sample_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(sampled_at - indexed_at).count() << "\n"
              << "total_ms=" << std::chrono::duration_cast<std::chrono::milliseconds>(finished - started).count() << "\n";
    return 1;
  } catch (const std::exception& error) {
    std::cerr << "error=" << error.what() << '\n';
    return 2;
  }
}
