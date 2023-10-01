#pragma once

#include <algorithm>
#include <bitset>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

namespace wordy_witch {

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
    `hard_mode_valid_candidates[prev_guess][candidate_guess_verdict][prev_verdict]`
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
      std::optional<int> sample_next_guesses[NUM_VERDICTS] = {};
      for (int j = 0; j < out_bank.num_words; j++) {
        int verdict = judge(out_bank.words[i], out_bank.words[j]);
        out_bank.verdicts[i][j] = verdict;
        sample_next_guesses[verdict] = j;
      }
      for (int candidate_verdict = 0; candidate_verdict < NUM_VERDICTS;
           candidate_verdict++) {
        if (!sample_next_guesses[candidate_verdict].has_value()) {
          continue;
        }
        for (int prev_verdict = 0; prev_verdict < NUM_VERDICTS;
             prev_verdict++) {
          if (!sample_next_guesses[candidate_verdict].has_value()) {
            continue;
          }
          char* candidate_guess =
              out_bank.words[sample_next_guesses[candidate_verdict].value()];
          int valid = check_is_hard_mode_valid(out_bank.words[i], prev_verdict,
                                               candidate_guess);
          out_bank
              .hard_mode_valid_candidates[i][candidate_verdict][prev_verdict] =
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

}  // namespace wordy_witch
