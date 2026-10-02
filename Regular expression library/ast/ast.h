#pragma once

#include <memory>
#include <string>
#include <vector>
#include <iostream>
#include <stdexcept>

namespace Ast {
    enum class node_type {
        root,
        leaf,
        concat,
        kleene,
        or_node,
        repeat,
        char_class,
        capture,
        backref,
        non_capture,
    	reverse
    };

    std::string NodeTypeToString(node_type type);

    struct Node {
        node_type type;
        std::string label;
        int repeat_count;
        int capture_index;
        std::vector<char> char_set;

        std::shared_ptr<Node> left;
        std::shared_ptr<Node> right;

        Node(const node_type t, const std::string& lbl = "")
            : type(t), label(lbl), repeat_count(0), capture_index(-1), left(nullptr), right(nullptr) {}
    };

    class AST {
    public:
        std::shared_ptr<Node> root;

        AST() : root(nullptr) {}
        explicit AST(std::shared_ptr<Node> n) : root(n) {}

        void print(const std::string& filename = "ast.dot") const;

    private:
        int DfsPrint(const std::shared_ptr<Node>& node, int& counter, std::ostream& out) const;
    };
}
