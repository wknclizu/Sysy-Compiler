target=parser
objs=ast.o parser.o printer.o tokens.o main.o 
CC=g++
CXXFLAGS += -std=c++17
YACC=yacc

$(target):${objs}
	$(CC) -o $(target)  $(objs) 
%.o:%.cpp
	$(CC) -c $< $(CXXFLAGS) 
parser.c:parser.y
	$(YACC) parser.y -Wno-yacc
.PHONY: clean
clean:
	rm $(objs) parser.cpp parser.hpp tokens.cpp parser -f
