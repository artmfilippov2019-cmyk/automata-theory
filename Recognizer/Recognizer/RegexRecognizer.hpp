#ifndef RECOGNIZER_REGEXRECOGNIZER_H
#define RECOGNIZER_REGEXRECOGNIZER_H
#include "IRecognizer.hpp"
#include <boost/regex.hpp>

class RegexRecognizer: public IRecognizer {
public:
	std::pair<bool, std::string> test(std::string_view input) override;
};

#endif