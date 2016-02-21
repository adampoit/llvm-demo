#pragma once

#include <pegtl.hh>

#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"

using namespace llvm;

std::string unique_identifier(size_t length)
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

	struct str_literal : pegtl::seq<pegtl::one<'"'>, pegtl::until<pegtl::one<'"'>>> {};

	template<typename Key>
	struct key : pegtl::seq<Key, pegtl::not_at<pegtl::identifier_other>> {};

	struct key_print : key<str_print> {};

	struct print_statement : pegtl::if_must<key_print, seps, str_literal> {};

	struct grammar : pegtl::must<print_statement, pegtl::eof> {};

	template<typename Rule>
	struct action : pegtl::nothing<Rule> {};

	template<> struct action<str_literal>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& expression)
		{
			expression.set(in.string().substr(1, in.string().size() - 2));
		}
	};

	template<> struct action<print_statement>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& expression)
		{
			auto function =
				cast<Function>(module->getOrInsertFunction(unique_identifier(10), Type::getInt32Ty(context),
					(Type *)0));

			auto printFunction =
				cast<Function>(module->getOrInsertFunction("puts", Type::getInt32Ty(context),
					Type::getInt8PtrTy(context),
					(Type*)0));

			auto block = BasicBlock::Create(context, "EntryBlock", function);
			IRBuilder<> builder(block);

			auto string = expression.get();
			auto stringPtr = builder.CreateGlobalStringPtr(string, unique_identifier(10));

			auto callRes = builder.CreateCall(printFunction, stringPtr);

			builder.CreateRet(builder.getInt32(0));

			std::vector<GenericValue> noargs;
			auto gv = engine->runFunction(function, noargs);
		}
	};
}
