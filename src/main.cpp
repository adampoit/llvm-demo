#include <iostream>
#include <stdio.h>

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

#include "grammar.h"

using namespace llvm;

extern "C"
int print(const char* string)
{
	return puts(string);
}

int main() 
{
	InitializeNativeTarget();

	LLVMContext Context;

	auto owner = make_unique<Module>("test", Context);
	auto module = owner.get();

	auto engine = EngineBuilder(std::move(owner)).create();

	auto printFunction = 
		cast<Function>(module->getOrInsertFunction("print", Type::getInt32Ty(Context),
			Type::getInt8PtrTy(Context),
			(Type*) 0));

	engine->addGlobalMapping(printFunction, &print);

	std::string input;
	while (true)
	{
		std::cout << ">";
		std::getline(std::cin, input);
		if (input == "exit")
			break;

		pegtl::parse<affinity::grammar, affinity::action>(input, input, module, Context, engine, affinity::expression(), affinity::expression());
	}

	delete engine;
	llvm_shutdown();
	return 0;
}
