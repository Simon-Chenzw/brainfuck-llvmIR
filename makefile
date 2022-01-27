comp: a.txt a.opt.txt
	diff a.txt a.opt.txt

a.txt: a.out
	./a.out > a.txt

a.opt.txt: a.opt.out
	./a.opt.out > a.opt.txt

a.out: test.ll
	clang-13 -O0 test.ll -Woverride-module

a.opt.out: test.opt.ll
	clang-13 -O0 test.opt.ll -Woverride-module -o a.opt.out

test.opt.ll: test.ll
	opt-13 test.ll -gvn | llvm-dis-13 > test.opt.ll 

test.ll: bf test.bf
	./bf < test.bf > test.ll

bf: bf.cc
	clang++-13 -g -O3 bf.cc \
	`llvm-config --cxxflags --ldflags --system-libs --libs core` \
	-std=c++17 -o bf
