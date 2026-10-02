#ifndef TA1_STATISTICS_STATISTICS_HPP
#define TA1_STATISTICS_STATISTICS_HPP

#include <istream>
#include <map>
#include <string>

class IRecognizer;

struct ProcessingResult {
	std::map<std::string, int> server_usage;
	int total_lines = 0;
	int valid_lines = 0;
	int invalid_lines = 0;
};

class Statistics {
public:
	static ProcessingResult process_input(std::istream& in, IRecognizer& recognizer);
};

#endif
