#include <iostream>

#include "bot.hh"

int main() {
  static wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::COMMON_WORDS);

  std::cerr << bank.words[0] << " " << bank.words[bank.num_targets] << " "
            << bank.num_words << " " << bank.num_targets << std::endl;
  int a = wordy_witch::find_word(bank, "stare").value();
  int b = wordy_witch::find_word(bank, "steed").value();
  std::cerr << bank.words[a] << " " << bank.words[b] << " "
            << wordy_witch::format_verdict(bank.verdicts[a][b]) << " "
            << wordy_witch::format_verdict(bank.verdicts[b][a]) << std::endl;
}
