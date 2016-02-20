#include <pegtl.hh>

#include "llvm/ADT/STLExtras.h"
#include "llvm/ExecutionEngine/GenericValue.h"
#include "llvm/ExecutionEngine/Interpreter.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/Instructions.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/Support/ManagedStatic.h"
#include "llvm/Support/TargetSelect.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

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

	struct sep : pegtl::sor<pegtl::ascii::space> {};
	struct seps : pegtl::star<sep> {};

	struct str_print : pegtl_string_t("print") {};

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
		static void apply(const pegtl::input& in, IRBuilder<>& builder, Function* printFunction, expression& expression)
		{
			expression.set(in.string());
		}
	};

	template<> struct action<print_statement>
	{
		static void apply(const pegtl::input& in, IRBuilder<>& builder, Function* printFunction, expression& expression)
		{
			auto string = expression.get();
			auto stringPtr = builder.CreateGlobalStringPtr(string, string);

			auto callRes = builder.CreateCall(printFunction, stringPtr);
		}
	};
}
