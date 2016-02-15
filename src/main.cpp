#include <iostream>

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

int main() 
{
	InitializeNativeTarget();

	LLVMContext Context;

	auto owner = make_unique<Module>("test", Context);
	auto module = owner.get();

	auto addFunction =
		cast<Function>(module->getOrInsertFunction("add", Type::getInt32Ty(Context),
			Type::getInt32Ty(Context),
			Type::getInt32Ty(Context),
			(Type *)0));

	auto block = BasicBlock::Create(Context, "EntryBlock", addFunction);
	IRBuilder<> builder(block);

	assert(addFunction->arg_begin() != addFunction->arg_end());
	auto firstArg = addFunction->arg_begin();
	firstArg->setName("x");
	auto secondArg = firstArg->getNextNode();
	secondArg->setName("y");

	auto add = builder.CreateAdd(firstArg, secondArg);
	builder.CreateRet(add);

	auto mainFunction =
		cast<Function>(module->getOrInsertFunction("main", Type::getInt32Ty(Context),
			(Type *)0));

	block = BasicBlock::Create(Context, "EntryBlock", mainFunction);
	builder.SetInsertPoint(block);

	auto firstInt = builder.getInt32(3);
	auto secondInt = builder.getInt32(4);

	std::vector<Value*> addArgs { firstInt, secondInt };

	auto addCallRes = builder.CreateCall(addFunction, addArgs);
	addCallRes->setTailCall(true);
	builder.CreateRet(addCallRes);

	auto engine = EngineBuilder(std::move(owner)).create();

	std::vector<GenericValue> noargs;
	auto gv = engine->runFunction(mainFunction, noargs);

	// Import result of execution:
	std::cout << "Result: " << gv.IntVal.getSExtValue() << "\n";
	delete engine;
	llvm_shutdown();
	return 0;
}