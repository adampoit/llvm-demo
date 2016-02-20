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

int main() 
{
	InitializeNativeTarget();

	LLVMContext Context;

	auto owner = make_unique<Module>("test", Context);
	auto module = owner.get();

	auto printFunction = 
		cast<Function>(module->getOrInsertFunction("puts", Type::getInt32Ty(Context),
			Type::getInt8PtrTy(Context),
			(Type*) 0));

	auto mainFunction =
		cast<Function>(module->getOrInsertFunction("main", Type::getInt32Ty(Context),
			(Type *)0));

	auto block = BasicBlock::Create(Context, "EntryBlock", mainFunction);
	IRBuilder<> builder(block);

	char** input = new char*[1];
	input[0] = "print 'This is a test'";
	pegtl::parse<affinity::grammar, affinity::action>(0, input, builder, printFunction, affinity::expression());

	builder.CreateRet(builder.getInt32(0));

	auto engine = EngineBuilder(std::move(owner)).create();

	std::vector<GenericValue> noargs;
	auto gv = engine->runFunction(mainFunction, noargs);

	delete engine;
	delete input;
	llvm_shutdown();
	return 0;
}
