#pragma once

#include <filesystem>
#include <fstream>

namespace wordy_witch {

constexpr int WORD_SIZE = 5;
constexpr int MAX_BANK_SIZE = 1 << 14;

struct Bank {
  char words[MAX_BANK_SIZE][WORD_SIZE + 1];
  int num_words;
  int num_targets;
};

enum struct BankGuessesInclusion : int {
  TARGETS_ONLY,
  COMMON_WORDS,
  ALL_WORDS,
};

void load_bank(Bank& out_bank, std::filesystem::path dict_path,
               BankGuessesInclusion guesses_inclusion) {
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
}

}  // namespace wordy_witch
