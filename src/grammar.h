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

	struct identifier : pegtl::identifier {};

	struct str_literal : pegtl::seq<pegtl::one<'"'>, pegtl::until<pegtl::one<'"'>>> {};

	template<typename Key>
	struct key : pegtl::seq<Key, pegtl::not_at<pegtl::identifier_other>> {};

	struct function_call : pegtl::if_must<identifier, pegtl::one<'('>, str_literal, pegtl::one<')'>> {};

	struct grammar : pegtl::must<function_call, pegtl::eof> {};

	template<typename Rule>
	struct action : pegtl::nothing<Rule> {};

	template<> struct action<str_literal>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& functionName, expression& argument)
		{
			argument.set(in.string().substr(1, in.string().size() - 2));
		}
	};

	template<> struct action<identifier>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& functionName, expression& argument)
		{
			functionName.set(in.string());
		}
	};

	template<> struct action<function_call>
	{
		static void apply(const pegtl::input& in, Module* module, LLVMContext& context, ExecutionEngine* engine, expression& functionName, expression& argument)
		{
			auto functionWrapper =
				cast<Function>(module->getOrInsertFunction(unique_identifier(10), Type::getInt32Ty(context),
					(Type *)0));

			auto function = module->getFunction(functionName.get());
			if (function == nullptr)
			{
				std::cout << "Unknown function: " << functionName.get() << std::endl;
			}
			else
			{
				auto block = BasicBlock::Create(context, "EntryBlock", functionWrapper);
				IRBuilder<> builder(block);

				auto string = argument.get();
				auto stringPtr = builder.CreateGlobalStringPtr(string, unique_identifier(10));

				auto callRes = builder.CreateCall(function, stringPtr);

				builder.CreateRet(builder.getInt32(0));

				std::vector<GenericValue> noargs;
				auto gv = engine->runFunction(functionWrapper, noargs);
			}
		}
	};
}
