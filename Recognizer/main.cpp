#include <iostream>
#include <memory>
#include <fstream>
#include "Recognizer/IRecognizer.hpp"
#include "Input/Input.hpp"
#include "Statistics/Statistics.hpp"

int main() {
	try {
		Input input;
		std::unique_ptr<IRecognizer> recognizer = input.choose_recognizer();

		std::ifstream file;
		std::istream* input_stream = input.choose_input(file);

		ProcessingResult result = Statistics::process_input(*input_stream, *recognizer);

		std::cout << "\nСтатистика использования имён серверов:\n";
		if (result.server_usage.empty()) {
			std::cout << "Нет корректных строк\n";
		} else {
			for (const auto& [server_name, count] : result.server_usage) {
				std::cout << server_name << ": " << count << std::endl;
			}
		}

		std::cout << "\nИтого:\n";
		std::cout << "Обработано строк: " << result.total_lines << "\n";
		std::cout << "Корректных строк: " << result.valid_lines << "\n";
		std::cout << "Некорректных строк: " << result.invalid_lines << "\n";

		if (file.is_open()) {
			file.close();
		}
	}
	catch (const std::exception& ex) {
		std::cout << "\nОшибка: " << ex.what() << std::endl;
		return 1;
	}

	return 0;
}