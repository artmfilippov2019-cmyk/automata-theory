#ifndef UNTITLED8_INPUTDIALOG_H
#define UNTITLED8_INPUTDIALOG_H

#include <optional>
#include <string>
#include <memory>
#include "Recognizer/IRecognizer.hpp"

class Input {
public:
	~Input() = default;
	std::optional<std::string> readline(std::istream& in);
	static std::istream* choose_input(std::ifstream& file);
	static std::unique_ptr<IRecognizer> choose_recognizer();
};

#endif