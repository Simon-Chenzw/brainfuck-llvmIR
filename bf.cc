/**
 * @file bf.cc
 * @author simon chen
 * @brief compile brainfuck to llvm-ir
 * @bug The generated intermediate representation may be wrong after gvn optimization by llvm
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
#include "llvm/Transforms/InstCombine/InstCombine.h"
#include "llvm/Transforms/Utils/Mem2Reg.h"
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
    ConstantInt* zero_i8 = ConstantInt::get(context, APInt(8, 0));
    ConstantInt* minus_one_i8 = ConstantInt::get(context, APInt(8, -1));
    ConstantInt* one_i8 = ConstantInt::get(context, APInt(8, 1));
    ConstantInt* zero_i32 = ConstantInt::get(context, APInt(32, 0));
    ConstantInt* minus_one_i32 = ConstantInt::get(context, APInt(32, -1));
    ConstantInt* one_i32 = ConstantInt::get(context, APInt(32, 1));

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
    ArrayType* cells_type = ArrayType::get(i8, cell_size);
    AllocaInst* cells = builder.CreateAlloca(cells_type, nullptr, "cells");
    AllocaInst* ptr = builder.CreateAlloca(i8p, nullptr, "ptr");

    // init
    builder.CreateStore(builder.CreateInBoundsGEP(cells_type, cells, {zero_i32, zero_i32}, "cells.begin"), ptr);

    stack<llvm::BasicBlock*> st;

    BasicBlock* bb = BasicBlock::Create(context, "bb", main);
    builder.CreateBr(bb);

    while (char ch = cin.get()) {
        if (cin.eof()) break;
        if (string("><+-,.[]").find(ch) == string::npos) continue;

        builder.SetInsertPoint(bb);
        BasicBlock* bb_nxt = BasicBlock::Create(context, "bb", main);

        switch (ch) {
            case '>': {
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* ptr_nxt = builder.CreateInBoundsGEP(i8, ptr_cur, one_i32, "ptr.nxt");
                builder.CreateStore(ptr_nxt, ptr);
                builder.CreateBr(bb_nxt);
            } break;
            case '<': {
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* ptr_pre = builder.CreateInBoundsGEP(i8, ptr_cur, minus_one_i32, "ptr.pre");
                builder.CreateStore(ptr_pre, ptr);
                builder.CreateBr(bb_nxt);
            } break;
            case '+': {
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(i8, ptr_cur, "cell.cur");
                Value* cell_inc = builder.CreateAdd(cell_cur, one_i8, "cell.inc");
                builder.CreateStore(cell_inc, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case '-': {
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(i8, ptr_cur, "cell.cur");
                Value* cell_dec = builder.CreateSub(cell_cur, one_i8, "cell.dec");
                builder.CreateStore(cell_dec, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case ',': {
                Value* input = builder.CreateCall(getchar, None, "char");
                Value* input_i8 = builder.CreateTrunc(input, i8, "char.trunc");
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                builder.CreateStore(input_i8, ptr_cur);
                builder.CreateBr(bb_nxt);
            } break;
            case '.': {
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(i8, ptr_cur, "cell.cur");
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
                Value* ptr_cur = builder.CreateLoad(i8p, ptr, "ptr.cur");
                Value* cell_cur = builder.CreateLoad(i8, ptr_cur, "cell.cur");
                Value* cond = builder.CreateICmp(CmpInst::Predicate::ICMP_NE, cell_cur, zero_i8, "cond");
                builder.CreateCondBr(cond, bb_left->getNextNode(), bb_nxt);
                // current
                builder.SetInsertPoint(bb);
                builder.CreateBr(bb_left);
            } break;
            default:
                // Impossible
                break;
        }
        bb = bb_nxt;
    }

    builder.SetInsertPoint(bb);
    builder.CreateRet(zero_i32);

    module.print(outs(), nullptr);
}