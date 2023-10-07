#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cctype>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>

#include "log.hh"

namespace wordy_witch {

#pragma region precomputing

constexpr int WORD_SIZE = 5;

static constexpr int ALPHABET_SIZE = 1 << 7;
static constexpr int VERDICT_VALUE_BLACK = 0;
static constexpr int VERDICT_VALUE_YELLOW = 1;
static constexpr int VERDICT_VALUE_GREEN = 2;

int judge(const char* guess, const char* target) {
  int target_letter_counts[ALPHABET_SIZE] = {};
  int verdict_tiles[WORD_SIZE] = {};
  for (int i = 0; i < WORD_SIZE; i++) {
    if (guess[i] == target[i]) {
      verdict_tiles[i] = VERDICT_VALUE_GREEN;
    } else {
      target_letter_counts[target[i]]++;
    }
  }
  for (int i = 0; i < WORD_SIZE; i++) {
    if (guess[i] != target[i] && target_letter_counts[guess[i]] > 0) {
      verdict_tiles[i] = VERDICT_VALUE_YELLOW;
      target_letter_counts[guess[i]]--;
    }
  }

  int verdict = 0;
  for (int i = 0; i < WORD_SIZE; i++) {
    verdict = verdict * 3 + verdict_tiles[i];
  }
  return verdict;
}

static constexpr char VERDICT_TILES[] = {'-', '^', '#'};

std::string format_verdict(int verdict) {
  std::string tiles(WORD_SIZE, '\0');
  for (int i = WORD_SIZE - 1; i >= 0; i--) {
    tiles[i] = VERDICT_TILES[verdict % 3];
    verdict /= 3;
  }
  return tiles;
}

bool check_is_hard_mode_valid(const char* prev_guess, int prev_verdict,
                              const char* candidate_guess) {
  int verdict_tiles[WORD_SIZE];
  for (int i = WORD_SIZE - 1; i >= 0; i--) {
    verdict_tiles[i] = prev_verdict % 3;
    prev_verdict /= 3;
  }

  int letter_counts[ALPHABET_SIZE] = {};
  for (int i = 0; i < WORD_SIZE; i++) {
    if (verdict_tiles[i] != VERDICT_VALUE_BLACK) {
      letter_counts[prev_guess[i]]++;
      if (verdict_tiles[i] == VERDICT_VALUE_GREEN) {
        if (candidate_guess[i] != prev_guess[i]) {
          return false;
        }
      }
    }
    letter_counts[candidate_guess[i]]--;
  }
  for (int i = 0; i < WORD_SIZE; i++) {
    if (letter_counts[prev_guess[i]] > 0) {
      return false;
    }
  }
  return true;
}

constexpr int MAX_BANK_SIZE = 1 << 14;
constexpr int NUM_VERDICTS = 243;

struct word_bank {
  char words[MAX_BANK_SIZE][WORD_SIZE + 1];
  int num_words;
  int num_targets;

  /* `verdict[guess][target]` => `judge(guess, target)` */
  uint8_t verdicts[MAX_BANK_SIZE][MAX_BANK_SIZE];
  /*
    `hard_mode_valid_candidates[prev_guess][prev_verdict][candidate_guess_verdict]`
    => under hard mode, whether some candidate word with verdict
    `judge(prev_guess, candidate_word)` may be used as the next guess if
    prev_verdict (i.e. `judge(prev_guess, target)`) was given
  */
  std::bitset<NUM_VERDICTS> hard_mode_valid_candidates[MAX_BANK_SIZE]
                                                      [NUM_VERDICTS];
};

enum struct word_bank_guesses_inclusion : int {
  TARGETS_ONLY,
  COMMON_WORDS,
  ALL_WORDS,
};

static void clear_bank_specific_caches();

void load_bank(word_bank& out_bank, std::filesystem::path dict_path,
               word_bank_guesses_inclusion guesses_inclusion) {
  clear_bank_specific_caches();

  const auto read_bank = [&out_bank, dict_path, guesses_inclusion]() -> void {
    out_bank.num_words = 0;
    out_bank.num_targets = 0;
    std::ifstream target_file(dict_path / "targets.txt");
    while (true) {
      target_file >> out_bank.words[out_bank.num_words];
      if (target_file.fail()) {
        break;
      }
      out_bank.num_words++;
      out_bank.num_targets++;
    }

    if (guesses_inclusion == word_bank_guesses_inclusion::TARGETS_ONLY) {
      return;
    }
    std::ifstream common_guesses_file(dict_path / "common_guesses.txt");
    while (true) {
      common_guesses_file >> out_bank.words[out_bank.num_words];
      if (common_guesses_file.fail()) {
        break;
      }
      out_bank.num_words++;
    }

    if (guesses_inclusion != word_bank_guesses_inclusion::ALL_WORDS) {
      return;
    }
    std::ifstream uncommon_guesses_file(dict_path / "uncommon_guesses.txt");
    while (true) {
      uncommon_guesses_file >> out_bank.words[out_bank.num_words];
      if (uncommon_guesses_file.fail()) {
        break;
      }
      out_bank.num_words++;
    }
  };
  read_bank();

  const auto transform_bank_words_to_upper = [&out_bank]() -> void {
    for (int i = 0; i < out_bank.num_words; i++) {
      for (int j = 0; j < WORD_SIZE; j++) {
        out_bank.words[i][j] = std::toupper(out_bank.words[i][j]);
      }
    }
  };
  transform_bank_words_to_upper();

  const auto precompute_judge_data = [&out_bank]() -> void {
    for (int i = 0; i < out_bank.num_words; i++) {
      if (std::popcount(static_cast<unsigned>(i)) == 1) {
        WORDY_WITCH_TRACE("Precomputing judge data", i, out_bank.num_words);
      }
      std::optional<int> sample_next_guesses[NUM_VERDICTS] = {};
      for (int j = 0; j < out_bank.num_words; j++) {
        int verdict = judge(out_bank.words[i], out_bank.words[j]);
        out_bank.verdicts[i][j] = verdict;
        sample_next_guesses[verdict] = j;
      }
      for (int prev_verdict = 0; prev_verdict < NUM_VERDICTS; prev_verdict++) {
        if (!sample_next_guesses[prev_verdict].has_value()) {
          continue;
        }
        for (int candidate_verdict = 0; candidate_verdict < NUM_VERDICTS;
             candidate_verdict++) {
          if (!sample_next_guesses[candidate_verdict].has_value()) {
            continue;
          }
          char* candidate_guess =
              out_bank.words[sample_next_guesses[candidate_verdict].value()];
          int valid = check_is_hard_mode_valid(out_bank.words[i], prev_verdict,
                                               candidate_guess);
          out_bank
              .hard_mode_valid_candidates[i][prev_verdict][candidate_verdict] =
              valid;
        }
      }
    }
  };
  precompute_judge_data();
}

std::optional<int> find_word(const word_bank& bank, std::string word) {
  for (int i = 0; i < bank.num_words; i++) {
    if (word == bank.words[i]) {
      return i;
    }
  }
  return {};
}

#pragma endregion

#pragma region playing

constexpr int MAX_NUM_ATTEMPTS = 6;

struct word_list {
  int num_words;
  int num_targets;
  int words[MAX_BANK_SIZE];
};

using verdict_groups = word_list[NUM_VERDICTS];

void group_remaining_words(verdict_groups& out_groups, const word_bank& bank,
                           const word_list& remaining_words, int guess) {
  for (word_list& group : out_groups) {
    group.num_words = 0;
    group.num_targets = 0;
  }
  for (int i = 0; i < remaining_words.num_targets; i++) {
    int candidate = remaining_words.words[i];
    int verdict = bank.verdicts[guess][candidate];
    word_list& group = out_groups[verdict];
    group.words[group.num_words] = candidate;
    group.num_words++;
    group.num_targets++;
  }
  for (int verdict = 0; verdict < NUM_VERDICTS; verdict++) {
    word_list& group = out_groups[verdict];
    if (group.num_targets == 0) {
      continue;
    }
    for (int i = 0; i < remaining_words.num_words; i++) {
      int candidate = remaining_words.words[i];
      int candidate_verdict = bank.verdicts[guess][candidate];
      if (i < remaining_words.num_targets && candidate_verdict == verdict) {
        /* This candidate was already added with exact verdict match. */
        continue;
      }
      if (!bank.hard_mode_valid_candidates[guess][verdict][candidate_verdict]) {
        continue;
      }
      group.words[group.num_words] = candidate;
      group.num_words++;
    }
  }
}

static constexpr int ALL_GREEN_VERDICT = NUM_VERDICTS - 1;

struct performance_info {
  int total_attempts;
  bool has_missed_targets;
};

struct candidate_info {
  int guess;
  performance_info performance;
};

using find_best_guess_callback_for_candidate =
    std::function<void(candidate_info candidate)>;

candidate_info find_best_guess(
    const word_bank& bank, int num_attempts, const word_list& verdict_group,
    find_best_guess_callback_for_candidate callback_for_candidate);

using evaluate_guess_callback_for_group = std::function<void(
    int verdict, const word_list& group, candidate_info best_guess)>;

performance_info evaluate_guess(
    const word_bank& bank, int num_attempts, const word_list& remaining_words,
    int guess, evaluate_guess_callback_for_group callback_for_group) {
  performance_info performance = {};

  static verdict_groups preallocated_groups_by_attempt[MAX_NUM_ATTEMPTS];
  verdict_groups& groups = preallocated_groups_by_attempt[num_attempts - 1];
  group_remaining_words(groups, bank, remaining_words, guess);

  for (int verdict = NUM_VERDICTS - 1; verdict >= 0; verdict--) {
    if (verdict == ALL_GREEN_VERDICT) {
      continue;
    }
    const word_list& group = groups[verdict];
    if (groups[verdict].num_targets == 0) {
      continue;
    }
    candidate_info best_guess = find_best_guess(bank, num_attempts, group, {});
    if (callback_for_group) {
      callback_for_group(verdict, group, best_guess);
    }
    if (best_guess.performance.has_missed_targets) {
      return performance_info{.has_missed_targets = true};
    }
    performance.total_attempts += best_guess.performance.total_attempts;
  }
  return performance;
}

struct guess_heuristic {
  int num_verdict_groups_with_targets;
  int num_targets_in_largest_verdict_group;
  double entropy;
};

guess_heuristic compute_guess_heuristic(const word_bank& bank,
                                        const word_list& remaining_words,
                                        int guess) {
  int num_targets_by_verdict[NUM_VERDICTS] = {};
  for (int i = 0; i < remaining_words.num_targets; i++) {
    int target = remaining_words.words[i];
    int verdict = bank.verdicts[guess][target];
    num_targets_by_verdict[verdict]++;
  }

  guess_heuristic heuristic = {};
  for (int verdict = 0; verdict < NUM_VERDICTS; verdict++) {
    int group_size = num_targets_by_verdict[verdict];
    if (group_size == 0) {
      continue;
    }
    heuristic.num_verdict_groups_with_targets++;
    heuristic.num_targets_in_largest_verdict_group =
        std::max(heuristic.num_targets_in_largest_verdict_group, group_size);
    double group_probability = group_size * 1.0 / remaining_words.num_targets;
    heuristic.entropy += -std::log2(group_probability) * group_probability;
  }
  return heuristic;
};

static constexpr int NUM_CODES_IN_GROUP_HASH = 2;

namespace {

using word_list_hash = std::array<uint64_t, NUM_CODES_IN_GROUP_HASH>;

struct word_list_hash_unordered_map_hasher {
  uint64_t operator()(word_list_hash word_list_hash) const {
    uint64_t combined_hash = 0;
    for (uint64_t code : word_list_hash) {
      combined_hash = combined_hash * 31 + code;
    }
    return combined_hash;
  }
};

}  // namespace

static word_list_hash hash_word_list(const word_list& list) {
  static constexpr uint64_t MOD[] = {(1ULL << 63) - 25, (1ULL << 63) - 165};

  static uint64_t pow_2_mod[MAX_BANK_SIZE * 2][NUM_CODES_IN_GROUP_HASH];
  static const int precompute_pow_2_mod = []() -> int {
    std::fill_n(pow_2_mod[0], NUM_CODES_IN_GROUP_HASH, 1);
    for (int i = 1; i < MAX_BANK_SIZE * 2; i++) {
      for (int m = 0; m < NUM_CODES_IN_GROUP_HASH; m++) {
        pow_2_mod[i][m] = pow_2_mod[i - 1][m] * 2 % MOD[m];
      }
    }
    return 0;
  }();

  word_list_hash hash = {};
  for (int i = 0; i < list.num_words; i++) {
    for (int m = 0; m < NUM_CODES_IN_GROUP_HASH; m++) {
      bool is_word_target = i < list.num_targets;
      int encoded_word_index =
          list.words[i] + (is_word_target ? MAX_BANK_SIZE : 0);
      hash[m] += pow_2_mod[encoded_word_index][m];
      hash[m] %= MOD[m];
    }
  }
  return hash;
};

namespace {

using find_best_guess_by_word_list_cache =
    std::unordered_map<word_list_hash, candidate_info,
                       word_list_hash_unordered_map_hasher>;

};

static find_best_guess_by_word_list_cache
    find_best_guess_cache_by_attempt[MAX_NUM_ATTEMPTS];

candidate_info find_best_guess(
    const word_bank& bank, int num_attempts, const word_list& remaining_words,
    find_best_guess_callback_for_candidate callback_for_candidate) {
  if (remaining_words.num_targets == 1) {
    return candidate_info{
        .guess = remaining_words.words[0],
        .performance = {.total_attempts = 1},
    };
  }
  if (num_attempts == 1) {
    return candidate_info{
        .guess = remaining_words.words[0],
        .performance = {.has_missed_targets = true},
    };
  }
  if (remaining_words.num_targets == 2) {
    return candidate_info{
        .guess = remaining_words.words[0],
        .performance = {.total_attempts = 1 + 2},
    };
  }

  word_list_hash remaining_words_hash = hash_word_list(remaining_words);
  find_best_guess_by_word_list_cache& cache =
      find_best_guess_cache_by_attempt[num_attempts - 1];
  if (auto it = cache.find(remaining_words_hash); it != cache.end()) {
    return it->second;
  }

  const auto find_candidates = [](word_list& out_candidates,
                                  const word_bank& bank, int num_attempts,
                                  const word_list& remaining_words) -> void {
    const int MAX_ENTROPY_PLACE_TO_CONSIDER = 128;
    const double MAX_ENTROPY_DIFFERENCE_TO_CONSIDER = 1;

    struct candidate_heuristic {
      int candidate;
      double entropy;
      bool is_target;
    };
    static candidate_heuristic heuristics[MAX_BANK_SIZE];
    double max_candidate_entropy = 0;
    for (int i = 0; i < remaining_words.num_words; i++) {
      int candidate = remaining_words.words[i];
      double entropy =
          compute_guess_heuristic(bank, remaining_words, candidate).entropy;
      heuristics[i] = {
          .candidate = candidate,
          .entropy = entropy,
          .is_target = candidate < remaining_words.num_targets,
      };
      max_candidate_entropy = std::max(max_candidate_entropy, entropy);
    }
    double min_entropy_to_consider =
        max_candidate_entropy - MAX_ENTROPY_DIFFERENCE_TO_CONSIDER;
    if (remaining_words.num_words > MAX_ENTROPY_PLACE_TO_CONSIDER) {
      const auto candidate_heuristic_greater =
          [](const candidate_heuristic& a,
             const candidate_heuristic& b) -> bool {
        return std::tuple{a.entropy, a.is_target} >
               std::tuple{b.entropy, b.is_target};
      };
      candidate_heuristic* cutting_point =
          heuristics + MAX_ENTROPY_PLACE_TO_CONSIDER - 1;
      std::nth_element(heuristics, cutting_point,
                       heuristics + remaining_words.num_words,
                       candidate_heuristic_greater);
      min_entropy_to_consider =
          std::max(min_entropy_to_consider, cutting_point->entropy);
    }

    out_candidates.num_words = 0;
    out_candidates.num_targets = 0;
    for (int i = 0; i < remaining_words.num_words; i++) {
      const candidate_heuristic& heuristic = heuristics[i];
      if (heuristic.entropy < min_entropy_to_consider) {
        continue;
      }
      out_candidates.words[out_candidates.num_words] = heuristic.candidate;
      out_candidates.num_words++;
      if (heuristic.is_target) {
        out_candidates.num_targets++;
      }
    }
  };
  static word_list preallocated_candidates_by_attempt[MAX_NUM_ATTEMPTS];
  word_list& candidates = preallocated_candidates_by_attempt[num_attempts - 1];
  find_candidates(candidates, bank, num_attempts, remaining_words);

  candidate_info best_guess = {
      .guess = remaining_words.words[0],
      .performance = {.has_missed_targets = true},
  };
  for (int i = 0; i < candidates.num_words; i++) {
    int candidate = candidates.words[i];
    performance_info guess_performance =
        evaluate_guess(bank, num_attempts - 1, remaining_words, candidate, {});
    guess_performance.total_attempts += remaining_words.num_targets;
    if (callback_for_candidate) {
      callback_for_candidate(candidate_info{
          .guess = candidate,
          .performance = guess_performance,
      });
    }
    if (std::tuple{guess_performance.has_missed_targets,
                   guess_performance.total_attempts} <
        std::tuple{best_guess.performance.has_missed_targets,
                   best_guess.performance.total_attempts}) {
      best_guess = {
          .guess = candidate,
          .performance = guess_performance,
      };
    }
  }

  cache[remaining_words_hash] = best_guess;
  return best_guess;
}

static void clear_bank_specific_caches() {
  for (find_best_guess_by_word_list_cache& cache :
       find_best_guess_cache_by_attempt) {
    cache = {};
  }
}

#pragma endregion

}  // namespace wordy_witch
