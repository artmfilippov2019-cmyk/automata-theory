#include "Statistics/Statistics.hpp"

#include <iostream>

#include "Input/Input.hpp"
#include "Recognizer/IRecognizer.hpp"

ProcessingResult Statistics::process_input(std::istream& in, IRecognizer& recognizer) {
	ProcessingResult result;

	Input input;
	while (true) {
		auto word_opt = input.readline(in);
		if (!word_opt.has_value()) break;

		const std::string& str = word_opt.value();
		result.total_lines++;

		if (str.empty()) {
			std::cout << "Пропущена пустая строка\n";
			continue;
		}

		auto [flag, server_name] = recognizer.test(str);

		if (flag) {
			std::cout << str << " - корректная строка (сервер: " << server_name << ")\n";
			result.server_usage[server_name]++;
			result.valid_lines++;
		} else {
			std::cout << str << " - некорректная строка\n";
			result.invalid_lines++;
		}
	}

	return result;
}
