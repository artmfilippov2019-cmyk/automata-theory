#include <array>
#include <string>
#include <string_view>
#include <vector>
#include <catch2/catch_all.hpp>
#include "../my_regex/my_regex.h"

namespace {
    struct MatchCase {
        std::string_view text;
        bool expected;
    };

    void CheckMatchCases(const std::string& pattern, const std::array<MatchCase, 3>& cases) {
        regex::Regex re(pattern);
        re.compile(-1);
        for (const auto& [text, expected] : cases) {
            INFO("pattern=" << pattern << ", text=" << text);
            CHECK(re.match(std::string(text)) == expected);
        }
    }
}

TEST_CASE("Meta: ~, test1", "[meta][match]") {
	CheckMatchCases("(abc)~", {{
		{"abccba", true},
		{"abcabc", false},
		{"abc", false}
		}});
}

TEST_CASE("Meta: ~, test2", "[meta][match]") {
	CheckMatchCases("(ab|ac)~", {{
			{"abba", true},
			{"acca", true},
			{"abca", false}
		}});
}

TEST_CASE("Meta: union | works via match", "[meta][match]") {
    CheckMatchCases("a|b", {{
        {"a", true},
        {"b", true},
        {"ab", false}
    }});
}

TEST_CASE("Meta: kleene ... works via match", "[meta][match]") {
    CheckMatchCases("a...", {{
        {"", true},
        {"aa", true},
        {"b", false}
    }});
}

TEST_CASE("Meta: repeat {n} works via match", "[meta][match]") {
    CheckMatchCases("ab{2}", {{
        {"abb", true},
        {"ab", false},
        {"abbb", false}
    }});
}

TEST_CASE("Meta: char class [] works via match", "[meta][match]") {
    CheckMatchCases("[abc]", {{
        {"a", true},
        {"c", true},
        {"d", false}
    }});
}

TEST_CASE("Meta: capture () works via match", "[meta][match]") {
    CheckMatchCases("(ab)c", {{
        {"abc", true},
        {"ab", false},
        {"zabc", false}
    }});
}

TEST_CASE("Meta: non-capture (:) works via match", "[meta][match]") {
    CheckMatchCases("(:ab)c", {{
        {"abc", true},
        {"ac", false},
        {"abcc", false}
    }});
}

TEST_CASE("Meta: epsilon $ works via match", "[meta][match]") {
    CheckMatchCases("$", {{
        {"", true},
        {"a", false},
        {"aa", false}
    }});
}

TEST_CASE("Meta: escaped char \\\\ works via match", "[meta][match]") {
    CheckMatchCases(R"(\|)", {{
        {"|", true},
        {"a", false},
        {"||", false}
    }});
}

TEST_CASE("Meta: combinations of multiple metacharacters work via match", "[meta][match]") {
    CheckMatchCases("((:ab)|c)...", {{
        {"", true},
        {"ab", true},
        {"ca", false}
    }});

    CheckMatchCases(R"((\||a){2})", {{
        {"a|", true},
        {"||", true},
        {"a", false}
    }});

    CheckMatchCases("[ab]{3}...", {{
        {"", true},
        {"aba", true},
        {"ab", false}
    }});

    CheckMatchCases("(a|b)(:cd)", {{
        {"acd", true},
        {"bcd", true},
        {"ac", false}
    }});

    CheckMatchCases("(:a|b){2}c", {{
        {"aac", true},
        {"bbc", true},
        {"ac", false}
    }});

    CheckMatchCases("$a|b", {{
        {"a", true},
        {"b", true},
        {"", false}
    }});

    CheckMatchCases("(a[bc]){2}...", {{
        {"", true},
        {"abab", true},
        {"ab", false}
    }});

    CheckMatchCases("[ab]|$", {{
        {"", true},
        {"a", true},
        {"z", false}
    }});

    CheckMatchCases("c[ab]{2}...", {{
        {"cab", true},
        {"caba", false},
        {"ca", false}
    }});

    CheckMatchCases("a|(bb){2}", {{
        {"bbbb", true},
        {"a", true},
        {"bb", false}
    }});

    CheckMatchCases("(ab|(:cd))...", {{
        {"", true},
        {"abcd", true},
        {"c", false}
    }});

    CheckMatchCases(R"((\||a){2}|$)", {{
        {"", true},
        {"aa", true},
        {"|", false}
    }});
}

TEST_CASE("MatchResult exposes capture groups via index and iteration", "[match][groups]") {
    regex::Regex re("(a)(b)");
    regex::MatchResult result;

    REQUIRE(re.match("ab", result));
    CHECK(result.matched);
    CHECK(result.position == 0);
    CHECK(result.length == 2);
    REQUIRE(result.size() == 3);

    CHECK(result[0] == "ab");
    CHECK(result[1] == "a");
    CHECK(result[2] == "b");

    std::vector<std::string> iterated(result.begin(), result.end());
    REQUIRE(iterated.size() == 3);
    CHECK(iterated[0] == "ab");
    CHECK(iterated[1] == "a");
    CHECK(iterated[2] == "b");
}

TEST_CASE("MatchResult index out of range throws", "[match][groups]") {
    regex::Regex re("(ab)");
    auto result = re.match_groups("ab");
    REQUIRE(result.matched);
    REQUIRE(result.size() == 2);
    CHECK_THROWS_AS(result[2], std::out_of_range);
}

TEST_CASE("match gives same result with and without explicit compile", "[compile][match]") {
    regex::Regex lazy("(ab)c");
    regex::Regex eager("(ab)c");
    eager.compile(-1);

    const std::array<MatchCase, 4> cases{{
        {"abc", true},
        {"ab", false},
        {"xabc", false},
        {"abcc", false}
    }};

    for (const auto& [text, expected] : cases) {
        INFO("text=" << text);
        CHECK(lazy.match(std::string(text)) == expected);
        CHECK(eager.match(std::string(text)) == expected);
    }
}

TEST_CASE("Recovered regex is equivalent to compiled DFA language", "[automaton][recover]") {
    regex::Regex source("a|bb");
    source.compile(-1);

    std::string recovered_pattern = source.recover();
    regex::Regex recovered(recovered_pattern);
    recovered.compile(-1);

    CHECK(source.is_equivalent_to(recovered));
}

TEST_CASE("Automaton operation: complement gives expected language", "[automaton][operations]") {
    regex::Regex source("[ab]");
    source.compile(-1);
    auto actual = source.complement();

    regex::Regex expected("$|[ab][ab][ab]...");
    expected.compile(-1);

    CHECK(actual.is_equivalent_to(expected));
}

TEST_CASE("Automaton operation: complement excludes length-1", "[automaton][operations]") {
    regex::Regex source("a|b");
    source.compile(-1);
    auto actual = source.complement();

    regex::Regex expected("$|[ab][ab][ab]...");
    expected.compile(-1);

    CHECK(actual.is_equivalent_to(expected));
}

TEST_CASE("Automaton operation: complement of starts-with-a language is expected", "[automaton][operations]") {
    regex::Regex startsWithA("a[ab]...");
    startsWithA.compile(-1);
    auto actual = startsWithA.complement();

    regex::Regex expected("$|b[ab]...");
    expected.compile(-1);

    CHECK(actual.is_equivalent_to(expected));
}

TEST_CASE("Automaton operation: intersect gives expected language", "[automaton][operations]") {
    regex::Regex onlyAandB("[ab]...");
    onlyAandB.compile(-1);
    regex::Regex startsWithA("a[ab]...");
    startsWithA.compile(-1);

    auto actual = regex::Regex::intersect(onlyAandB, startsWithA);

    regex::Regex expected("a[ab]...");
    expected.compile(-1);

    CHECK(actual.is_equivalent_to(expected));
}

TEST_CASE("Automaton operation: intersect with universal language keeps the language", "[automaton][operations]") {
    regex::Regex a("[ab]...");
    a.compile(-1);
    regex::Regex b("a[ab]...");
    b.compile(-1);

    auto ab = regex::Regex::intersect(a, b);
    regex::Regex expected("a[ab]...");
    expected.compile(-1);

    CHECK(ab.is_equivalent_to(expected));
}

TEST_CASE("Automaton operation: intersect language with itself is unchanged", "[automaton][operations]") {
    regex::Regex source("a[ab]...");
    source.compile(-1);
    auto actual = regex::Regex::intersect(source, source);

    regex::Regex expected("a[ab]...");
    expected.compile(-1);

    CHECK(actual.is_equivalent_to(expected));
}

TEST_CASE("Recovered regex is equivalent for starts-with-a language", "[automaton][recover]") {
    regex::Regex source("a[ab]...");
    source.compile(-1);

    std::string recovered_pattern = source.recover();
    regex::Regex recovered(recovered_pattern);
    recovered.compile(-1);

    CHECK(source.is_equivalent_to(recovered));
}

TEST_CASE("Recovered regex is equivalent for intersect automaton", "[automaton][recover]") {
    regex::Regex onlyAandB("[ab]...");
    onlyAandB.compile(-1);
    regex::Regex startsWithA("a[ab]...");
    startsWithA.compile(-1);
    auto inter = regex::Regex::intersect(onlyAandB, startsWithA);

    std::string recovered_pattern = inter.recover();
    regex::Regex recovered(recovered_pattern);
    recovered.compile(-1);

    CHECK(inter.is_equivalent_to(recovered));
}


TEST_CASE("Recovered regex is equivalent for intersect automaton 2", "[automaton][recover]") {
	regex::Regex r1("[ab]...[cd][cd]...");
	r1.compile(-1);
	regex::Regex r2("[ab][ab]...[cd]...");
	r2.compile(-1);
	auto inter = regex::Regex::intersect(r1, r2);


	regex::Regex r3("[ab][ab]...[cd][cd]...");
	r3.compile(-1);

	CHECK(inter.is_equivalent_to(r3));
}
