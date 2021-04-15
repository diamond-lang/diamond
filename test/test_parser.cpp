#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"

TEST_CASE("testing parse::number")  {
	std::string str;
	Source source;

	str = "4.3";
	source = Source("", str.begin(), str.end());
	CHECK(((Ast::Number*) parse::number(source).value)->value == 4.3);
	CHECK(at_end(parse::number(source).source) == true);

	str = "6";
	source = Source("", str.begin(), str.end());
	CHECK(((Ast::Number*) parse::number(source).value)->value == 6);
	CHECK(at_end(parse::number(source).source) == true);

	str = ".8";
	source = Source("", str.begin(), str.end());
	CHECK(((Ast::Number*) parse::number(source).value)->value == 0.8);
	CHECK(at_end(parse::number(source).source) == true);

	str = "0.33333333";
	source = Source("", str.begin(), str.end());
	CHECK(((Ast::Number*) parse::number(source).value)->value == 0.33333333);
	CHECK(at_end(parse::number(source).source) == true);

	str = "hello";
	source = Source("", str.begin(), str.end());
	CHECK(parse::number(source).error() == true);
	CHECK(at_end(parse::number(source).source) == false);
}

TEST_CASE("testing parse::regex") {
	std::string str;
	Source source;
	std::string regex;

	str = " identifier 2343 555 ";
	regex = "[a-zA-Z_][a-zA-Z_0-9]+";
	source = Source("", str.begin(), str.end());
	CHECK(parse::regex(source, regex).error() == true);

	str = " identifier 2343 555 ";
	regex = "[a-zA-Z_][a-zA-Z_0-9]+";
	source = Source("", str.begin(), str.end());
	CHECK(parse::regex(source + 1, regex).value == "identifier");
}
