#ifndef RECOGNIZER_SMCRECOGNIZER_H
#define RECOGNIZER_SMCRECOGNIZER_H
#include <string_view>
#include "IRecognizer.hpp"


class SmcRecognizerContext;

class SmcRecognizer : public IRecognizer {
public:
	SmcRecognizer();
	std::pair<bool, std::string> test(std::string_view input) override;

	void reset();
	void append_path(char c);
	void finalize();

	[[nodiscard]] std::string get_path() const;
	[[nodiscard]] bool is_ok() const;

private:
	std::string _path;
	std::string _server;
	bool _collecting_server;
	bool _ok;
};
#endif