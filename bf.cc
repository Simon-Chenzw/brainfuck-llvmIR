/**
 * @file bf.cc
 * @author simon chen
 * @brief compile brainfuck to llvm-ir
 * @bug The generated intermediate representation may be wrong after optimization by llvm
 */

#include <functional>
#include <iostream>
#include <stack>
#include "llvm/ADT/APFloat.h"
#include "llvm/ADT/STLExtras.h"
#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constants.h"
#include "llvm/IR/DerivedTypes.h"
#include "llvm/IR/Function.h"
#include "llvm/IR/IRBuilder.h"
#include "llvm/IR/LLVMContext.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Type.h"
#include "llvm/IR/Verifier.h"
using namespace std;
using namespace llvm;

const int cell_size = 30'000;

int main() {
    // llvm context
    LLVMContext context;
    Module module("brainfuck", context);
    IRBuilder<> builder(context);

    // llvm Type & const
    Type* i8 = IntegerType::getInt8Ty(context);
    Type* i32 = IntegerType::getInt32Ty(context);
    Type* i8p = PointerType::get(i8, 0);
    llvm::ConstantInt* zero_i8 = ConstantInt::get(context, APInt(8, 0));
    llvm::ConstantInt* minus_one_i8 = ConstantInt::get(context, APInt(8, -1));
    llvm::ConstantInt* one_i8 = ConstantInt::get(context, APInt(8, 1));

    // external function
    FunctionType* FT_i32_void = FunctionType::get(i32, false);
    FunctionType* FT_i32_i32 = FunctionType::get(i32, i32, false);
    Function* getchar = Function::Create(FT_i32_void, Function::ExternalLinkage, "getchar", module);
    Function* putchar = Function::Create(FT_i32_i32, Function::ExternalLinkage, "putchar", module);

    // main function
    FunctionType* mainType = FunctionType::get(i32, false);
    Function* main = Function::Create(mainType, Function::ExternalLinkage, "main", module);
    BasicBlock* entry = BasicBlock::Create(context, "entry", main);
    builder.SetInsertPoint(entry);

    // alloca cell
    AllocaInst* cells = builder.CreateAlloca(ArrayType::get(i8, cell_size), nullptr, "cells");
    AllocaInst* ptr = builder.CreateAlloca(i8p, nullptr, "ptr");

    // init
    builder.CreateStore(builder.CreateGEP(cells, {zero_i8, zero_i8}, "cells.begin"), ptr);

    stack<llvm::BasicBlock*> st;

    BasicBlock* bb = BasicBlock::Create(context, "", main);
    builder.CreateBr(bb);

    while (char ch = cin.get()) {
        if (cin.eof()) break;
        if (string("><+-,.[]").find(ch) == string::npos) continue;

        builder.SetInsertPoint(bb);
        BasicBlock* bb_nxt = BasicBlock::Create(context, "", main);

        switch (ch) {
            case '>': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* ptr_nxt = builder.CreateGEP(ptr_cur, one_i8, "ptr.nxt");
                builder.CreateStore(ptr_nxt, ptr);
                builder.CreateBr(bb_nxt);
            } break;
            case '<': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* ptr_pre = builder.CreateGEP(ptr_cur, minus_one_i8, "ptr.pre");
                builder.CreateStore(ptr_pre, ptr);
                builder.CreateBr(bb_nxt);
            } break;
            case '+': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(ptr_cur, "cell.cur");
                Value* cell_inc = builder.CreateAdd(cell_cur, one_i8, "cell.inc");
                builder.CreateStore(cell_inc, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case '-': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(ptr_cur, "cell.cur");
                Value* cell_dec = builder.CreateSub(cell_cur, one_i8, "cell.dec");
                builder.CreateStore(cell_dec, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case ',': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* input = builder.CreateCall(getchar, None, "char");
                Value* input_i8 = builder.CreateTrunc(input, i8, "char.trunc");
                builder.CreateStore(input_i8, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case '.': {
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(ptr_cur, "cell.cur");
                Value* cell_cur_i32 = builder.CreateZExt(cell_cur, i32, "cell.cur.ext");
                builder.CreateCall(putchar, cell_cur_i32, "putchar.ret");
                builder.CreateBr(bb_nxt);
            } break;
            case '[': {
                // defer generation of jump statements until matching closing parenthesis
                st.push(bb);
            } break;
            case ']': {
                // Generate jump for matching left parenthesis
                BasicBlock* bb_left = st.top();
                st.pop();
                builder.SetInsertPoint(bb_left);
                builder.CreateBr(bb);
                // current
                builder.SetInsertPoint(bb);
                Value* ptr_cur = builder.CreateLoad(ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(ptr_cur, "cell.cur");
                Value* cond = builder.CreateICmp(CmpInst::Predicate::ICMP_NE, cell_cur, zero_i8, "cond");
                builder.CreateCondBr(cond, bb_left->getNextNode(), bb_nxt);
            } break;
            default:
                // Unpossible
                break;
        }
        bb = bb_nxt;
    }

    builder.SetInsertPoint(bb);
    builder.CreateRet(ConstantInt::get(context, APInt(32, 0)));

    module.print(outs(), nullptr);
}