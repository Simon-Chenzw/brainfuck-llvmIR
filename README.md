# brainfuck-llvmIR

### interpreter
brainfuck interpreter. Implementing with LLVM IR

- codes not more than 2,000 chars
- code & input split by '!'
- 30,000 cells with u8 (0 ~ 255 wraparound)

### compiler

require: `llvm >= 13`

```bash
clang++ -g -O3 bf.cc \
`llvm-config --cxxflags --ldflags --system-libs --libs core` \
-std=c++17 -o bf
```
