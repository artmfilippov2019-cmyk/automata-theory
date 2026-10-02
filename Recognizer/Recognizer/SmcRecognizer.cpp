#include "SmcRecognizer.hpp"
#include "SmcRecognizer_sm.h"

#include <cstddef>
#include <stdexcept>

SmcRecognizer::SmcRecognizer()
	: _collecting_server(true), _ok(false)
{}

void SmcRecognizer::reset() {
	_path.clear();
	_server.clear();
	_collecting_server = true;
	_ok = false;
}

void SmcRecognizer::append_path(char c) {
	if (_collecting_server) {
		if (c == '/') {
			_collecting_server = false;
		} else {
			_server += c;
		}
	}
	_path += c;
}

void SmcRecognizer::finalize() {
	_ok = true;
}

std::string SmcRecognizer::get_path() const {
	return _path;
}

bool SmcRecognizer::is_ok() const {
	return _ok;
}

std::pair<bool, std::string> SmcRecognizer::test(std::string_view input) {
	static constexpr std::size_t kHeaderLen = 6;
	static constexpr std::size_t kMaxTailLen = 63;

	if (input.size() > kHeaderLen + kMaxTailLen) {
		return {false, ""};
	}

	reset();
	try {
		SmcRecognizerContext ctx(*this);
		ctx.enterStartState();

		for (char ch : input) {
			ctx.Letter(ch);
			if (std::string_view(ctx.getState().getName()) == "map1::Error") {
				return {false, ""};
			}
		}

		ctx.EOS();

		if (!_ok) {
			return {false, ""};
		}
		if (input.size() < kHeaderLen || input.size() - kHeaderLen > kMaxTailLen) {
			return {false, ""};
		}
		return {true, _server};
	} catch (const std::exception&) {
		return {false, ""};
	}
}
