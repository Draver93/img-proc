/*
 * Command Line Parser
 * ===================
 * 
 * Implements a flexible command-line argument parsing system using the
 * Interpreter pattern for handling various argument formats.
 * 
 * Author: Finoshkin Aleksei
 * License: MIT
 */

#ifndef IMG_DEINT_COMMAND_LINE_PARSER_H
#define IMG_DEINT_COMMAND_LINE_PARSER_H


#include <StdAfx.h>

#include <unordered_map>
#include <regex>

namespace media_proc {

	class Context {
	public:
		void set(const std::string& key, const std::string& value) {
			m_Options[key] = value;
		}

		bool has(const std::string& key) const {
			return m_Options.find(key) != m_Options.end();
		}

		size_t count() const { return m_Options.size(); }

		std::string get(const std::string& key, const std::string& defaultVal = "") const {
			auto it = m_Options.find(key);
			return it != m_Options.end() ? it->second : defaultVal;
		}

		int getInt(const std::string& key, int defaultVal = 0) const {
			auto it = m_Options.find(key);
			return it != m_Options.end() ? std::stoi(it->second) : defaultVal;
		}

		bool getBool(const std::string& key) const {
			auto it = m_Options.find(key);
			if (it == m_Options.end()) return false;
			return it->second == "true" || it->second == "1";
		}

	private:
		std::unordered_map<std::string, std::string> m_Options;
	};

	class Expression {
	public:
		virtual void interpret(Context& ctx, const std::vector<std::string>& args) = 0;
		virtual ~Expression() = default;
	};

	class CommandLineParser {
	public:
		CommandLineParser(int argc, char* argv[]);

		void addExpression(std::shared_ptr<Expression> expr) { m_Expressions.push_back(expr); }
		bool hasOption(const std::string& key) const { return m_Context.has(key); }
		size_t getOptCount() const { return m_Context.count(); }
		std::string getOption(const std::string& key, const std::string& defaultVal = "") const { return m_Context.get(key, defaultVal); }
		int getIntOption(const std::string& key, int defaultVal = 0) const { return m_Context.getInt(key, defaultVal); }
		bool getBoolOption(const std::string& key) const { return m_Context.getBool(key); }

	private:
		Context m_Context;
		std::vector<std::shared_ptr<Expression>> m_Expressions;
	};

}


#endif //!IMG_DEINT_COMMAND_LINE_PARSER_H