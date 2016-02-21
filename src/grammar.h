#pragma once

#include <pegtl.hh>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;

std::string func_name(size_t length)
{
	auto randchar = []() -> char
	{
		const char charset[] =
			"0123456789"
			"ABCDEFGHIJKLMNOPQRSTUVWXYZ"
			"abcdefghijklmnopqrstuvwxyz";
		const size_t max_index = (sizeof(charset) - 1);
		return charset[rand() % max_index];
	};

	std::string str(length, 0);
	std::generate_n(str.begin(), length, randchar);
	return str;
}

namespace affinity
{
	struct expression
	{
		void set(std::string content)
		{
			value = content;
		}

		std::string get()
		{
			return value;
		}

	private:
		std::string value;
	};

	struct sep : pegtl::sor<pegtl::space> {};
	struct seps : pegtl::star<sep> {};

	struct str_print : pegtl::string<'p', 'r', 'i', 'n', 't'> {};

	struct str_content : pegtl::plus<pegtl::alpha, seps> {};
	struct str_literal : pegtl::seq<pegtl::one<'\''>, str_content, pegtl::one<'\''>> {};

	template<typename Key>
	struct key : pegtl::seq<Key, pegtl::not_at<pegtl::identifier_other>> {};

	struct key_print : key<str_print> {};

	struct print_statement : pegtl::if_must<key_print, seps, str_literal> {};

	struct grammar : pegtl::must<print_statement, pegtl::eof> {};

	template<typename Rule>
	struct action : pegtl::nothing<Rule> {};

	template<> struct action<str_content>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& expression)
		{
			expression.set(in.string());
		}
	};

	template<> struct action<print_statement>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& expression)
		{
			auto function =
				cast<Function>(module->getOrInsertFunction(func_name(10), Type::getInt32Ty(context),
					(Type *)0));

			auto printFunction =
				cast<Function>(module->getOrInsertFunction("puts", Type::getInt32Ty(context),
					Type::getInt8PtrTy(context),
					(Type*)0));

			auto block = BasicBlock::Create(context, "EntryBlock", function);
			IRBuilder<> builder(block);

			auto string = expression.get();
			auto stringPtr = builder.CreateGlobalStringPtr(string, string);

			auto callRes = builder.CreateCall(printFunction, stringPtr);

			builder.CreateRet(builder.getInt32(0));

			std::vector<GenericValue> noargs;
			auto gv = engine->runFunction(function, noargs);
		}
	};
}
