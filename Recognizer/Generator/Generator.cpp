#include "Generator.hpp"
#include <fstream>
#include <random>
#include <ctime>

Generator::Generator() {
    auto seed = static_cast<std::mt19937::result_type>(std::time(nullptr));
    generator.seed(seed);
}

std::string Generator::random_name(int min_len, int max_len) {
    std::uniform_int_distribution<> lenDist(min_len, max_len);
    std::uniform_int_distribution<> charDist(0, 51);

    int len = lenDist(generator);
    std::string result;
    result.reserve(len);

    for (int i = 0; i < len; ++i) {
        int c = charDist(generator);
        if (c < 26)
            result += static_cast<char>('a' + c);
        else
            result += static_cast<char>('A' + (c - 26));
    }

    return result;
}

void Generator::generate_valid_file(const std::string& filename, int count) {
    std::ofstream out(filename);

    for (int i = 0; i < count; ++i) {
        std::string body;
        std::string server = random_name(1, 10);
        std::string directory = random_name(1, 10);

        body = server + "/" + directory;

        std::uniform_int_distribution<> subDirCountDist(0, 3);
        int subCount = subDirCountDist(generator);

        for (int j = 0; j < subCount; ++j) {
            std::string subdir = random_name(1, 10);
            if (body.size() + 1 + subdir.size() <= 63)
                body += "/" + subdir;
        }

        std::uniform_int_distribution<> fileChance(0, 1);
        if (fileChance(generator) == 1) {
            std::string file = random_name(1, 10);
            if (body.size() + 1 + file.size() <= 63)
                body += "/" + file;
        }

        if (body.size() > 63) {
            body = body.substr(0, 63);
        }

        out << "nfs://" << body << "\n";
    }
}

void Generator::generate_invalid_file(const std::string& filename, int count) {
    std::ofstream out(filename);

    for (int i = 0; i < count; ++i) {
        switch (i % 6) {
            case 0:
                out << "http://server/dir\n";
                break;
            case 1:
                out << "nfs://server123/dir\n";
                break;
            case 2:
                out << "nfs:///dir\n";
                break;
            case 3:
                out << "nfs://server//dir\n";
                break;
            case 4:
                out << "nfs://server/thisdirectorynameiswaytoolongandshouldexceedsixtythreecharactersssssssssssss\n";
                break;
            case 5:
                out << "nfs://serv!er/dir\n";
                break;
        }
    }
}

std::string Generator::random_string(int length) {
	std::uniform_int_distribution<> segLenDist(1, 10);
	std::uniform_int_distribution<> charDist(0, 51);

	auto random_segment = [&](int max_len) -> std::string {
		int len = std::min(segLenDist(generator), max_len);
		std::string seg;
		seg.reserve(len);
		for (int i = 0; i < len; ++i) {
			int c = charDist(generator);
			seg += (c < 26) ? static_cast<char>('a' + c)
							: static_cast<char>('A' + c - 26);
		}
		return seg;
	};

	std::string result = "nfs://";

	result += random_segment(10);
	result += '/';
	result += random_segment(10);

	while (static_cast<int>(result.size()) < length) {
		int remaining = length - static_cast<int>(result.size()) - 1;
		if (remaining <= 0) break;
		result += '/';
		result += random_segment(std::min(remaining, 10));
	}

	while (static_cast<int>(result.size()) < length) {
		int c = charDist(generator);
		result += (c < 26) ? static_cast<char>('a' + c)
						   : static_cast<char>('A' + c - 26);
	}

	return result;
}