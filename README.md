# Sysy语法分析实验报告

## 程序功能

本实验编写了一个程序，对Sysy语言进行词法分析，主要功能是将Sysy源代码分解为一系列的词法单元（Tokens），并对每个词法单元进行分类和标记。它可以识别Sysy语言中的关键字、标识符、运算符、常量、分隔符等，并将它们作为输出。程序使用了GNU Flex进行分析。具体实现：

1. 使用正则表达式定义Sysy语言的词法规则：根据Sysy语言的语法规范，确定需要识别的关键字、标识符、运算符、常量等词法单元的模式。定义的token如下：

   enum Token {
       INT=258, FLOAT, VOID, CONST, RETURN, IF, ELSE, WHILE, BREAK, CONTINUE,
       LP, RP, LB, RB, LC, RC, COMMA, SEMICOLON, QUESTION, COLON,
       MINUS, NOT, TILDE, ASSIGN, ADD, MUL, DIV, MOD, AND, OR, EQ, NE, LT, LE, GT, GE,
       ID, INT_LIT, FLOAT_LIT,
       LEX_ERR
   };

2. 处理词法单元：在词法分析器中，对于识别到的INT_LIT类型，判断其为八进制、十进制还是十六进制，并转换为十进制输出。同时，我在程序中通过添加正则表达式实现了对FLOAT_LIT（浮点数）类型的识别。

## 编译与测试

文件结构

``` text
.
│   README.md 
├───flex
│   │   lex.yy.cc
│   │   scanner
│   │	sysy.l
│   
└───test_cases
    │   01_test_basic.sy
    │   02_test_var.sy
    │	...
```

编译指令

```bash
cd src
make
```

测试指令

```bash
cd src
./scanner ..test_cases/01_test_basic.sy
./scanner ..test_cases/02_test_var.sy
```

如果要测试其他用例同理。

对于如下的代码

```cpp
int e[2][3] = {1, 2, 3, 4, 5, 6};
int b;
int c;
float d;
int main() {
    b = ~5;
    c = 0x5;
    c = 087;
    c = 0xh5;
    d = 1.2
    return b + c + c * b;
}
```

输出如下

