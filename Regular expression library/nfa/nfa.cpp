#include "nfa.h"
#include <algorithm>
#include <stdexcept>
#include <fstream>
#include <iostream>

namespace automats {
	Nfa::Nfa(char symbol) {
		states.push_back(std::make_shared<State>(false));
		states.push_back(std::make_shared<State>(true));
		states[0]->transitions[symbol].push_back(1);
		start_states = {0};
		terminal_states = {1};
	}

	Nfa::Nfa(const Nfa &other) {
		states.resize(other.states.size());
		for (size_t i = 0; i < states.size(); i++) {
			states[i] = std::make_shared<State>(other.states[i]->is_terminal);
			states[i]->transitions = other.states[i]->transitions;
		}
		start_states = other.start_states;
		terminal_states = other.terminal_states;
	}

	Nfa Nfa::MakeEpsilon() {
		Nfa n;
		n.states.push_back(std::make_shared<State>(false));
		n.states.push_back(std::make_shared<State>(true));
		n.states[0]->transitions['$'].push_back(1);
		n.start_states = {0};
		n.terminal_states = {1};
		return n;
	}

	Nfa Nfa::Union(Nfa &other) {
		Nfa res;
		res.states.push_back(std::make_shared<State>(false));

		size_t off1 = res.states.size();
		for (auto &s: this->states) {
			res.states.push_back(std::make_shared<State>(s->is_terminal));
		}
		for (size_t i = 0; i < this->states.size(); i++) {
			for (auto &[sym, dsts]: this->states[i]->transitions) {
				for (auto d: dsts) {
					res.states[i + off1]->transitions[sym].push_back(d + off1);
				}
			}
		}

		size_t off2 = res.states.size();
		for (auto &s: other.states) {
			res.states.push_back(std::make_shared<State>(s->is_terminal));
		}
		for (size_t i = 0; i < other.states.size(); i++) {
			for (auto &[sym, dsts]: other.states[i]->transitions) {
				for (auto d: dsts) {
					res.states[i + off2]->transitions[sym].push_back(d + off2);
				}
			}
		}

		res.states.push_back(std::make_shared<State>(true));
		size_t acc = res.states.size() - 1;

		res.states[0]->transitions['$'].push_back(off1 + this->start_states[0]);
		res.states[0]->transitions['$'].push_back(off2 + other.start_states[0]);

		for (auto t: this->terminal_states) {
			res.states[off1 + t]->transitions['$'].push_back(acc);
		}
		for (auto t: other.terminal_states) {
			res.states[off2 + t]->transitions['$'].push_back(acc);
		}

		for (auto &s: res.states) {
			s->is_terminal = false;
		}
		res.states[acc]->is_terminal = true;

		res.start_states = {0};
		res.terminal_states = {acc};
		return res;
	}

	Nfa Nfa::Reverse() const {
		Nfa rev;

		rev.states.push_back(std::make_shared<State>(false));
		size_t new_start = 0;

		size_t offset = rev.states.size();
		for (size_t i = 0; i < states.size(); i++) {
			rev.states.push_back(std::make_shared<State>(states[i]->is_terminal));
		}
		for (size_t i = 0; i < states.size(); i++) {
			for (auto &[sym, dsts]: states[i]->transitions) {
				for (size_t dst: dsts) {
					rev.states[dst + offset]->transitions[sym].push_back(i + offset);
				}
			}
		}

		for (size_t t: terminal_states) {
			rev.states[new_start]->transitions['$'].push_back(t + offset);
		}
		rev.start_states = {new_start};
		rev.terminal_states.clear();
		for (size_t s: start_states) {
			rev.terminal_states.push_back(s + offset);
			rev.states[s + offset]->is_terminal = true;
		}

		for (size_t i = offset; i < rev.states.size(); i++) {
			if (std::find(rev.terminal_states.begin(), rev.terminal_states.end(), i) == rev.terminal_states.end()) {
				rev.states[i]->is_terminal = false;
			}
		}

		return rev;
	}

	Nfa Nfa::Concat(Nfa &other) {
		Nfa res = *this;
		size_t off = res.states.size();

		for (auto &s: other.states) {
			res.states.push_back(std::make_shared<State>(s->is_terminal));
		}
		for (size_t i = 0; i < other.states.size(); i++) {
			for (auto &[sym, dsts]: other.states[i]->transitions) {
				for (auto d: dsts) {
					res.states[i + off]->transitions[sym].push_back(d + off);
				}
			}
		}

		for (auto t: res.terminal_states) {
			res.states[t]->transitions['$'].push_back(off + other.start_states[0]);
		}

		for (auto &s: res.states) s->is_terminal = false;
		res.terminal_states.clear();
		for (auto t: other.terminal_states) {
			res.terminal_states.push_back(t + off);
			res.states[t + off]->is_terminal = true;
		}
		return res;
	}

	Nfa Nfa::KleeneStar() {
		Nfa res;
		res.states.push_back(std::make_shared<State>(false));

		size_t off = res.states.size();
		for (auto &s: this->states) {
			res.states.push_back(std::make_shared<State>(s->is_terminal));
		}
		for (size_t i = 0; i < this->states.size(); i++) {
			for (auto &[sym, dsts]: this->states[i]->transitions) {
				for (auto d: dsts) {
					res.states[i + off]->transitions[sym].push_back(d + off);
				}
			}
		}

		res.states.push_back(std::make_shared<State>(true));
		size_t acc = res.states.size() - 1;

		res.states[0]->transitions['$'].push_back(off + this->start_states[0]);
		res.states[0]->transitions['$'].push_back(acc);

		for (auto t: this->terminal_states) {
			res.states[off + t]->transitions['$'].push_back(off + this->start_states[0]);
			res.states[off + t]->transitions['$'].push_back(acc);
		}

		for (auto &s: res.states) s->is_terminal = false;
		res.states[acc]->is_terminal = true;
		res.start_states = {0};
		res.terminal_states = {acc};
		return res;
	}

	Nfa Nfa::BuildFromAst(const std::shared_ptr<Ast::Node> &node) {
		using NT = Ast::node_type;

		switch (node->type) {
			case NT::leaf: {
				if (node->label.empty()) {
					throw std::runtime_error("Пустой символ в листе AST");
				}
				return Nfa(node->label[0]);
			}

			case NT::reverse: {
				auto inner = BuildFromAst(node->left);
				auto rev = inner.Reverse();
				return inner.Concat(rev);
			}

			case NT::concat: {
				auto left = BuildFromAst(node->left);
				auto right = BuildFromAst(node->right);
				return left.Concat(right);
			}

			case NT::or_node: {
				auto left = BuildFromAst(node->left);
				auto right = BuildFromAst(node->right);
				return left.Union(right);
			}

			case NT::kleene: {
				auto inner = BuildFromAst(node->left);
				return inner.KleeneStar();
			}

			case NT::repeat: {
				int cnt = node->repeat_count;
				if (cnt <= 0) return MakeEpsilon();
				auto unit = BuildFromAst(node->left);
				Nfa res = unit;
				for (int i = 1; i < cnt; i++) {
					auto copy = BuildFromAst(node->left);
					res = res.Concat(copy);
				}
				return res;
			}

			case NT::char_class: {
				if (node->char_set.empty()) {
					throw std::runtime_error("Пустой символьный класс");
				}
				Nfa res(node->char_set[0]);
				for (size_t i = 1; i < node->char_set.size(); i++) {
					Nfa ch(node->char_set[i]);
					res = res.Union(ch);
				}
				return res;
			}

			case NT::capture:
			case NT::non_capture: {
				return BuildFromAst(node->left);
			}

			case NT::backref: {
				throw std::runtime_error("Обратные ссылки не поддерживаются");
			}

			default:
				throw std::runtime_error("Неизвестный тип узла в BuildFromAst");
		}
	}

	void Nfa::Print() const {
		std::cout << "NFA: " << states.size() << " states\n";
		for (size_t i = 0; i < states.size(); i++) {
			std::cout << "  [" << i << "]" << (states[i]->is_terminal ? " (terminal)" : "") << "\n";
			for (auto &[sym, dsts]: states[i]->transitions) {
				for (auto d: dsts) {
					std::cout << "    --" << sym << "--> " << d << "\n";
				}
			}
		}
	}

	void Nfa::ToGraphviz(const std::string &filename) const {
		std::ofstream out(filename);
		if (!out.is_open()) {
			std::cerr << "Cannot open " << filename << "\n";
			return;
		}
		out << "digraph NFA {\n  rankdir=LR;\n";
		for (size_t t: terminal_states) {
			out << "  " << t << " [shape=doublecircle];\n";
		}
		for (size_t i = 0; i < states.size(); i++) {
			for (auto &[sym, dsts]: states[i]->transitions) {
				for (auto d: dsts) {
					out << "  " << i << " -> " << d
							<< " [label=\"" << (sym == '$' ? "ε" : std::string(1, sym)) << "\"];\n";
				}
			}
		}
		out << "}\n";
	}
}