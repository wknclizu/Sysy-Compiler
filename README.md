# how to run

environment install

```shell
wget https://www.antlr.org/download/antlr4-cpp-runtime-4.9.3-source.zip
unzip antlr4-cpp-runtime-4.9.3-source.zip -d runtime
cd runtime
mkdir build && mkdir run && cd build
cmake .. -DANTLR_JAR_LOCATION=full/path/to/antlr-4.9.3-SNAPSHOT.jar -DWITH_DEMO=True
make
sudo make install
```

then modify the HEADERS in Makefile. For Example: `HEADERS += -I/home/wknclizu/Desktop/Compiler/runtime/runtime/src`

test

```shell
cd src
make
./parser ../test/01_case.sy
./parser ../test/02_case.sy -ast
```

when error `error while loading shared libraries: libantlr4-runtime.so.4.9.3: cannot open shared object file: No such file or directory` occur, please add command like `LD_LIBRARY_PATH=/home/wknclizu/Desktop/Compiler/runtime/dist` in front of the `./parser`

Here's an example Makefile put in the ./src/

```Makefile
target=parser
CC=g++
CXXFLAGS += -std=c++17  -g -w
LIBS += -lantlr4-runtime
HEADERS += -I/home/wknclizu/Desktop/Compiler/runtime/runtime/src

# Grammer files
LEXS = Sysy.g4 SysyLex.g4

# Source files
SRCS = SysyLexer.cpp main.cpp ast.cpp AstVisitor.cpp AstChecker.cpp errorReporter.cpp SysyBaseListener.cpp SysyBaseVisitor.cpp SysyListener.cpp SysyParser.cpp SysyVisitor.cpp

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
    

```
