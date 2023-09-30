#include <iostream>

#include "bot.hh"

int main() {
  wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::COMMON_WORDS);

  std::cerr << bank.words[0] << " " << bank.words[bank.num_targets] << " "
            << bank.num_words << " " << bank.num_targets << std::endl;
}
