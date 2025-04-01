all: obj
	g++ ptrace.o helper.o

obj: helper.s ptrace.cpp
	g++ -c -g ptrace.cpp
	g++ -c helper.s
