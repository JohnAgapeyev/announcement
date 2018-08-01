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
#include <cstdint>
#include <vector>
#include <algorithm>
#include <iostream>
#include <climits>
#include <cassert>
#include "utils.h"
#include "tools.h"
#include "announce.h"

void find_announcement(const std::vector<agent>& agents) noexcept {
    std::vector<std::vector<std::vector<int32_t>>> goals;

    for (const auto& agent : agents) {
        goals.emplace_back(agent.goal);
    }

    if (goals_consistent(goals)) {
        std::cout << "Goals are consistent\n";
        //Need to handle/print them here
        return;
    }

    const auto max_var = get_variable_count(agents);
    assert(max_var <= 64);
    for (uint64_t i = 0; i < (1ul << (max_var - 1)); ++i) {
        std::vector<int32_t> test_case;
        for (auto j = 0; j < max_var; ++j) {
            const auto val = (1 << j) & i;
            if (val) {
                test_case.emplace_back((j + 1));
            } else {
                test_case.emplace_back(-(j + 1));
            }
        }
        //Now test_case is a n bit brute force generating DNF formulas
        //Need to do belief revision on this and compare with goals
        std::vector<std::vector<int32_t>> revision_formula;
        revision_formula.emplace_back(std::move(test_case));

        for (const auto& agent : agents) {
            auto revised = belief_revise(agent.beliefs, revision_formula);
            const auto abs_cmp = [](const auto a, const auto b){return std::abs(a) < std::abs(b);};
            for (auto& clause : revised) {
                std::sort(clause.begin(), clause.end(), abs_cmp);
            }
            std::sort(revised.begin(), revised.end());
            if (!std::includes(revised.begin(), revised.end(), agent.goal.begin(), agent.goal.end())) {
                //Revised beliefs does not include the goal
                goto bad_result;
            }
        }
        std::cout << "Found an announcement that works\n";
        for (const auto& clause : revision_formula) {
            for (const auto term : clause) {
                std::cout << term << " " << "\n";
            }
        }
        return;
bad_result:
        continue;
    }
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

    conjunction = convert_normal_forms(conjunction);

    const auto abs_cmp = [](const auto a, const auto b){return std::abs(a) < std::abs(b);};

    //Remove duplicates and empties, while sorting the 2d vector
    for (auto& clause : conjunction) {
        std::sort(clause.begin(), clause.end(), abs_cmp);
        clause.erase(std::unique(clause.begin(), clause.end()), clause.end());
    }
    std::sort(conjunction.begin(), conjunction.end());
    conjunction.erase(std::unique(conjunction.begin(), conjunction.end()), conjunction.end());
    conjunction.erase(std::remove_if(conjunction.begin(), conjunction.end(), [](const auto& clause){return clause.empty();}), conjunction.end());

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
                max_variable_count = std::max(max_variable_count, term);
            }
        }
    }
    return max_variable_count;
}
