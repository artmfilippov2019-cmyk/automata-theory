#pragma once

#include <map>
#include <memory>
#include <vector>
#include <string>
#include "../ast/ast.h"

namespace automats {
    class Nfa {
        friend class Dfa;

    public:
        Nfa() = default;
        explicit Nfa(char symbol);
        Nfa(const Nfa& other);
    	Nfa Reverse() const;

        static Nfa BuildFromAst(const std::shared_ptr<Ast::Node>& node);

        void ToGraphviz(const std::string& filename = "nfa.dot") const;
        void Print() const;

    protected:
        struct State {
            bool is_terminal = false;
            std::map<char, std::vector<size_t>> transitions;

            explicit State(bool term = false) : is_terminal(term) {}
        };

        std::vector<std::shared_ptr<State>> states;
        std::vector<size_t> start_states;
        std::vector<size_t> terminal_states;

        static Nfa MakeEpsilon();
        Nfa Union(Nfa& other);
        Nfa Concat(Nfa& other);
        Nfa KleeneStar();
    };
}
