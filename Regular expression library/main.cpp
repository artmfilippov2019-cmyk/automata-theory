#include <iostream>
#include <string>
#include <cstdlib>
#include <filesystem>
#include "ast_builder/ast_builder.h"
#include "nfa/nfa.h"
#include "dfa/dfa.h"
#include "my_regex/my_regex.h"

namespace {
	bool RenderDotToPngIfExists(const std::string& dotFile, const std::string& pngFile) {
	    if (!std::filesystem::exists(dotFile)) {
	        return false;
	    }

	    const std::string cmd = "dot -Tpng \"" + dotFile + "\" -o \"" + pngFile + "\"";
	    const int rc = std::system(cmd.c_str());
	    if (rc != 0) {
	        throw std::runtime_error("Не удалось отрисовать " + dotFile);
	    }
	    std::cout << "wrote: " << pngFile << "\n";
	    return true;
	}
}

int main() {
    std::ios::sync_with_stdio(false);
    std::cin.tie(nullptr);

    std::string expr;
    if (!std::getline(std::cin, expr)) {
        std::cerr << "Expected regex expression on stdin.\n";
        return 2;
    }
    std::string text;
    std::getline(std::cin, text);

    try {
        int captureCount = 0;
        Ast::AST ast = Ast::Parser::Parse(expr, captureCount);
        // automats::Nfa nfa = automats::Nfa::BuildFromAst(ast.root);
        // automats::Dfa dfa = automats::Dfa::FromNfa(nfa);
        // automats::Dfa dfaMin = dfa;
        // dfaMin.Minimize();

        const std::string dotFile = "ast.dot";
        const std::string pngFile = "ast.png";

        ast.print(dotFile);
    	// automats::Nfa rev = nfa.Reverse();
    	// rev.ToGraphviz("rev.dot");
     //    nfa.ToGraphviz("nfa.dot");
     //    dfa.ToGraphviz("dfa.dot");
     //    dfaMin.ToGraphviz("dfa_min.dot");

        std::cout << "captures: " << captureCount << "\n";
        std::cout << "wrote: " << dotFile << "\n";

        RenderDotToPngIfExists(dotFile, pngFile);
        RenderDotToPngIfExists("nfa.dot", "nfa.png");
        RenderDotToPngIfExists("dfa.dot", "dfa.png");
        RenderDotToPngIfExists("dfa_min.dot", "dfa_min.png");

        regex::Regex re(expr);
        re.compile(-1);
        std::cout << "match(\"" << text << "\"): " << (re.match(text) ? "true" : "false") << "\n";

        return 0;
    } catch (const std::exception& e) {
        std::cerr << "Ошибка: " << e.what() << "\n";
        return 1;
    }
}
