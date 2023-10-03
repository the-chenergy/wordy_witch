#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <numeric>
#include <optional>
#include <string>
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

struct Bank {
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

enum struct BankGuessesInclusion : int {
  TARGETS_ONLY,
  COMMON_WORDS,
  ALL_WORDS,
};

void load_bank(Bank& out_bank, std::filesystem::path dict_path,
               BankGuessesInclusion guesses_inclusion) {
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

    if (guesses_inclusion == BankGuessesInclusion::TARGETS_ONLY) {
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

    if (guesses_inclusion != BankGuessesInclusion::ALL_WORDS) {
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

std::optional<int> find_word(const Bank& bank, std::string word) {
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
constexpr int COST_INFINITY = 1 << 18;

static constexpr int NUM_CODES_IN_GROUP_HASH = 2;
using GroupHash = std::array<uint64_t, NUM_CODES_IN_GROUP_HASH>;

struct Group {
  int num_words;
  int num_targets;
  int words[MAX_BANK_SIZE];
};

GroupHash hash_group(const Group& group) {
  static constexpr uint64_t MOD[] = {
      (1ULL << 63) - 25, (1ULL << 63) - 165,
      // (1ULL << 63) - 259,
      // (1ULL << 63) - 301,
  };

  static uint64_t pow_2_mod[MAX_BANK_SIZE * 2][NUM_CODES_IN_GROUP_HASH];
  static const int compute_pow_2_mod = []() -> int {
    std::fill_n(pow_2_mod[0], NUM_CODES_IN_GROUP_HASH, 1);
    for (int i = 1; i < MAX_BANK_SIZE * 2; i++) {
      for (int m = 0; m < NUM_CODES_IN_GROUP_HASH; m++) {
        pow_2_mod[i][m] = pow_2_mod[i - 1][m] * 2 % MOD[m];
      }
    }
    return 0;
  }();

  GroupHash hash = {};
  for (int i = 0; i < group.num_words; i++) {
    for (int m = 0; m < NUM_CODES_IN_GROUP_HASH; m++) {
      bool is_word_target = i < group.num_targets;
      int encoded_word_index =
          group.words[i] + (is_word_target ? MAX_BANK_SIZE : 0);
      hash[m] += pow_2_mod[encoded_word_index][m];
      hash[m] %= MOD[m];
    }
  }
  return hash;
}

struct GroupingHeuristic {
  int num_groups_with_targets;
  int largest_group_num_targets;
  double entropy;
};

struct Grouping {
  GroupingHeuristic heuristic;
  Group groups[NUM_VERDICTS];
};

void group_guesses(Grouping& out_grouping, const Bank& bank,
                   const Group& prev_group, int prev_guess) {
  for (Group& g : out_grouping.groups) {
    g.num_words = 0;
    g.num_targets = 0;
  }
  for (int i = 0; i < prev_group.num_targets; i++) {
    int candidate = prev_group.words[i];
    int verdict = bank.verdicts[prev_guess][candidate];
    Group& group = out_grouping.groups[verdict];
    group.words[group.num_words] = candidate;
    group.num_words++;
    group.num_targets++;
  }
  GroupingHeuristic& heuristic = out_grouping.heuristic;
  heuristic.num_groups_with_targets = 0;
  heuristic.largest_group_num_targets = 0;
  heuristic.entropy = 0;
  for (int prev_verdict = 0; prev_verdict < NUM_VERDICTS; prev_verdict++) {
    Group& group = out_grouping.groups[prev_verdict];
    if (group.num_targets == 0) {
      continue;
    }
    heuristic.num_groups_with_targets++;
    heuristic.largest_group_num_targets =
        std::max(heuristic.largest_group_num_targets, group.num_targets);
    double group_probability = 1.0 * group.num_targets / prev_group.num_targets;
    heuristic.entropy += -std::log2(group_probability) * group_probability;

    for (int i = 0; i < prev_group.num_words; i++) {
      int candidate = prev_group.words[i];
      int candidate_verdict = bank.verdicts[prev_guess][candidate];
      if (i < prev_group.num_targets && candidate_verdict == prev_verdict) {
        continue;
      }
      if (!bank.hard_mode_valid_candidates[prev_guess][prev_verdict]
                                          [candidate_verdict]) {
        continue;
      }
      group.words[group.num_words] = candidate;
      group.num_words++;
    }
  }
}

static constexpr int ALL_GREEN_VERDICT = NUM_VERDICTS - 1;

struct BestGuessInfo {
  int guess;
  int cost;
};

BestGuessInfo find_best_guess(const Bank& bank, int num_attempts,
                              const Group& guessable);

int evaluate_guess(const Bank& bank, int num_attempts, const Grouping& grouping,
                   std::function<void(int verdict, const Group& group,
                                      BestGuessInfo best_guess)>
                       callback_for_group) {
  int total_cost = 0;
  for (int verdict = NUM_VERDICTS - 1; verdict >= 0; verdict--) {
    if (verdict == ALL_GREEN_VERDICT) {
      continue;
    }
    const Group& group = grouping.groups[verdict];
    if (grouping.groups[verdict].num_targets == 0) {
      continue;
    }
    BestGuessInfo best_guess = find_best_guess(bank, num_attempts, group);
    if (callback_for_group) {
      callback_for_group(verdict, group, best_guess);
    }
    if (best_guess.cost >= COST_INFINITY) {
      return COST_INFINITY;
    }
    total_cost += best_guess.cost;
  }
  return total_cost;
}

struct GroupHashUnorderedMapHasher {
  uint64_t operator()(GroupHash group_hash) const {
    uint64_t hash = 0;
    for (uint64_t code : group_hash) {
      hash = hash * 31 + code;
    }
    return hash;
  }
};
static std::unordered_map<GroupHash, BestGuessInfo, GroupHashUnorderedMapHasher>
    find_best_guess_cache[MAX_NUM_ATTEMPTS];

BestGuessInfo find_best_guess(const Bank& bank, int num_attempts,
                              const Group& guessable) {
  assert(num_attempts > 0);
  if (guessable.num_targets == 1) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = 1};
  }
  if (num_attempts == 1) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = COST_INFINITY};
  }
  if (guessable.num_targets == 2) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = 1 + 2};
  }

  GroupHash guessable_hash = hash_group(guessable);
  auto& cache = find_best_guess_cache[num_attempts - 1];
  if (auto it = cache.find(guessable_hash); it != cache.end()) {
    return it->second;
  }

  static Grouping preallocated_grouping_by_attempt[MAX_NUM_ATTEMPTS];
  Grouping& grouping = preallocated_grouping_by_attempt[num_attempts - 1];

  using GuessHeuristic = std::pair<int, GroupingHeuristic>;
  static GuessHeuristic
      preallocated_guesses_heuristics_by_attempt[MAX_NUM_ATTEMPTS]
                                                [MAX_BANK_SIZE];
  auto& guesses_and_heuristics =
      preallocated_guesses_heuristics_by_attempt[num_attempts - 1];
  double max_guess_entropy = 0;
  for (int i = 0; i < guessable.num_words; i++) {
    int guess = guessable.words[i];
    group_guesses(grouping, bank, guessable, guess);
    guesses_and_heuristics[i] = {guess, grouping.heuristic};
    max_guess_entropy = std::max(max_guess_entropy, grouping.heuristic.entropy);
  }
  constexpr int MAX_NUM_CANDIDATES_TO_CONSIDER = 24;
  constexpr double MAX_ENTROPY_DIFFERENCE_TO_CONSIDER = 0.5;
  if (guessable.num_words > MAX_NUM_CANDIDATES_TO_CONSIDER) {
    std::nth_element(guesses_and_heuristics,
                     guesses_and_heuristics + MAX_NUM_CANDIDATES_TO_CONSIDER,
                     guesses_and_heuristics + guessable.num_words,
                     [](GuessHeuristic a, GuessHeuristic b) -> bool {
                       return a.second.entropy > b.second.entropy;
                     });
  }

  BestGuessInfo best_guess = {.guess = guessable.words[0],
                              .cost = COST_INFINITY};
  for (int i = 0;
       i < std::min(guessable.num_words, MAX_NUM_CANDIDATES_TO_CONSIDER); i++) {
    auto [guess, heuristic] = guesses_and_heuristics[i];
    if (heuristic.entropy <
        max_guess_entropy - MAX_ENTROPY_DIFFERENCE_TO_CONSIDER) {
      continue;
    }
    group_guesses(grouping, bank, guessable, guess);
    int cost = evaluate_guess(bank, num_attempts - 1, grouping, {}) +
               guessable.num_targets;
    if (cost < best_guess.cost) {
      best_guess = BestGuessInfo{.guess = guess, .cost = cost};
    }
  }

  cache[guessable_hash] = best_guess;
  return best_guess;
}

#pragma endregion

}  // namespace wordy_witch
