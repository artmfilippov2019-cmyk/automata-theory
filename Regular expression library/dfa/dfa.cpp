#include "dfa.h"
#include <stdexcept>
#include <algorithm>
#include <stack>

namespace automats {
    std::set<size_t> Dfa::EpsilonClosure(const Nfa& nfa, const std::set<size_t>& s) {
        std::set<size_t> closure = s;
        std::queue<size_t> q;
        for (auto x : s) {
	        q.push(x);
        }

        while (!q.empty()) {
            size_t st = q.front();
        	q.pop();
            auto it = nfa.states[st]->transitions.find('$');
            if (it == nfa.states[st]->transitions.end()) continue;
            for (auto nxt : it->second) {
	            if (!closure.count(nxt)) {
	            	closure.insert(nxt); q.push(nxt);
	            }
            }
        }
        return closure;
    }

    Dfa Dfa::FromNfa(const Nfa& nfa) {
        Dfa dfa;
        std::map<std::set<size_t>, size_t> nfa2dfa;
        std::queue<std::set<size_t>> work;

        std::set<size_t> start_set(nfa.start_states.begin(), nfa.start_states.end());
        std::set<size_t> start_closure = EpsilonClosure(nfa, start_set);

        dfa.states.push_back(std::make_shared<State>());
        nfa2dfa[start_closure] = 0;
        dfa.start_id = 0;
        work.push(start_closure);

        while (!work.empty()) {
            auto curr = work.front();
			work.pop();
            size_t dfa_id = nfa2dfa[curr];

            std::set<char> syms;
            for (auto nst : curr) {
	            for (auto &[sym, _] : nfa.states[nst]->transitions) {
	            	if (sym != '$') syms.insert(sym);
	            }
            }

            for (auto& sym : syms) {
                std::set<size_t> moved;
                for (auto nst : curr) {
                    auto it = nfa.states[nst]->transitions.find(sym);
                    if (it != nfa.states[nst]->transitions.end()) {
	                    for (auto d : it->second) moved.insert(d);
                    }
                }
                auto closure = EpsilonClosure(nfa, moved);
                if (closure.empty()) continue;

                if (!nfa2dfa.count(closure)) {
                    nfa2dfa[closure] = dfa.states.size();
                    dfa.states.push_back(std::make_shared<State>());
                    work.push(closure);
                }
                dfa.states[dfa_id]->trans[sym] = nfa2dfa[closure];
            }
        }

        std::set<size_t> nfa_terminals(nfa.terminal_states.begin(), nfa.terminal_states.end());
        for (auto& [nfa_set, dfa_id] : nfa2dfa) {
	        for (auto nst : nfa_set) {
	        	if (nfa_terminals.count(nst)) {
	        		dfa.states[dfa_id]->is_terminal = true;
	        		break;
	        	}
	        }
        }
        return dfa;
    }

    void Dfa::Minimize() {
        if (states.empty()) return;

        std::vector<char> alpha;
        std::set<char> aset;
        for (auto& s : states) {
	        for (auto& [sym, _] : s->trans) aset.insert(sym);
        }
        alpha.assign(aset.begin(), aset.end());

        std::vector<std::set<size_t>> parts;
        std::set<size_t> term, nonterm;
        for (size_t i = 0; i < states.size(); i++) {
	        (states[i]->is_terminal ? term : nonterm).insert(i);
        }
        if (!nonterm.empty()) parts.push_back(nonterm);
        if (!term.empty()) parts.push_back(term);

        std::vector<int> blk(states.size(), -1);
        bool changed = true;
        while (changed) {
            for (size_t b = 0; b < parts.size(); b++) {
	            for (auto s : parts[b]) {
	            	blk[s] = static_cast<int>(b);
	            }
            }

            changed = false;
            std::vector<std::set<size_t>> newp;
            for (auto& grp : parts) {
                if (grp.size() <= 1) {
	                newp.push_back(grp);
                	continue;
                }
                std::map<std::vector<int>, std::set<size_t>> spl;
                for (auto s : grp) {
                    std::vector<int> sig;
                    for (auto& sym : alpha) {
                        auto it = states[s]->trans.find(sym);
                        sig.push_back(it != states[s]->trans.end() ? blk[it->second] : -1);
                    }
                    spl[sig].insert(s);
                }
                if (spl.size() == 1) newp.push_back(grp);
                else {
	                for (auto& [_, g] : spl) {
		                newp.push_back(g); changed = true;
	                }
                }
            }
            parts.swap(newp);
        }

        std::map<size_t, size_t> old2new;
        for (size_t b = 0; b < parts.size(); b++) {
	        for (auto s : parts[b]) {
		        old2new[s] = b;
	        }
        }

        std::vector<std::shared_ptr<State>> ns(parts.size());
        size_t nstart = old2new[start_id];

        for (size_t b = 0; b < parts.size(); b++) {
            size_t repr = *parts[b].begin();
            ns[b] = std::make_shared<State>();
            ns[b]->is_terminal = states[repr]->is_terminal;
            for (auto& [sym, dst] : states[repr]->trans) {
	            ns[b]->trans[sym] = old2new[dst];
            }
        }

        states.swap(ns);
        start_id = nstart;
    }

	Dfa Dfa::Complement(const std::set<char>& alphabet) const {
        Dfa comp = *this;

        size_t trap = comp.states.size();
        comp.states.push_back(std::make_shared<State>());

        for (size_t i = 0; i <= trap; i++) {
	        for (auto& sym : alphabet) {
	        	if (!comp.states[i]->trans.count(sym)) {
	        		comp.states[i]->trans[sym] = trap;
	        	}
	        }
        }

        for (size_t i = 0; i < comp.states.size(); i++) {
            comp.states[i]->is_terminal = !comp.states[i]->is_terminal;
        }
        return comp;
    }

    Dfa Dfa::Intersect(const Dfa& a, const Dfa& b) {
        Dfa res;
        std::map<std::pair<size_t,size_t>, size_t> idx;
        std::queue<std::pair<size_t,size_t>> work;

        auto get = [&](size_t ai, size_t bi) -> size_t {
            auto key = std::make_pair(ai, bi);
            if (!idx.count(key)) {
                idx[key] = res.states.size();
                bool term = a.states[ai]->is_terminal && b.states[bi]->is_terminal;
                res.states.push_back(std::make_shared<State>());
                res.states.back()->is_terminal = term;
                work.push(key);
            }
            return idx[key];
        };

        res.start_id = get(a.start_id, b.start_id);

        while (!work.empty()) {
            auto [ai, bi] = work.front();
        	work.pop();
            size_t rid = idx[{ai, bi}];

            for (auto& [sym, anext] : a.states[ai]->trans) {
                auto bit = b.states[bi]->trans.find(sym);
                if (bit == b.states[bi]->trans.end()) continue;
                size_t bnext = bit->second;
                res.states[rid]->trans[sym] = get(anext, bnext);
            }
        }
        return res;
    }

    bool Dfa::IsEquivalentTo(const Dfa& other) const {
        const size_t dead_this = states.size();
        const size_t dead_other = other.states.size();

        auto is_terminal_this = [&](size_t state) -> bool {
            return state < states.size() && states[state]->is_terminal;
        };
        auto is_terminal_other = [&](size_t state) -> bool {
            return state < other.states.size() && other.states[state]->is_terminal;
        };

        auto next_this = [&](size_t state, char sym) -> size_t {
            if (state >= states.size()) return dead_this;
            auto it = states[state]->trans.find(sym);
            return it == states[state]->trans.end() ? dead_this : it->second;
        };
        auto next_other = [&](size_t state, char sym) -> size_t {
            if (state >= other.states.size()) return dead_other;
            auto it = other.states[state]->trans.find(sym);
            return it == other.states[state]->trans.end() ? dead_other : it->second;
        };

        std::set<char> alphabet = GetAlphabet();
        auto other_alphabet = other.GetAlphabet();
        alphabet.insert(other_alphabet.begin(), other_alphabet.end());

        std::set<std::pair<size_t, size_t>> visited;
        std::stack<std::pair<size_t, size_t>> dfs;
        dfs.push({start_id, other.start_id});

        while (!dfs.empty()) {
            auto [left, right] = dfs.top();
            dfs.pop();

            if (!visited.insert({left, right}).second) continue;

            if (is_terminal_this(left) != is_terminal_other(right)) {
                return false;
            }

            for (const auto& sym : alphabet) {
                size_t next_left = next_this(left, sym);
                size_t next_right = next_other(right, sym);
                if (!visited.count({next_left, next_right})) {
                    dfs.push({next_left, next_right});
                }
            }
        }

        return true;
    }

    bool Dfa::Accepts(const std::string& s) const {
        size_t cur = start_id;
        for (char c : s) {
            auto it = states[cur]->trans.find(c);
            if (it == states[cur]->trans.end()) return false;
            cur = it->second;
        }
        return states[cur]->is_terminal;
    }

	std::vector<char> Dfa::TransitionLabels(size_t from, size_t to) const {
    	std::vector<char> res;
    	for (auto& [sym, dst] : states.at(from)->trans) {
    		if (dst == to) res.push_back(sym);
    	}
    	return res;
    }

	std::set<char> Dfa::GetAlphabet() const {
    	std::set<char> a;
    	for (auto& s : states) {
    		for (auto& [sym, _] : s->trans) a.insert(sym);
    	}
    	return a;
    }

    std::vector<size_t> Dfa::FinalStates() const {
        std::vector<size_t> finals;
        finals.reserve(states.size());
        for (size_t i = 0; i < states.size(); ++i) {
            if (states[i]->is_terminal) finals.push_back(i);
        }
        return finals;
    }

    void Dfa::Print() const {
        std::cout << "DFA: " << states.size() << " states, start=" << start_id << "\n";
        for (size_t i = 0; i < states.size(); i++) {
            std::cout << "  [" << i << "]" << (states[i]->is_terminal ? " (terminal)" : "") << "\n";
            for (auto& [sym, dst] : states[i]->trans) {
	            std::cout << "    --" << sym << "--> " << dst << "\n";
            }
        }
    }

    void Dfa::ToGraphviz(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out) {
	        std::cerr << "Cannot open " << filename << "\n";
        	return;
        }
        out << "digraph DFA {\n  rankdir=LR;\n";
        out << "  node [shape=doublecircle];";
        for (auto t : FinalStates()) {
	        out << " q" << t;
        }
        out << ";\n  node [shape=circle];\n";
        out << "  init [shape=point];\n  init -> q" << start_id << ";\n";
        for (size_t i = 0; i < states.size(); i++) {
	        for (auto& [sym, dst] : states[i]->trans) {
	        	out << "  q" << i << " -> q" << dst << " [label=\"" << sym << "\"];\n";
	        }
        }
        out << "}\n";
    }
}