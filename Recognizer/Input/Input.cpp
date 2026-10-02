#include "Input.hpp"
#include <iostream>
#include <memory>
#include <fstream>
#include <limits>
#include "Recognizer/IRecognizer.hpp"
#include "Recognizer/RegexRecognizer.hpp"
#include "Recognizer/SmcRecognizer.hpp"
#include "Recognizer/FlexRecognizer.hpp"

std::optional<std::string> Input::readline(std::istream& in) {
	if (&in == &std::cin) {
		std::cout << "Введите слово (или напишите 'exit' для завершения): ";
	}
	std::string line;
	if (!std::getline(in, line)) {
		if (in.eof()) {
			//throw std::runtime_error("EOF");
			return std::nullopt;
		}
		if (in.bad()) {
			throw std::runtime_error("Критическая ошибка потока");
		}
	}
	if (line == "exit") {
		return std::nullopt;
	}
	if (line.empty()) {
		throw std::invalid_argument("Ошибка: пустая строка");
	}

	return line;
}

std::unique_ptr<IRecognizer> Input::choose_recognizer() {
	int choice;

	std::cout << "Выберите распознаватель:\n";
	std::cout << "1 - Regex\n";
	std::cout << "2 - Smc\n";
	std::cout << "3 - Flex\n";
	std::cout << "Ваш выбор: ";
	std::cin >> choice;

	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	switch (choice) {
		case 1:
			return std::make_unique<RegexRecognizer>();
		case 2:
			return std::make_unique<SmcRecognizer>();
		case 3:
			return std::make_unique<FlexRecognizer>();
		default:
			throw std::invalid_argument("Неверный выбор распознавателя");
	}
}

std::istream* Input::choose_input(std::ifstream& file) {
	int choice;

	std::cout << "\nВыберите источник ввода:\n";
	std::cout << "1 - Консоль\n";
	std::cout << "2 - Файл\n";
	std::cout << "Ваш выбор: ";
	std::cin >> choice;

	std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');

	if (choice == 1) {
		return &std::cin;
	}
	if (choice == 2) {
		std::string filename;
		std::cout << "Введите имя файла: ";
		std::getline(std::cin, filename);

		file.open(filename);
		if (!file.is_open()) {
			throw std::runtime_error("Не удалось открыть файл: " + filename);
		}

		return &file;
	}
	throw std::invalid_argument("Неверный выбор источника ввода");
}
