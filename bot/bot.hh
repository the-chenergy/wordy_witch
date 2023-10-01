#pragma once

#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <fstream>
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

constexpr int MAX_BANK_SIZE = 1 << 14;

struct Bank {
  char words[MAX_BANK_SIZE][WORD_SIZE + 1];
  int num_words;
  int num_targets;

  uint8_t verdicts[MAX_BANK_SIZE][MAX_BANK_SIZE];
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
    for (int g = 0; g < out_bank.num_words; g++) {
      for (int t = 0; t < out_bank.num_words; t++) {
        out_bank.verdicts[g][t] = judge(out_bank.words[g], out_bank.words[t]);
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
