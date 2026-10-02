#pragma once

#include "../ast/ast.h"
#include <string>

namespace Ast {
    class Parser {
    public:
        static AST Parse(const std::string& expr, int& capture_count);

    private:
        struct Context {
            const std::string& src;
            size_t pos;
            int& capture_count;
        };

        static std::shared_ptr<Node> ParseExpr(Context& ctx);
        static std::shared_ptr<Node> ParseTerm(Context& ctx);
        static std::shared_ptr<Node> ParseFactor(Context& ctx);
        static std::shared_ptr<Node> ParseAtom(Context& ctx);

        static bool IsMetaChar(char c);
        static char CurrentChar(const Context& ctx);
        static bool HasMore(const Context& ctx);
        static void Consume(Context& ctx, char expected);
    };
}
