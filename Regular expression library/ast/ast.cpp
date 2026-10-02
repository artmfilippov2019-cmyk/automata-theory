#include "ast.h"
#include <fstream>

namespace Ast {
    std::string NodeTypeToString(node_type type) {
        switch (type) {
            case node_type::root: return "root";
            case node_type::leaf: return "leaf";
            case node_type::concat: return "concat";
            case node_type::kleene: return "kleene(...)";
            case node_type::or_node: return "or(|)";
            case node_type::repeat: return "repeat";
            case node_type::char_class: return "char_class";
            case node_type::capture: return "capture";
            case node_type::backref: return "backref";
            case node_type::non_capture: return "non_capture";
        	case node_type::reverse: return "reverse";
        }
        return "unknown";
    }

    int AST::DfsPrint(const std::shared_ptr<Node>& node, int& counter, std::ostream& out) const {
        if (!node) return -1;
        int id = counter++;

        out << "  node" << id << " [label=\"" << NodeTypeToString(node->type);
        if (!node->label.empty()) {
	        out << "\\n" << node->label;
        }
        if (node->type == node_type::repeat) {
	        out << "\\n{" << node->repeat_count << "}";
        }
        if (node->type == node_type::capture || node->type == node_type::backref) {
	        out << "\\n#" << node->capture_index;
        }
        if (node->type == node_type::char_class) {
            out << "\\n[";
            for (char c : node->char_set) out << c;
            out << "]";
        }
        out << "\"];\n";

        if (node->left) {
            int lid = DfsPrint(node->left, counter, out);
            out << "  node" << id << " -> node" << lid << ";\n";
        }
        if (node->right) {
            int rid = DfsPrint(node->right, counter, out);
            out << "  node" << id << " -> node" << rid << ";\n";
        }
        return id;
    }

    void AST::print(const std::string& filename) const {
        std::ofstream out(filename);
        if (!out.is_open()) {
	        throw std::runtime_error("Не удалось открыть: " + filename);
        }
        out << "digraph AST {\n";
        int c = 0;
        DfsPrint(root, c, out);
        out << "}\n";
    }
}
