#ifndef RECOGNIZER_TIMECOUNTER_H
#define RECOGNIZER_TIMECOUNTER_H
#include <string>
#include "Recognizer/IRecognizer.hpp"

class TimeCounter {
public:
	static long long measure_nanoseconds(IRecognizer& recognizer, const std::string& input, int runs = 100);
};

#endif