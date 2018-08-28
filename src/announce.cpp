/**
 Copyright   John Agapeyev 2018

 This program is free software: you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation, either version 3 of the License, or
 (at your option) any later version.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program.  If not, see <http://www.gnu.org/licenses/>.
*/
#include <algorithm>
#include <atomic>
#include <cassert>
#include <climits>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <sstream>
#include <vector>

#include "announce.h"
#include "interactive.h"
#include "tools.h"
#include "utils.h"

std::string find_announcement(const std::vector<agent>& agents) noexcept {
    std::vector<std::vector<std::vector<int32_t>>> goals;

    for (const auto& agent : agents) {
        goals.emplace_back(agent.goal);
    }

    if (goals_consistent(goals)) {
        std::cout << "Goals are consistent\n";

        if (verbose) {
            for (const auto& agent : agents) {
                for (const auto& clause : agent.goal) {
                    for (const auto term : clause) {
                        std::cout << term << " ";
                    }
                    std::cout << "\n";
                }
                std::cout << "\n";
            }
        }

        std::vector<std::vector<int32_t>> conjunction;
        std::vector<std::vector<int32_t>> base_goal{goals.front()};

        for (const auto& goal : goals) {
            if (goal == goals.front()) {
                continue;
            }
            for (const auto& base_clause : base_goal) {
                for (const auto& clause : goal) {
                    std::vector<int32_t> appended{base_clause};
                    appended.insert(appended.end(), clause.begin(), clause.end());
                    conjunction.emplace_back(std::move(appended));
                }
            }
        }

        //Conjunction will be empty when the only goal is a single term function
        if (conjunction.empty()) {
            conjunction = std::move(base_goal);
        }
        simplify_dnf(conjunction);

        return print_formula_dnf(conjunction);
    }

    std::cout << "Goals are inconsistent, trying to find a solution\n";

    const auto max_var = get_variable_count(agents);
    assert(max_var <= 64);

    for (auto i = 1; i <= max_var; ++i) {
        std::vector<bool> v(max_var);
        std::fill(v.begin(), v.begin() + i, true);

        //Generate all length i combinations of variables
        do {
            std::vector<int32_t> combination;
            for (int j = 0; j < max_var; ++j) {
                if (v[j]) {
                    combination.emplace_back(j + 1);
                }
            }

            //Generate all variable negation variations
            for (uint64_t j = 0; j < (1ul << i); ++j) {
                std::vector<int32_t> negated_comb{combination};
                for (auto k = 0; k < i; ++k) {
                    const auto val = (1 << k) & j;
                    if (!val) {
                        negated_comb[k] *= -1;
                    }
                }

                //Generate all conjunction permutations
                for (uint64_t k = 0; k < (1ul << (i - 1)); ++k) {
                    std::vector<bool> conjunctions;
                    for (auto l = 0; l < (i - 1); ++l) {
                        const auto val = (1 << l) & k;
                        conjunctions.emplace_back(val);
                    }

                    std::vector<std::vector<int32_t>> revision_formula;
                    for (auto l = 0; l < i; ++l) {
                        if (l == 0) {
                            revision_formula.push_back({negated_comb[0]});
                        } else {
                            if (conjunctions[l - 1]) {
                                //AND
                                revision_formula.back().push_back(negated_comb[l]);
                            } else {
                                //OR
                                revision_formula.push_back({negated_comb[l]});
                            }
                        }
                    }
                    if (test_announcement(agents, revision_formula)) {
                        std::cout << "GOOD Found an announcement that works\n";
                        return print_formula_dnf(revision_formula);
                    }
                }
            }
        } while (std::prev_permutation(v.begin(), v.end()));
    }

    std::cout << "No possible satisfying assignment was found\n";
    return "No possible satisfying assignment was found\n";
}

//Goals must be vector of DNF formulas
bool goals_consistent(const std::vector<std::vector<std::vector<int32_t>>>& goals) noexcept {
    std::vector<std::vector<int32_t>> conjunction;

    std::vector<std::vector<int32_t>> base_goal{goals.front()};

    for (const auto& goal : goals) {
        if (goal == goals.front()) {
            continue;
        }
        for (const auto& base_clause : base_goal) {
            for (const auto& clause : goal) {
                std::vector<int32_t> appended{base_clause};
                appended.insert(appended.end(), clause.begin(), clause.end());
                conjunction.emplace_back(std::move(appended));
            }
        }
    }

    //Conjunction will be empty when the only goal is a single term function
    if (conjunction.empty()) {
        conjunction = std::move(base_goal);
    }

    simplify_dnf(conjunction);

    if (conjunction.size() == 1) {
        //Convert single clause DNF to CNF
        std::vector<std::vector<int32_t>> converted_form;
        for (const auto clause : conjunction.front()) {
            std::vector<int32_t> cnf_clause;
            cnf_clause.emplace_back(clause);
            converted_form.emplace_back(std::move(cnf_clause));
        }
        conjunction = converted_form;
    } else {
        conjunction = convert_normal_forms(conjunction);
    }

    //simplify_dnf(conjunction);

    if (conjunction.empty()) {
        return false;
    }

    std::cout << "Goal consistency input to SAT solver:\n";
    for (const auto& clause : conjunction) {
        for (const auto term : clause) {
            std::cout << term << " ";
        }
        std::cout << "\n";
    }

    return sat(conjunction);
}

int32_t get_variable_count(const std::vector<agent>& agents) noexcept {
    int32_t max_variable_count = INT32_MIN;
    for (const auto& ag : agents) {
        for (const auto& clause : ag.beliefs) {
            for (const auto term : clause) {
                max_variable_count = std::max(max_variable_count, std::abs(term));
            }
        }
        for (const auto& clause : ag.goal) {
            for (const auto term : clause) {
                max_variable_count = std::max(max_variable_count, std::abs(term));
            }
        }
    }
    return max_variable_count;
}

bool test_announcement(const std::vector<agent>& agents,
        const std::vector<std::vector<int32_t>>& revision_formula) noexcept {
    for (const auto& agent : agents) {
        auto revised = belief_revise(agent.beliefs, revision_formula);
        const auto abs_cmp = [](const auto a, const auto b) { return std::abs(a) < std::abs(b); };
        for (auto& clause : revised) {
            std::sort(clause.begin(), clause.end(), abs_cmp);
        }
        std::sort(revised.begin(), revised.end());

        if (verbose) {
            std::cout << "\n";
            std::cout << "Agent beliefs: \n";
            print_formula_dnf(agent.beliefs);

            std::cout << "Revision formula: \n";
            print_formula_dnf(revision_formula);

            std::cout << "Revised output: \n";
            print_formula_dnf(revised);

            std::cout << "Agent goal: \n";
            print_formula_dnf(agent.goal);
        }

        for (const auto& clause : agent.goal) {
            if (std::find(revised.cbegin(), revised.cend(), clause) == revised.cend()) {
                return false;
            }
        }
    }
    return true;
}
