#include <iomanip>
#include <iostream>
#include <numeric>
#include <optional>
#include <vector>

#include "bot.hh"
#include "log.hh"

int main() {
  static wordy_witch::Bank bank;
  wordy_witch::load_bank(bank, "../bank/co_wordle",
                         wordy_witch::BankGuessesInclusion::ALL_WORDS);
  std::vector<std::string> state = {
      "least",
      // "-----",
      // "crump",
  };

  const auto populate_group_with_full_bank =
      [](wordy_witch::Group& out_group) -> void {
    out_group.num_words = bank.num_words;
    out_group.num_targets = bank.num_targets;
    std::iota(out_group.words, out_group.words + out_group.num_words, 0);
  };

  static wordy_witch::Group group;
  populate_group_with_full_bank(group);

  static wordy_witch::Grouping grouping;

  std::cout << std::setprecision(4);

  const auto display_initial_message =
      [](const std::vector<std::string>& state) -> void {
    std::cout << "WordyWitch analysis" << std::endl;
    std::cout << std::endl;
    std::cout << "Board state to analyze:" << std::endl;
    for (int i = 0; i < state.size(); i++) {
      if (i % 2 == 0) {
        int guess = wordy_witch::find_word(bank, state[i]).value();
        wordy_witch::group_guesses(grouping, bank, group, guess);
      } else {
        const auto parse_verdict =
            [](const std::string& tiles) -> std::optional<int> {
          for (int verdict = 0; verdict < wordy_witch::NUM_VERDICTS;
               verdict++) {
            if (wordy_witch::format_verdict(verdict) == tiles) {
              return {verdict};
            }
          }
          return {};
        };
        int verdict = parse_verdict(state[i]).value();
        group = grouping.groups[verdict];
      }
      std::cout << state[i] << std::endl;
    }
    std::cout << std::endl;
  };
  display_initial_message(state);

  const auto find_and_display_best_guess = [](int num_attempts) -> void {
    std::cout << "Candidate best guesses in this board state:" << std::endl;
    std::cout
        << "(CID: the index of this candidate; only for making looking up "
           "easier)"
        << std::endl;
    std::cout << "(Guess: a candidate best guess in this board state, after "
                 "basic pruning by entropy)"
              << std::endl;
    std::cout << "(Cost: total number of attempts to win every remaining "
                 "possible Wordle, under best play after guessing this word)"
              << std::endl;
    std::cout << "(EA: expected attempts; `Cost` divided by the number of "
                 "remaining possible target words)"
              << std::endl;
    std::cout << "(H: the Shannon entropy of this guess)" << std::endl;
    std::cout << "(NVG: the number of verdict groups this guess produces)"
              << std::endl;
    std::cout
        << "(LVG: the number of possible target words in the largest verdict "
           "group after guessing this word)"
        << std::endl;
    std::cout << "CID\tGuess\tCost\tEA\tH\tNVG\tLVG" << std::endl;
    wordy_witch::BestGuessInfo best_guess = wordy_witch::find_best_guess(
        bank, num_attempts, group,
        [](int candidate_index, int candidate, int cost) -> void {
          WORDY_WITCH_TRACE("Analyzed verdict group", candidate, cost);

          static wordy_witch::Grouping temp_grouping;
          wordy_witch::group_guesses(temp_grouping, bank, group, candidate);

          std::cout << candidate_index << "\t" << bank.words[candidate] << "\t"
                    << cost << "\t" << cost * 1.0 / group.num_targets << "\t"
                    << temp_grouping.heuristic.entropy << "\t"
                    << temp_grouping.heuristic.num_groups_with_targets << "\t"
                    << temp_grouping.heuristic.largest_group_num_targets
                    << std::endl;
        });
    std::cout << std::endl;
    std::cout << "Best guess in the input board state: "
              << bank.words[best_guess.guess]
              << " (CID: " << best_guess.guess_candidate_index
              << ", Cost: " << best_guess.cost
              << ", EA: " << best_guess.cost * 1.0 / group.num_targets << ")"
              << std::endl;
  };

  const auto find_and_display_best_guess_by_verdict =
      [](int num_attempts, const std::string& last_guess) -> void {
    std::cout << "Best guesses in this board state for each possible verdict:"
              << std::endl;
    std::cout << "(VID: a base-3 encoded number of this verdict)" << std::endl;
    std::cout << "(LG: your last guess)" << std::endl;
    std::cout << "(V: the verdict tiles: - black, ^ yellow, # green)"
              << std::endl;
    std::cout << "(NG: the best next guess given this verdict)" << std::endl;
    std::cout << "(GL: the number of guessable words left in Hard Mode after "
                 "this verdict shows)"
              << std::endl;
    std::cout << "(TL: the number of possible target words left after this "
                 "verdict shows)"
              << std::endl;
    std::cout
        << "(Cost: total number of attempts to win every remaining "
           "possible Wordle, under best play after guessing the best word)"
        << std::endl;
    std::cout << "(EA: expected attempts; `Cost` divided by the number of "
                 "remaining possible target words after this verdict)"
              << std::endl;
    std::cout << "(H: the Shannon entropy of the best guess)" << std::endl;
    std::cout << "(NVG: the number of verdict groups the best produces)"
              << std::endl;
    std::cout
        << "(LVG: the number of possible target words in the largest verdict "
           "group after guessing the best guess)"
        << std::endl;
    std::cout << "VID\tLG\tV\tNG\tGL\tTL\tCost\tEA\tH\tNVG\tLVG" << std::endl;
    int cost = wordy_witch::evaluate_guess(
        bank, num_attempts, grouping,
        [&last_guess](int verdict, const wordy_witch::Group& group,
                      wordy_witch::BestGuessInfo best_guess) -> void {
          WORDY_WITCH_TRACE("Analyzed candidate", verdict, best_guess.guess,
                            best_guess.cost);

          static wordy_witch::Grouping temp_grouping;
          wordy_witch::group_guesses(temp_grouping, bank, group,
                                     best_guess.guess);

          std::cout << verdict << "\t" << last_guess << "\t"
                    << wordy_witch::format_verdict(verdict) << "\t"
                    << bank.words[best_guess.guess] << "\t" << group.num_words
                    << "\t" << group.num_targets << "\t" << best_guess.cost
                    << "\t" << best_guess.cost * 1.0 / group.num_targets << "\t"
                    << temp_grouping.heuristic.entropy << "\t"
                    << temp_grouping.heuristic.num_groups_with_targets << "\t"
                    << temp_grouping.heuristic.largest_group_num_targets
                    << std::endl;
        });
    std::cout << std::endl;
    std::cout << "Overall, best play after guessing \"" << last_guess
              << "\" (H: " << grouping.heuristic.entropy
              << ", NVG: " << grouping.heuristic.num_groups_with_targets
              << ", LVG: " << grouping.heuristic.largest_group_num_targets
              << ") produces a mean of " << cost * 1.0 / group.num_targets
              << " (Cost: " << cost << ")" << std::endl;
  };

  if (state.size() % 2 == 0) {
    find_and_display_best_guess(6 - state.size() / 2);
  } else {
    find_and_display_best_guess_by_verdict(5 - state.size() / 2, state.back());
  }
}
