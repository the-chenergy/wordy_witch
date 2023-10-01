#include <iostream>
#include <numeric>

#include "bot.hh"
#include "log.hh"

int main() {
  static wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::COMMON_WORDS);

  static wordy_witch::Group initial_group;
  initial_group.num_words = bank.num_words;
  initial_group.num_targets = bank.num_targets;
  std::iota(initial_group.words, initial_group.words + initial_group.num_words,
            0);
  static wordy_witch::Group groups[wordy_witch::NUM_VERDICTS];
  wordy_witch::group_guesses(groups, bank, initial_group,
                             wordy_witch::find_word(bank, "least").value());
  int cost = wordy_witch::evaluate_guess(
      bank, 2, groups, 1,
      [](int verdict, const wordy_witch::Group &group,
         std::optional<int> best_guess, int best_guess_cost) -> void {
        int best_guess_index = best_guess.has_value() ? best_guess.value() : 0;
        WORDY_WITCH_TRACE(wordy_witch::format_verdict(verdict), group.num_words,
                          group.num_targets, bank.words[best_guess_index],
                          best_guess_cost,
                          best_guess_cost * 1.0 / group.num_targets);
      });
  WORDY_WITCH_TRACE(cost, cost * 1.0 / initial_group.num_targets);
}
