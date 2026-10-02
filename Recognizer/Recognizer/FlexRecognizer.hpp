#ifndef RECOGNIZER_FLEXRECOGNIZER_H
#define RECOGNIZER_FLEXRECOGNIZER_H

#include <string_view>
#include "IRecognizer.hpp"

class FlexRecognizer: public IRecognizer {
	std::pair<bool, std::string> test(std::string_view input) override;
};

#endif