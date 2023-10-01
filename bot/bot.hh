#pragma once

#include <algorithm>
#include <bit>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <optional>
#include <string>
#include <utility>

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

struct Group {
  int num_words;
  int num_targets;
  int words[MAX_BANK_SIZE];
};

void group_guesses(Group (&out_groups)[NUM_VERDICTS], const Bank& bank,
                   const Group& prev_group, int prev_guess) {
  for (Group& g : out_groups) {
    g.num_words = 0;
    g.num_targets = 0;
  }
  for (int i = 0; i < prev_group.num_targets; i++) {
    Group& group = out_groups[bank.verdicts[prev_guess][prev_group.words[i]]];
    group.words[group.num_words] = prev_group.words[i];
    group.num_words++;
    group.num_targets++;
  }
  for (int prev_verdict = 0; prev_verdict < NUM_VERDICTS; prev_verdict++) {
    Group& group = out_groups[prev_verdict];
    if (group.num_targets == 0) {
      continue;
    }
    for (int i = 0; i < prev_group.num_words; i++) {
      int candidate_verdict = bank.verdicts[prev_guess][prev_group.words[i]];
      if (i < prev_group.num_targets && candidate_verdict == prev_verdict) {
        continue;
      }
      if (!bank.hard_mode_valid_candidates[prev_guess][prev_verdict]
                                          [candidate_verdict]) {
        continue;
      }
      group.words[group.num_words] = prev_group.words[i];
      group.num_words++;
    }
  }
}

std::pair<int, std::optional<int>> find_best_guess(const Bank& bank,
                                                   int num_attempts,
                                                   const Group& guessable,
                                                   int cost_so_far);

int evaluate_guess(
    const Bank& bank, int num_attempts, const Group (&groups)[NUM_VERDICTS],
    int cost_so_far,
    std::function<void(int verdict, const Group& group,
                       std::optional<int> best_guess, int best_guess_cost)>
        callback_for_group) {
  int total_cost = 0;
  if (groups[NUM_VERDICTS - 1].num_targets > 0) {
    total_cost += cost_so_far;
  }
  for (int verdict = 0; verdict < NUM_VERDICTS - 1; verdict++) {
    const Group& group = groups[verdict];
    if (group.num_targets == 0) {
      continue;
    }
    auto [best_cost, best_next_guess] =
        find_best_guess(bank, num_attempts, group, cost_so_far);
    if (callback_for_group) {
      callback_for_group(verdict, group, best_next_guess, best_cost);
    }
    if (best_cost >= COST_INFINITY || !best_next_guess.has_value()) {
      return COST_INFINITY;
    }
    total_cost += best_cost;
  }
  return total_cost;
}

std::pair<int, std::optional<int>> find_best_guess(const Bank& bank,
                                                   int num_attempts,
                                                   const Group& guessable,
                                                   int cost_so_far) {
  if (num_attempts <= 0) {
    return {COST_INFINITY, {}};
  }
  if (guessable.num_targets == 1) {
    return {cost_so_far + 1, guessable.words[0]};
  }
  if (num_attempts == 1) {
    if (guessable.num_targets > 1) {
      return {COST_INFINITY, {}};
    }
    return {cost_so_far + 1, guessable.words[0]};
  }
  if (guessable.num_targets == 2) {
    return {(cost_so_far + 1) + (cost_so_far + 2), guessable.words[0]};
  }

  static Group preallocated_groups_by_attempt[MAX_NUM_ATTEMPTS][NUM_VERDICTS];
  auto& groups = preallocated_groups_by_attempt[num_attempts - 1];

  std::pair<int, std::optional<int>> best = {COST_INFINITY, {}};
  for (int i = 0; i < guessable.num_words; i++) {
    int guess = guessable.words[i];
    group_guesses(groups, bank, guessable, guess);
    int guess_cost =
        evaluate_guess(bank, num_attempts - 1, groups, cost_so_far + 1, {});
    if (guess_cost < best.first) {
      best = {guess_cost, guess};
    }
  }
  return best;
}

#pragma endregion

}  // namespace wordy_witch