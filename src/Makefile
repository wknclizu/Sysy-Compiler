target=parser
CC=g++
CXXFLAGS += -std=c++17  -g -w
LIBS += -lantlr4-runtime
HEADERS += -I/usr/include/antlr4-runtime/

# Grammer files
LEXS = Sysy.g4 SysyLex.g4

# Source files
SRCS = SysyLexer.cpp main.cpp ast.cpp AstVisitor.cpp SysyBaseListener.cpp SysyBaseVisitor.cpp SysyListener.cpp SysyParser.cpp SysyVisitor.cpp

# Object files
OBJS = $(SRCS:.cpp=.o)

# Default target
all: $(target)

$(target): $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS) $(HEADERS)

SysyLexer.cpp: $(LEXS)
	java -jar ../antlr-4.9.3-complete.jar Sysy.g4 -Dlanguage=Cpp -listener -visitor -o .

%.o: %.cpp
	$(CC) -c $< -o $@ $(CXXFLAGS) $(HEADERS)

.PHONY: clean
clean:
	rm -f $(OBJS) Sysy*.cpp Sysy*.h parser
    
