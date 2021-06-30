#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"
#include "parser.hpp"

TEST_CASE("testing parse::binary")  {
	std::string str;
	Source source;

	str = "4 + 6 * 5";
	source = Source("", str.begin(), str.end());
}

TEST_CASE("testing parse::number")  {
	std::string str;
	Source source;

	str = "4.3";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Number>(parse::number(source).get_value())->value == 4.3);
	CHECK(at_end(parse::number(source).get_source()) == true);

	str = "6";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Number>(parse::number(source).get_value())->value == 6);
	CHECK(at_end(parse::number(source).get_source()) == true);

	str = ".8";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Number>(parse::number(source).get_value())->value == 0.8);
	CHECK(at_end(parse::number(source).get_source()) == true);

	str = "0.33333333";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Number>(parse::number(source).get_value())->value == 0.33333333);
	CHECK(at_end(parse::number(source).get_source()) == true);

	str = "hello";
	source = Source("", str.begin(), str.end());
	CHECK(parse::number(source).is_error() == true);
	CHECK(at_end(parse::number(source).get_source()) == false);
}

TEST_CASE("testing parse::identifier")  {
	std::string str;
	Source source;

	str = "my_identifier_number_35";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Identifier>(parse::identifier(source).get_value())->value == "my_identifier_number_35");
	CHECK(at_end(parse::identifier(source).get_source()) == true);

	// Identifiers can't start with a number
	str = "35var";
	source = Source("", str.begin(), str.end());
	CHECK(parse::identifier(source).is_error() == true);

	// Identifiers can start with _
	str = "_35var";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Identifier>(parse::identifier(source).get_value())->value == "_35var");

	// false
	str = " false ";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Boolean>(parse::identifier(source).get_value())->value == false);

	// true
	str = " true ";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Boolean>(parse::identifier(source).get_value())->value == true);
}

TEST_CASE("testing parse::op")  {
	std::string str;
	Source source;

	str = "    >=3 ";
	source = Source("", str.begin(), str.end());
	CHECK(std::dynamic_pointer_cast<Ast::Identifier>(parse::op(source).get_value())->value == ">=");
	CHECK(current(parse::op(source).get_source()) == '3');
}

TEST_CASE("testing parse::token") {
	std::string str;
	Source source;

	str = "      token  ";
	source = Source("", str.begin(), str.end());
	CHECK(parse::token(source, "token").get_value() == "token");
	CHECK(current(parse::token(source, "token").get_source()) == ' ');
	CHECK(at_end(parse::token(source, "token").get_source() + 2) == true);
}

TEST_CASE("testing parse::whitespace") {
	std::string str;
	Source source;

	str = "      -- This is whitespace";
	source = Source("", str.begin(), str.end());
	CHECK(parse::whitespace(source).get_value() == "      ");
	CHECK(current(parse::whitespace(source).get_source()) == '-');
}

TEST_CASE("testing parse::comment") {
	std::string str;
	Source source;

	str = "-- Hello, this is a comment";
	source = Source("", str.begin(), str.end());
	CHECK(parse::comment(source).get_value() == "-- Hello, this is a comment");
	CHECK(at_end(parse::comment(source).get_source()) == true);

	str = "4 + 6 -- Not a comment";
	source = Source("", str.begin(), str.end());
	CHECK(parse::comment(source).is_error() == true);
	CHECK(current(parse::comment(source).get_source()) == '4');
}

TEST_CASE("testing parse::regex") {
	std::string str;
	Source source;
	std::string regex;

	str = " identifier 2343 555 ";
	regex = "[a-zA-Z_][a-zA-Z_0-9]+";
	source = Source("", str.begin(), str.end());
	CHECK(parse::regex(source, regex).is_error() == true);

	str = " identifier 2343 555 ";
	regex = "[a-zA-Z_][a-zA-Z_0-9]+";
	source = Source("", str.begin(), str.end());
	CHECK(parse::regex(source + 1, regex).value == "identifier");
}
