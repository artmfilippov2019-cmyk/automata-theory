#pragma once

#include <map>
#include <set>
#include <vector>
#include <memory>
#include <string>
#include <fstream>
#include <iostream>
#include <queue>
#include "../nfa/nfa.h"

namespace automats {
    class Dfa {
    public:
        Dfa() = default;
        Dfa(const Dfa&) = default;
        Dfa& operator=(const Dfa&) = default;

        static Dfa FromNfa(const Nfa& nfa);
        [[nodiscard]] Dfa Complement(const std::set<char>& alphabet) const;
        static Dfa Intersect(const Dfa& a, const Dfa& b);
        [[nodiscard]] bool IsEquivalentTo(const Dfa& other) const;

        void Minimize();

        [[nodiscard]] bool Accepts(const std::string& s) const;
        [[nodiscard]] std::vector<char> TransitionLabels(size_t from, size_t to) const;
        [[nodiscard]] std::set<char> GetAlphabet() const;
        [[nodiscard]] size_t NumStates() const { return states.size(); }
        [[nodiscard]] size_t StartState() const { return start_id; }
        [[nodiscard]] std::vector<size_t> FinalStates() const;

        void ToGraphviz(const std::string& filename = "dfa.dot") const;
        void Print() const;

    private:
        struct State {
            bool is_terminal = false;
            std::map<char, size_t> trans;
        };

        std::vector<std::shared_ptr<State>> states;
        size_t start_id = 0;

        static std::set<size_t> EpsilonClosure(const Nfa& nfa, const std::set<size_t>& s);
    };
}
