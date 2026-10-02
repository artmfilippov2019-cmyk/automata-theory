#ifndef RECOGNIZER_GENERATOR_H
#define RECOGNIZER_GENERATOR_H

#include <string>
#include <random>

class Generator {
public:
	Generator();
	void generate_valid_file(const std::string& filename, int count);
	void generate_invalid_file(const std::string& filename, int count);
	std::string random_string(int length);
private:
	std::mt19937 generator;
	std::random_device random_device;
	std::string random_name(int min_len = 1, int max_len = 10);
};

#endif