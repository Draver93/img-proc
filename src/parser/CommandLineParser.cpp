#include "CommandLineParser.h"

#include <unordered_map>
#include <regex>

namespace img_deinterlace {
	class DoubleDashExpression : public Expression {
	public:
		void interpret(Context& ctx, const std::vector<std::string>& args) override {
			for (size_t i = 0; i < args.size(); ++i) {
				if (args[i].rfind("--", 0) == 0) {
					if (i + 1 < args.size() && args[i + 1].rfind("-", 0) != 0) {
						ctx.set(args[i], args[i + 1]);
						++i; // Skip the value in next iteration
					} else {
						ctx.set(args[i], "true"); // flag without value
					}
				}
			}
		}
	};

	class SingleDashExpression : public Expression {
	public:
		void interpret(Context& ctx, const std::vector<std::string>& args) override {
			std::regex pattern(R"(-(\w+)=(\w+))");
			for (const auto& arg : args) {
				std::smatch match;
				if (std::regex_match(arg, match, pattern)) {
					ctx.set("-" + match[1].str(), match[2].str());
				}
			}
		}
	};

	CommandLineParser::CommandLineParser(int argc, char* argv[]) {
		std::vector<std::string> args(argv + 1, argv + argc);

		addExpression(std::make_shared<DoubleDashExpression>());
		addExpression(std::make_shared<SingleDashExpression>());

		for (auto& expr : m_Expressions) expr->interpret(m_Context, args);
	}
}