#ifndef PARSER_HPP
#undef PARSER_HPP

#include <string>
#include <vector>
#include <cmath>
#include <numbers>
#include <unordered_map>
#include <functional>

struct Expression
{
	Expression(std::string_view token = "");
	Expression(std::string_view token, const Expression& rhs);
	Expression(std::string_view token, const Expression& lhs, const Expression& rhs);

	std::string token;
	std::vector<Expression> arguments;
};

class Parser
{
public:
	enum class State
	{
		Ok,
		InvalidSyntax,
		UnknownBinaryOperator,
		UnknownUnaryOperator,
		UnknownExpressionType
	};

public:
	Parser();
	~Parser();

	long double Get(std::string input, bool radians);

	State GetState() const;
	bool IsOk() const;

	void AddOperator(std::string_view text, const std::function<long double(long double, long double)>& handler);
	void AddFunction(std::string_view text, const std::function<long double(long double)>& handler);
	void AddConstant(std::string_view text, long double value);

private:
	bool ParseToken(std::string& token);

	Expression ParseSimpleExpression();
	Expression ParseBinaryExpression(int minPriority);

	long double Evaluate(const Expression& expr);

	int GetPriority(std::string_view binaryOp);

private:
	State m_State = State::Ok;

	char* m_Input;
	bool m_Radians;

	std::list<std::string_view> TOKENS = {
		"+", "-", "^", "*", "/", "%", "(", ")", "!", "e", "lg", "ln", "pi", "abs", "sin", "cos", "tan",
		"sqrt", "asin", "acos", "atan", "log2"
	};

	std::unordered_map<std::string_view, std::string> CONSTANTS =
	{
		{ "pi", std::to_string(std::numbers::pi) },
		{ "e", std::to_string(std::numbers::e) }
	};

	std::unordered_map<std::string, std::function<long double(long double)>> FUNCTIONS =
	{
		{ "+", [](long double a) { return a; } },
		{ "-", [](long double a) { return -a; } },
		{ "abs", [&](long double a) { return abs(a); } },
		{ "log2", [&](long double a) { return log2(a); } },
		{ "lg", [&](long double a) { return log10(a); } },
		{ "ln", [&](long double a) { return log(a); } },
		{ "sin", [&](long double a) { return m_Radians ? sin(a) : sin(a * std::numbers::pi / 180.0); } },
		{ "cos", [&](long double a) { return m_Radians ? cos(a) : cos(a * std::numbers::pi / 180.0); } },
		{ "tan", [&](long double a) { return m_Radians ? tan(a) : tan(a * std::numbers::pi / 180.0); } },
		{ "asin", [&](long double a) { return m_Radians ? asin(a) : asin(a) / std::numbers::pi * 180.0; } },
		{ "acos", [&](long double a) { return m_Radians ? acos(a) : acos(a) / std::numbers::pi * 180.0; } },
		{ "atan", [&](long double a) { return m_Radians ? atan(a) : atan(a) / std::numbers::pi * 180.0; } },
		{ "sqrt", [&](long double a) { return sqrt(a); } },
		{ "!", [&](long double a) { return tgamma(a + 1.0L); } }
	};

	std::unordered_map<std::string, std::function<long double(long double, long double)>> OPERATORS =
	{
		{ "+", [](long double a, long double b) { return a + b; } },
		{ "-", [](long double a, long double b) { return a - b; } },
		{ "*", [](long double a, long double b) { return a * b; } },
		{ "/", [](long double a, long double b) { return a / b; } },
		{ "^", [](long double a, long double b) { return pow(a, b); } },
		{ "%", [](long double a, long double b) { return (long int)a % (long int)b; } }
	};

};

#ifdef PARSER_IMPL
#undef PARSER_IMPL


Expression::Expression(std::string_view token) : token(token)
{
}

Expression::Expression(std::string_view token, const Expression& rhs) : token(token), arguments{rhs}
{
}

Expression::Expression(std::string_view token, const Expression& lhs, const Expression& rhs) : token(token), arguments{lhs, rhs}
{
}



Parser::Parser()
{

}


Parser::~Parser()
{
}


long double Parser::Get(std::string input, bool radians)
{
	for (auto& c : input)
	{
		if (isalpha(c))
			c = tolower(c);
	}

	m_Input = (char*)input.c_str();
	m_Radians = radians;

	m_State = State::Ok;

	long double result = Evaluate(ParseBinaryExpression(0));
	m_Input = nullptr;

	return result;
}


bool Parser::ParseToken(std::string& token)
{
	while (std::isspace(*m_Input))
		m_Input++;

	if (*m_Input == '\0')
		return false;

	if (std::isdigit(*m_Input))
	{
		while (std::isdigit(*m_Input) || *m_Input == '.')
			token.push_back(*m_Input++);

		if (token.back() == '.')
		{
			m_State = State::InvalidSyntax;
			return true;
		}

		return true;
	}

	for (auto t = TOKENS.crbegin(); t != TOKENS.crend(); t++)
		if (std::strncmp(m_Input, t->data(), t->length()) == 0)
		{
			m_Input += t->length();

			if (CONSTANTS.contains(*t))
			{
				token = CONSTANTS[*t];
				return true;
			}

			token = t->data();
			return false;
		}

	m_State = State::InvalidSyntax;
	return false;
}


Expression Parser::ParseSimpleExpression()
{
	std::string token;
	bool isNumber = ParseToken(token);

	if (!IsOk())
		return {};

	if (isNumber)
		return Expression(token);

	if (token == "(")
	{
		Expression result = ParseBinaryExpression(0);

		token.clear();
		isNumber = ParseToken(token);

		if (token != ")")
		{
			m_State = State::InvalidSyntax;
			return {};
		}

		return result;
	}

	return Expression(token, ParseSimpleExpression());
}


Expression Parser::ParseBinaryExpression(int minPriority)
{
	Expression lhs = ParseSimpleExpression();

	while (1)
	{
		std::string op;
		bool isNumber = ParseToken(op);

		int priority = GetPriority(op);

		if (priority <= minPriority)
		{
			m_Input -= op.size();
			return lhs;
		}

		Expression rhs = ParseBinaryExpression(priority);
		lhs = Expression(op, lhs, rhs);
	}
}


long double Parser::Evaluate(const Expression& expr)
{
	switch (expr.arguments.size())
	{
	case 2:
	{
		if (OPERATORS.contains(expr.token))
		{
			return OPERATORS[expr.token](
				Evaluate(expr.arguments[0]),
				Evaluate(expr.arguments[1]));
		}

		m_State = State::UnknownBinaryOperator;
	}
	break;

	case 1:
	{
		if (FUNCTIONS.contains(expr.token))
		{
			return FUNCTIONS[expr.token](
				Evaluate(expr.arguments[0]));
		}

		m_State = State::UnknownUnaryOperator;
	}
	break;

	case 0:
		if (expr.token.empty() || std::find_if(expr.token.begin(),
			expr.token.end(), [](char c) { return !std::isdigit(c) && c != '.'; }) != expr.token.end())
		{
			m_State = State::UnknownExpressionType;
			return 0.0;
		}

		return std::stold(expr.token);
	}
}


Parser::State Parser::GetState() const
{
	return m_State;
}


int Parser::GetPriority(std::string_view binaryOp)
{
	if (binaryOp == "+") return 1;
	if (binaryOp == "-") return 1;
	if (binaryOp == "*") return 2;
	if (binaryOp == "/") return 2;
	if (binaryOp == "%") return 3;
	if (binaryOp == "^") return 3;

	return 0;
}


bool Parser::IsOk() const
{
	return m_State == State::Ok;
}

void Parser::AddOperator(std::string_view text, const std::function<long double(long double, long double)>& handler)
{
	OPERATORS.insert({ text.data(), handler});

	for (auto it = TOKENS.begin(); it != TOKENS.end(); it++)
	{
		if (it->length() > text.length())
			TOKENS.insert(it, text);
	}
}

void Parser::AddFunction(std::string_view text, const std::function<long double(long double)>& handler)
{
	FUNCTIONS.insert({ text.data(), handler});
	TOKENS.push_back(text);

	for (auto it = TOKENS.begin(); it != TOKENS.end(); it++)
	{
		if (it->length() > text.length())
			TOKENS.insert(it, text);
	}
}

void Parser::AddConstant(std::string_view text, long double value)
{
	CONSTANTS.insert({ text, std::to_string(value) });
}

#endif

#endif
