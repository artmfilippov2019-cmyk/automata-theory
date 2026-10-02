#pragma once
#include <string>
#include <vector>
#include <optional>
#include <stdexcept>
#include "../ast/ast.h"
#include "../ast_builder/ast_builder.h"
#include "../nfa/nfa.h"
#include "../dfa/dfa.h"

namespace regex {
	class MatchResult {
	public:
		bool matched = false;

		size_t position = std::string::npos;
		size_t length = 0;

		std::vector<std::string> groups;

		explicit operator bool() const { return matched; }

		const std::string& operator[](size_t idx) const {
			if (idx >= groups.size())
				throw std::out_of_range("Group index out of range");
			return groups[idx];
		}

		[[nodiscard]] size_t size() const { return groups.size(); }

		[[nodiscard]] auto begin() const { return groups.begin(); }
		[[nodiscard]] auto end()   const { return groups.end(); }
	};

	class Regex {
	public:
		struct FromDfa {};

		explicit Regex(const std::string& pattern);
		Regex(FromDfa, automats::Dfa dfa);

		int compile(int dot_suffix = 0);

		[[nodiscard]] bool match(const std::string& s) const;
		bool match(const std::string& s, MatchResult& out) const;

		[[nodiscard]] MatchResult match_groups(const std::string& s) const;
		[[nodiscard]] Regex complement() const;
		static Regex intersect(const Regex& a, const Regex& b);
		[[nodiscard]] bool is_equivalent_to(const Regex& other) const;
		[[nodiscard]] std::string recover() const;

		void print_dfa(const std::string& filename = "dfa.dot") const;

	private:
		struct SimCtx {
			const std::string& s;
			size_t pos;
			size_t end;
			std::vector<std::string> groups;
			std::vector<bool> has_group;
		};

		std::string pattern_;
		int capture_count_ = 0;
		bool compiled_ = false;
		bool dfa_available_ = false;

		Ast::AST ast_;
		automats::Nfa nfa_;
		automats::Dfa dfa_;

		static bool SimNode(SimCtx& ctx, const std::shared_ptr<Ast::Node>& node);
		static std::optional<std::string> unite(const std::optional<std::string>& a, const std::optional<std::string>& b);
		static std::optional<std::string> concat(const std::optional<std::string>& a, const std::optional<std::string>& b);
		static std::optional<std::string> star(const std::optional<std::string>& a);
		[[nodiscard]] MatchResult SimulateNfa(const std::string& s) const;
		[[nodiscard]] MatchResult SimulateNfaSpan(const std::string& text, size_t start, size_t end) const;

		void EnsureCompiled() const;
		void EnsureDfa() const;
	};
}
