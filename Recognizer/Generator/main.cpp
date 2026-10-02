#include "Generator.hpp"
#include <iostream>

int main() {
	try {
		Generator gen;

		gen.generate_valid_file("valid_nfs_paths.txt", 100);
		std::cout << "Файл valid_nfs_paths.txt успешно создан с 100 корректными строками" << std::endl;

		gen.generate_invalid_file("invalid_nfs_paths.txt", 100);
		std::cout << "Файл invalid_nfs_paths.txt успешно создан с 100 некорректными строками" << std::endl;

		std::cout << "\nВсе файлы успешно сгенерированы!" << std::endl;
	} catch (const std::exception& e) {
		std::cerr << "Ошибка при генерации файлов: " << e.what() << std::endl;
		return 1;
	}

	return 0;
}