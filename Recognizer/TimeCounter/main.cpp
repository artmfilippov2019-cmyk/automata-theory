#include <iostream>
#include <fstream>
#include <iomanip>
#include "Generator/Generator.hpp"
#include "TimeCounter.hpp"
#include "Recognizer/RegexRecognizer.hpp"
#include "Recognizer/FlexRecognizer.hpp"
#include "Recognizer/SmcRecognizer.hpp"

int main() {
	Generator gen;

	RegexRecognizer rec1;
	FlexRecognizer rec2;
	SmcRecognizer rec3;

	const int length_min = 1000;
	const int length_max = 100000;
	const int length_step = 10000;
	const int runs = 1000;

	std::cout << std::setw(10) << "Length"
			  << std::setw(15) << "Regex(ns)"
			  << std::setw(15) << "Flex(ns)"
			  << std::setw(15) << "Smc(ns)"
			  << "\n";
	std::cout << std::string(55, '-') << "\n";

	std::ofstream csv("timing_results.csv");
	csv << "length,regex,flex,smc\n";

	for (int len = length_min; len <= length_max; len += length_step) {
		std::string s = gen.random_string(len);

		long long t1 = TimeCounter::measure_nanoseconds(rec1, s, runs);
		long long t2 = TimeCounter::measure_nanoseconds(rec2, s, runs);
		long long t3 = TimeCounter::measure_nanoseconds(rec3, s, runs);

		std::cout << std::setw(10) << len
				  << std::setw(15) << t1
				  << std::setw(15) << t2
				  << std::setw(15) << t3
				  << "\n";

		csv << len << "," << t1 << "," << t2 << "," << t3 << "\n";
	}

	csv.close();
	std::cout << "\nРезультаты сохранены в timing_results.csv\n";
	return 0;
}