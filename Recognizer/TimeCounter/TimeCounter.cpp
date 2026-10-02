#include "TimeCounter.hpp"
#include <chrono>

long long TimeCounter::measure_nanoseconds(IRecognizer& recognizer, const std::string& input, int runs) {
	using namespace std::chrono;
	auto start = steady_clock::now();

	for (int i = 0; i < runs; ++i) {
		recognizer.test(input);
	}

	auto end = steady_clock::now();
	return duration_cast<nanoseconds>(end - start).count() / runs;
}