#include "ast_builder.h"
#include <stdexcept>
#include <cctype>

namespace Ast {
    bool Parser::IsMetaChar(char c) {
        return c == '|' || c == '.' || c == '{' || c == '}' ||
               c == '[' || c == ']' || c == '(' || c == ')' ||
               c == '\\' || c == '$';
    }

    char Parser::CurrentChar(const Context& ctx) {
        if (ctx.pos >= ctx.src.size()) return '\0';
        return ctx.src[ctx.pos];
    }

    bool Parser::HasMore(const Context& ctx) {
        return ctx.pos < ctx.src.size();
    }

    void Parser::Consume(Context& ctx, char expected) {
        if (ctx.pos >= ctx.src.size() || ctx.src[ctx.pos] != expected) {
	        throw std::runtime_error(std::string("Ожидался символ '") + expected + "' в позиции " + std::to_string(ctx.pos));
        }
        ctx.pos++;
    }

    AST Parser::Parse(const std::string& expr, int& capture_count) {
        capture_count = 0;
        Context ctx{expr, 0, capture_count};
        auto root = ParseExpr(ctx);
        if (HasMore(ctx)) {
	        throw std::runtime_error("Неожиданный символ в позиции " + std::to_string(ctx.pos));
        }
        return AST(root);
    }

    std::shared_ptr<Node> Parser::ParseExpr(Context& ctx) {
        auto left = ParseTerm(ctx);
        while (HasMore(ctx) && CurrentChar(ctx) == '|') {
            ctx.pos++;
            auto right = ParseTerm(ctx);
            auto node = std::make_shared<Node>(node_type::or_node, "|");
            node->left = left;
            node->right = right;
            left = node;
        }
        return left;
    }

    std::shared_ptr<Node> Parser::ParseTerm(Context& ctx) {
        auto isTermStart = [&]() {
            if (!HasMore(ctx)) return false;
            char c = CurrentChar(ctx);
            return c != ')' && c != '|';
        };

        std::shared_ptr<Node> left = nullptr;
        while (isTermStart()) {
            auto f = ParseFactor(ctx);
            if (!left) { left = f; continue; }
            auto node = std::make_shared<Node>(node_type::concat, ".");
            node->left = left;
            node->right = f;
            left = node;
        }
        if (!left) {
            auto eps = std::make_shared<Node>(node_type::leaf, "$");
            return eps;
        }
        return left;
    }

    std::shared_ptr<Node> Parser::ParseFactor(Context& ctx) {
        auto atom = ParseAtom(ctx);

        while (HasMore(ctx)) {
            if (ctx.pos + 2 < ctx.src.size() && ctx.src[ctx.pos] == '.' && ctx.src[ctx.pos+1] == '.' &&
            	ctx.src[ctx.pos+2] == '.') {
                ctx.pos += 3;
                auto node = std::make_shared<Node>(node_type::kleene, "...");
                node->left = atom;
                atom = node;
                continue;
            }

            if (CurrentChar(ctx) == '{') {
                ctx.pos++;
                std::string num;
                while (HasMore(ctx) && CurrentChar(ctx) != '}') {
                    num += CurrentChar(ctx);
                    ctx.pos++;
                }
                Consume(ctx, '}');
                if (num.empty()) {
	                throw std::runtime_error("Пустой счётчик повторений {}");
                }
                for (char ch : num) {
                    if (!std::isdigit(static_cast<unsigned char>(ch))) {
                        throw std::runtime_error("Некорректный счётчик повторений {" + num +
                                                 "}: ожидается неотрицательное целое число");
                    }
                }
                int cnt = 0;
                try {
                    cnt = std::stoi(num);
                } catch (const std::exception&) {
                    throw std::runtime_error("Слишком большой счётчик повторений {" + num + "}");
                }
                auto node = std::make_shared<Node>(node_type::repeat, "{}");
                node->repeat_count = cnt;
                node->left = atom;
                atom = node;
                continue;
            }
        	if (CurrentChar(ctx) == '~') {
        		ctx.pos++;
        		auto node = std::make_shared<Node>(node_type::reverse, "~");
        		node->left = atom;
        		atom = node;
        		continue;
        	}

            break;
        }
        return atom;
    }

    std::shared_ptr<Node> Parser::ParseAtom(Context& ctx) {
        if (!HasMore(ctx)) {
	        throw std::runtime_error("Неожиданный конец выражения");
        }

        char c = CurrentChar(ctx);

        if (c == '\\') {
            ctx.pos++;
            if (!HasMore(ctx)) {
	            throw std::runtime_error("Обратная косая черта в конце выражения");
            }
            if (CurrentChar(ctx) >= '1' && CurrentChar(ctx) <= '9') {
                long long idx = 0;
                while (HasMore(ctx) && CurrentChar(ctx) >= '0' && CurrentChar(ctx) <= '9') {
                    idx = idx * 10 + (CurrentChar(ctx) - '0');
                    if (idx > 1'000'000)
                        throw std::runtime_error("Слишком большой индекс обратной ссылки");
                    ctx.pos++;
                }
                if (idx < 1) {
	                throw std::runtime_error("Некорректная обратная ссылка");
                }
                auto node = std::make_shared<Node>(node_type::backref, "\\" + std::to_string(idx));
                node->capture_index = static_cast<int>(idx);
                return node;
            }

            char next = CurrentChar(ctx);
            ctx.pos++;

            auto node = std::make_shared<Node>(node_type::leaf, std::string(1, next));
            return node;
        }

        if (c == '$') {
            ctx.pos++;
            return std::make_shared<Node>(node_type::leaf, "$");
        }

        if (c == '[') {
	        ctx.pos++;
        	auto node = std::make_shared<Node>(node_type::char_class, "[]");
        	while (HasMore(ctx) && CurrentChar(ctx) != ']') {
                char ch = CurrentChar(ctx);
                ctx.pos++;
                if (ch == '\\' && HasMore(ctx)) {
                    ch = CurrentChar(ctx);
                    ctx.pos++;
                }
                node->char_set.push_back(ch);
            }
            Consume(ctx, ']');
            if (node->char_set.empty()) {
	            throw std::runtime_error("Пустой символьный класс []");
            }
            return node;
        }

        if (c == '(') {
            ctx.pos++;

            if (HasMore(ctx) && CurrentChar(ctx) == ':') {
                ctx.pos++;
                auto inner = ParseExpr(ctx);
                Consume(ctx, ')');
                auto node = std::make_shared<Node>(node_type::non_capture, "(:)");
                node->left = inner;
                return node;
            }

            int my_index = ++ctx.capture_count;
            auto inner = ParseExpr(ctx);
            Consume(ctx, ')');
            auto node = std::make_shared<Node>(node_type::capture, "()");
            node->capture_index = my_index;
            node->left = inner;
            return node;
        }

        ctx.pos++;
        return std::make_shared<Node>(node_type::leaf, std::string(1, c));
    }
}
