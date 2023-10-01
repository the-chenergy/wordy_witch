#include <iostream>

#include "bot.hh"

int main() {
  static wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::ALL_WORDS);

  std::cerr << bank.words[0] << " " << bank.words[bank.num_targets] << " "
            << bank.num_words << " " << bank.num_targets << std::endl;
  int a = wordy_witch::find_word(bank, "sneer").value();
  int b = wordy_witch::find_word(bank, "screw").value();
  std::cerr << bank.words[a] << " " << bank.words[b] << " "
            << wordy_witch::format_verdict(bank.verdicts[a][b]) << " "
            << wordy_witch::format_verdict(bank.verdicts[b][a]) << std::endl;
  for (int v = 0; v < wordy_witch::NUM_VERDICTS; v++) {
    std::cerr << v << " " << bank.words[a] << " " << bank.words[b] << " "
              << wordy_witch::format_verdict(v) << " "
              << bank.hard_mode_valid_candidates[a][bank.verdicts[a][b]][v] << " "
              << bank.hard_mode_valid_candidates[b][bank.verdicts[b][a]][v]
              << "\n";
  }
}
