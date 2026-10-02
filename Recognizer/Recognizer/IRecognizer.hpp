#ifndef UNTITLED8_IRECOGNIZER_H
#define UNTITLED8_IRECOGNIZER_H

#include <string>
#include <utility>

class IRecognizer {
public:
	virtual ~IRecognizer() = default;
	virtual std::pair<bool, std::string> test(std::string_view input) = 0;
};

#endif