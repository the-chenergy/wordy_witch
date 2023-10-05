#pragma once

#include <algorithm>
#include <array>
#include <bit>
#include <bitset>
#include <cassert>
#include <cmath>
#include <cstdint>
#include <ext/pb_ds/assoc_container.hpp>
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

struct Group {
  int num_words;
  int num_targets;
  int words[MAX_BANK_SIZE];
};

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

using Cost = double;

constexpr Cost INFINITE_COST = std::numeric_limits<Cost>::infinity();

struct BestGuessInfo {
  int guess;
  Cost cost;
  int guess_candidate_index;
};

using FindBestGuessCallbackForCandidate =
    std::function<void(int candidate_index, int candidate, Cost cost)>;

BestGuessInfo find_best_guess(
    const Bank& bank, int num_attempts, const Group& guessable,
    FindBestGuessCallbackForCandidate callback_for_candidate);

using EvaluateGuessCallbackForGroup = std::function<void(
    int verdict, const Group& group, BestGuessInfo best_guess)>;

Cost evaluate_guess(const Bank& bank, int num_attempts,
                    const Grouping& grouping,
                    EvaluateGuessCallbackForGroup callback_for_group) {
  Cost total_cost = 0;
  for (int verdict = NUM_VERDICTS - 1; verdict >= 0; verdict--) {
    if (verdict == ALL_GREEN_VERDICT) {
      continue;
    }
    const Group& group = grouping.groups[verdict];
    if (grouping.groups[verdict].num_targets == 0) {
      continue;
    }
    BestGuessInfo best_guess = find_best_guess(bank, num_attempts, group, {});
    if (callback_for_group) {
      callback_for_group(verdict, group, best_guess);
    }
    if (best_guess.cost >= INFINITE_COST) {
      return INFINITE_COST;
    }
    total_cost += best_guess.cost;
  }
  return total_cost;
}

BestGuessInfo find_best_guess(
    const Bank& bank, int num_attempts, const Group& guessable,
    FindBestGuessCallbackForCandidate callback_for_candidate) {
  assert(num_attempts > 0);
  if (guessable.num_targets == 1) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = 1};
  }
  if (num_attempts == 1) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = INFINITE_COST};
  }
  if (guessable.num_targets == 2) {
    return BestGuessInfo{.guess = guessable.words[0], .cost = 1 + 2};
  }

  constexpr int NUM_CODES_IN_GROUP_HASH = 2;
  using GroupHash = std::array<uint64_t, NUM_CODES_IN_GROUP_HASH>;
  const auto hash_group = [](const Group& group) -> GroupHash {
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
  };

  struct GroupHashFindBestGuessCacheHasher {
    uint64_t operator()(GroupHash group_hash) const {
      uint64_t combined_hash = 0;
      for (uint64_t code : group_hash) {
        combined_hash = combined_hash * 31 + code;
      }
      return combined_hash;
    }
  };
  using FindBestGuessCache =
      __gnu_pbds::gp_hash_table<GroupHash, BestGuessInfo,
                                GroupHashFindBestGuessCacheHasher>;
  static FindBestGuessCache find_best_guess_cache_by_attempt[MAX_NUM_ATTEMPTS];
  GroupHash guessable_hash = hash_group(guessable);
  FindBestGuessCache& cache =
      find_best_guess_cache_by_attempt[num_attempts - 1];
  if (auto it = cache.find(guessable_hash); it != cache.end()) {
    return it->second;
  }

  static Grouping preallocated_grouping_by_attempt[MAX_NUM_ATTEMPTS];
  Grouping& grouping = preallocated_grouping_by_attempt[num_attempts - 1];

  const auto prune_candidates = [num_attempts, &bank, &guessable, &grouping](
                                    int& out_num_candidates,
                                    int* out_candidates) -> void {
    constexpr int MAX_ENTROPY_PLACE_TO_CONSIDER = 31;
    constexpr double MAX_ENTROPY_DIFFERENCE_TO_CONSIDER = 1;

    struct CandidateHeuristic {
      int candidate;
      bool is_candidate_target;
      GroupingHeuristic heuristic;
    };
    static CandidateHeuristic candidate_heuristics[MAX_BANK_SIZE];

    double max_candidate_entropy = 0;
    for (int i = 0; i < guessable.num_words; i++) {
      int candidate = guessable.words[i];
      group_guesses(grouping, bank, guessable, candidate);
      candidate_heuristics[i] = {
          .candidate = candidate,
          .is_candidate_target = candidate < guessable.num_targets,
          .heuristic = grouping.heuristic,
      };
      max_candidate_entropy =
          std::max(max_candidate_entropy, grouping.heuristic.entropy);
    }
    double min_entropy_to_consider =
        max_candidate_entropy - MAX_ENTROPY_DIFFERENCE_TO_CONSIDER;
    if (guessable.num_words > MAX_ENTROPY_PLACE_TO_CONSIDER) {
      const auto candidate_heuristic_greater =
          [&guessable](const CandidateHeuristic& a,
                       const CandidateHeuristic& b) -> bool {
        return std::tuple{a.heuristic.entropy, a.is_candidate_target} >
               std::tuple{b.heuristic.entropy, b.is_candidate_target};
      };
      CandidateHeuristic* cutting_point =
          candidate_heuristics + MAX_ENTROPY_PLACE_TO_CONSIDER;
      std::nth_element(candidate_heuristics, cutting_point,
                       candidate_heuristics + guessable.num_words,
                       candidate_heuristic_greater);
      min_entropy_to_consider =
          std::max(min_entropy_to_consider, cutting_point->heuristic.entropy);
    }

    out_num_candidates = 0;
    for (int i = 0; i < guessable.num_words; i++) {
      const CandidateHeuristic& ch = candidate_heuristics[i];
      if (ch.heuristic.entropy < min_entropy_to_consider) {
        continue;
      }
      out_candidates[out_num_candidates] = ch.candidate;
      out_num_candidates++;
    }
  };

  static int preallocated_candidates_by_attempt[MAX_NUM_ATTEMPTS]
                                               [MAX_BANK_SIZE];
  int* candidates = preallocated_candidates_by_attempt[num_attempts - 1];
  int num_candidates_to_consider;
  prune_candidates(num_candidates_to_consider, candidates);

  BestGuessInfo best_guess = {.guess = guessable.words[0],
                              .cost = INFINITE_COST};
  for (int i = 0; i < num_candidates_to_consider; i++) {
    int candidate = candidates[i];
    group_guesses(grouping, bank, guessable, candidate);
    Cost cost = guessable.num_targets +
                evaluate_guess(bank, num_attempts - 1, grouping, {});
    if (callback_for_candidate) {
      callback_for_candidate(i, candidate, cost);
    }
    if (cost < best_guess.cost) {
      best_guess = BestGuessInfo{
          .guess = candidate,
          .cost = cost,
          .guess_candidate_index = i,
      };
    }
  }

  cache[guessable_hash] = best_guess;
  return best_guess;
}

#pragma endregion

}  // namespace wordy_witch
