#include "RegexRecognizer.hpp"
#include <regex>

static const boost::regex nfsPattern(
	R"(^nfs://([a-zA-Z]+)(?:/[a-zA-Z]+)+$)",
	boost::regex::optimize
);

std::pair<bool, std::string> RegexRecognizer::test(std::string_view input) {
	std::string str(input);

	const std::string header = "nfs://";
	if (str.length() <= header.length()) {
		return {false, ""};
	}

	if (str.length() - header.length() > 63) {
		return {false, ""};
	}

	boost::smatch match;
	if (!boost::regex_match(str, match, nfsPattern))
		return {false, ""};

	return {true, match[1]};
}