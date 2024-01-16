#include <iostream>

#define PARSER_IMPL
#include "Parser.hpp"

int main()
{
	Parser parser;

	parser.AddFunction("exp", [&](long double a) { return std::exp(a); });

	while (1)
	{
		std::string input;

		std::cout << ">>> ";
		std::getline(std::cin, input);

		long double result = parser.Get(input, true);

		if (parser.IsOk())
			std::cout << result << std::endl;
		else
		{
			switch (parser.GetState())
			{
			case Parser::State::InvalidSyntax: std::cerr << "Invalid syntax" << std::endl; break;
			case Parser::State::UnknownBinaryOperator: std::cerr << "Unknown binary operator" << std::endl; break;
			case Parser::State::UnknownUnaryOperator: std::cerr << "Unknown unary operator" << std::endl; break;
			case Parser::State::UnknownExpressionType: std::cerr << "Unknown expression type" << std::endl; break;
			}
		}
	}

	return 0;
}
