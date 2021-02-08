#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"

TEST_CASE("testing parse::regex") {
	std::string str = " identifier 2343 555 ";
	SourceFile source = SourceFile("file_name", str.begin(), str.end());

	auto result = parse::regex(source, "[a-zA-Z_][a-zA-Z_0-9]+");
	CHECK(result.error() == true);

	result = parse::regex(source + 1, "[a-zA-Z_][a-zA-Z_0-9]+");
	CHECK(*(result.value.get()) == "identifier");
}
