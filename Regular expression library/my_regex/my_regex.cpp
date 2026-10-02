#include "my_regex.h"
#include <algorithm>
#include <iostream>
#include <utility>

namespace regex {
	bool Regex::SimNode(SimCtx& ctx, const std::shared_ptr<Ast::Node>& node) {
	    using NT = Ast::node_type;
	    switch (node->type) {
	        case NT::leaf:
	            if (node->label == "$") {
		            return true;
	            }
	            if (ctx.pos < ctx.end && ctx.s[ctx.pos] == node->label[0]) {
		            ctx.pos++;
	            	return true;
	            }
	            return false;

	    	case NT::reverse: {
	    		size_t start = ctx.pos;
	    		if (!SimNode(ctx, node->left)) return false;

	    		std::string matched = ctx.s.substr(start, ctx.pos - start);
	    		std::string rev(matched.rbegin(), matched.rend());
	    		if (ctx.pos + rev.size() > ctx.end) return false;
	    		if (ctx.s.substr(ctx.pos, rev.size()) != rev) return false;
	    		ctx.pos += rev.size();
	    		return true;
	    	}

	        case NT::concat: {
	            size_t save = ctx.pos;
	            if (SimNode(ctx, node->left) && SimNode(ctx, node->right)) return true;
	            ctx.pos = save;
	            return false;
	        }

	        case NT::or_node: {
	            size_t save = ctx.pos;
	            auto gsave = ctx.groups;
	            auto hsave = ctx.has_group;
	            if (SimNode(ctx, node->left)) return true;
	            ctx.pos = save;
	            ctx.groups = gsave;
	            ctx.has_group = hsave;
	            return SimNode(ctx, node->right);
	        }

	        case NT::kleene: {
	            while (true) {
	                size_t save = ctx.pos;
	                auto gsave = ctx.groups;
	                auto hsave = ctx.has_group;
	                if (!SimNode(ctx, node->left)) {
	                    ctx.pos = save;
	                    ctx.groups = gsave;
	                    ctx.has_group = hsave;
	                    break;
	                }
	                if (ctx.pos == save) {
	                    break;
	                }
	            }
	            return true;
	        }

	        case NT::repeat: {
	            for (int i = 0; i < node->repeat_count; i++) {
	                size_t save = ctx.pos;
	                if (!SimNode(ctx, node->left)) {
		                ctx.pos = save;
	                	return false;
	                }
	            }
	            return true;
	        }

    		case NT::char_class: {
	            if (ctx.pos >= ctx.end) return false;
	            char c = ctx.s[ctx.pos];
	            for (char ch : node->char_set) {
		            if (ch == c) {
		            	ctx.pos++;
		            	return true;
		            }
	            }
	            return false;
	        }

	        case NT::capture: {
	            size_t start = ctx.pos;
	            if (!SimNode(ctx, node->left)) return false;
	            int idx = node->capture_index;
	            if (idx > 0 && idx < (int)ctx.groups.size()) {
	                ctx.groups[idx] = ctx.s.substr(start, ctx.pos - start);
	                ctx.has_group[idx] = true;
	            }
	            return true;
	        }

	        case NT::non_capture:
	            return SimNode(ctx, node->left);

	        case NT::backref: {
	            int idx = node->capture_index;
	            if (idx <= 0 || idx >= static_cast<int>(ctx.has_group.size()) || !ctx.has_group[idx]) {
		            return false;
	            }
	            const std::string& ref = ctx.groups[idx];
	            if (ctx.pos + ref.size() > ctx.end) return false;
	            if (ctx.s.substr(ctx.pos, ref.size()) == ref) {
		            ctx.pos += ref.size();
	            	return true;
	            }
	            return false;
	        }

	        default: return false;
	    }
	}

    bool AstHasBackref(const std::shared_ptr<Ast::Node>& n) {
        if (!n) return false;
        if (n->type == Ast::node_type::backref) return true;
        return AstHasBackref(n->left) || AstHasBackref(n->right);
    }

    bool AstHasReverse(const std::shared_ptr<Ast::Node>& n) {
        if (!n) return false;
        if (n->type == Ast::node_type::reverse) return true;
        return AstHasReverse(n->left) || AstHasReverse(n->right);
    }

    int MaxBackrefIndex(const std::shared_ptr<Ast::Node>& n) {
        if (!n) return 0;
        int m = 0;
        if (n->type == Ast::node_type::backref) {
	        m = n->capture_index;
        }
        return std::max({m, MaxBackrefIndex(n->left), MaxBackrefIndex(n->right)});
    }

    Regex::Regex(const std::string& pattern) : pattern_(pattern) {}

    Regex::Regex(FromDfa, automats::Dfa dfa): dfa_(std::move(dfa)), compiled_(true), dfa_available_(true) {}

    int Regex::compile(int suffix) {
        capture_count_ = 0;
        ast_ = Ast::Parser::Parse(pattern_, capture_count_);
        if (suffix >= 0) {
	        ast_.print("ast_" + std::to_string(suffix) + ".dot");
        }

        int br_max = MaxBackrefIndex(ast_.root);
        if (br_max > capture_count_) {
	        throw std::runtime_error("Backreference index exceeds number of capturing groups");
        }

        if (AstHasBackref(ast_.root) || AstHasReverse(ast_.root)) {
            dfa_available_ = false;
            compiled_ = true;
            return 0;
        }

        nfa_ = automats::Nfa::BuildFromAst(ast_.root);
        if (suffix >= 0) {
	        nfa_.ToGraphviz("nfa_" + std::to_string(suffix) + ".dot");
        }

        dfa_ = automats::Dfa::FromNfa(nfa_);
        if (suffix >= 0) {
	        dfa_.ToGraphviz("dfa_" + std::to_string(suffix) + ".dot");
        }

        dfa_.Minimize();
        if (suffix >= 0) {
	        dfa_.ToGraphviz("dfa_min_" + std::to_string(suffix) + ".dot");
        }

        dfa_available_ = true;
        compiled_ = true;
        return 0;
    }

    void Regex::EnsureCompiled() const {
        if (!compiled_) {
	        throw std::runtime_error("Regex not compiled — call compile() first");
        }
    }

    void Regex::EnsureDfa() const {
        EnsureCompiled();
        if (!dfa_available_) {
	        throw std::runtime_error("This operation needs a compiled DFA; the pattern contains backreferences");
        }
    }

    bool Regex::match(const std::string& s) const {
        if (!compiled_) {
	        const_cast<Regex *>(this)->compile(-1);
        }
        if (!dfa_available_) {
	        return SimulateNfa(s).matched;
        }
        return dfa_.Accepts(s);
    }

    bool Regex::match(const std::string& s, MatchResult& out) const {
        out = match_groups(s);
        return out.matched;
    }

    MatchResult Regex::match_groups(const std::string& s) const {
        if (!compiled_)
            const_cast<Regex *>(this)->compile(-1);

        if (dfa_available_ && capture_count_ == 0) {
            MatchResult r;
            r.matched = dfa_.Accepts(s);
            if (r.matched) {
                r.position = 0;
                r.length = s.size();
                r.groups = {s};
            }
            return r;
        }

        return SimulateNfa(s);
    }

    MatchResult Regex::SimulateNfa(const std::string& s) const {
        return SimulateNfaSpan(s, 0, s.size());
    }

    MatchResult Regex::SimulateNfaSpan(const std::string& text, size_t start, size_t end) const {
        SimCtx ctx{
            text,
            start,
            end,
            std::vector<std::string>(static_cast<size_t>(capture_count_) + 1),
            std::vector<bool>(static_cast<size_t>(capture_count_) + 1, false)};
        bool ok = SimNode(ctx, ast_.root);

        MatchResult r;
        r.matched = ok && (ctx.pos == end);
        if (r.matched) {
            r.position = start;
            r.length = end - start;
            r.groups = std::move(ctx.groups);
            if (r.groups.empty()) {
	            r.groups = {text.substr(start, end - start)};
            }
            else {
	            r.groups[0] = text.substr(start, end - start);
            }
        }
        else {
            r.groups = std::move(ctx.groups);
        }
        return r;
    }

	Regex Regex::complement() const {
        EnsureDfa();
        auto alpha = dfa_.GetAlphabet();
        auto comp_dfa = dfa_.Complement(alpha);
        comp_dfa.Minimize();
        return Regex(FromDfa{}, std::move(comp_dfa));
    }

    Regex Regex::intersect(const Regex& a, const Regex& b) {
        a.EnsureDfa();
        b.EnsureDfa();
        auto inter_dfa = automats::Dfa::Intersect(a.dfa_, b.dfa_);
        inter_dfa.Minimize();
        return Regex(FromDfa{}, std::move(inter_dfa));
    }

    bool Regex::is_equivalent_to(const Regex& other) const {
        EnsureDfa();
        other.EnsureDfa();
        return dfa_.IsEquivalentTo(other.dfa_);
    }

    std::optional<std::string> Regex::unite(const std::optional<std::string>& a, const std::optional<std::string>& b) {
        if (!a) return b;
        if (!b) return a;
        if (*a == *b) return a;
        return "(" + *a + "|" + *b + ")";
    }

    std::optional<std::string> Regex::concat(const std::optional<std::string>& a, const std::optional<std::string>& b) {
        if (!a || !b) return std::nullopt;
        if (*a == "$") return b;
        if (*b == "$") return a;
        return *a + *b;
    }

    std::optional<std::string> Regex::star(const std::optional<std::string>& a) {
        if (!a || *a == "$") return std::string("$");
        if (a->size() == 1) return *a + "...";
        return "(" + *a + ")...";
    }

    std::string Regex::recover() const {
        EnsureDfa();

        int N = dfa_.NumStates();
        int start = (int)dfa_.StartState();
        auto finals = dfa_.FinalStates();
        std::vector<bool> is_final(N, false);
        for (auto f : finals) is_final[f] = true;

        using Expr = std::optional<std::string>;
        using Vec3 = std::vector<std::vector<std::vector<Expr>>>;
        Vec3 R(N + 1, std::vector<std::vector<Expr>>(N, std::vector<Expr>(N, std::nullopt)));

        for (int i = 0; i < N; i++) {
	        for (int j = 0; j < N; j++) {
	        	auto syms = dfa_.TransitionLabels(i, j);
	        	Expr r = std::nullopt;
	        	for (auto lbl : syms) r = unite(r, Expr(std::string(1, lbl)));
	        	if (i == j) r = unite(r, Expr(std::string("$")));
	        	R[0][i][j] = r;
	        }
        }

        for (int k = 1; k <= N; k++) {
            for (int i = 0; i < N; i++) {
                for (int j = 0; j < N; j++) {
                    Expr r = R[k - 1][i][j];
                    const auto& rik = R[k - 1][i][k - 1];
                    const auto& rkk = R[k - 1][k - 1][k - 1];
                    const auto& rkj = R[k - 1][k - 1][j];
                    auto through_k = concat(rik, concat(star(rkk), rkj));
                    r = unite(r, through_k);
                    R[k][i][j] = r;
                }
            }
        }

        Expr result = std::nullopt;
        for (int f = 0; f < N; f++) {
	        if (is_final[f]) result = unite(result, R[N][start][f]);
        }

        return result.value_or("");
    }

    void Regex::print_dfa(const std::string& filename) const {
        EnsureDfa();
        dfa_.ToGraphviz(filename);
    }
}
