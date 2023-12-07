
lexer grammar SysyLex;

// keyword
INT : 'int';
FLOAT : 'float';
VOID : 'void';
CONST : 'const';
RETURN : 'return';
IF : 'if';
ELSE : 'else';
WHILE : 'while';
BREAK : 'break';
CONTINUE : 'continue'; 

// delimeter
LP : '(' ;
RP : ')' ;
LB : '[' ;
RB : ']' ;
LC : '{' ;
RC : '}' ;
COMMA : ',' ;
SEMICOLON : ';';
QUESTION : '?';
COLON : ':';

// operator
MINUS : '-';
NOT : '!';
ASSIGN : '=';
ADD : '+';
MUL : '*';
DIV : '/';
MOD : '%';
AND : '&&';
OR : '||';
EQ : '==';
NEQ : '!=';
LT : '<';
LE : '<=';
GT : '>';
GE : '>=';

// integer literal
INT_LIT : '0'[1-7]+|[1-9][0-9]*|('0x'|'0X')[0-9a-zA-Z]+;

// float literal
// connect 2 no-delim? TODO
FLOAT_LIT : [0-9]*(FLOAT_EXP_LIT|FLOAT_EXP_LIT FLOAT_POINT_LIT|FLOAT_POINT_LIT)[fF]?;

// fragment for float literal
FLOAT_EXP_LIT : [Ee][+-]?[0-9]+;
FLOAT_POINT_LIT : '.'[0-9]+;

// identifier
ID : [_a-zA-Z][_a-zA-Z0-9]*;

// ID : 'main"
// string
STRING : '"'(ESC|.)*?'"';

// fragment for string

ESC : '\\"'|'\\\\';

// whitespace
WS : 
    [ \t\r\n] -> skip
    ;

// comments
LINE_COMMENT : '//' .*? '\r'? '\n' -> skip;
BLOCK_COMMENT : '/*'.*?'*/'-> skip ;

LEX_ERR : [0-9][0-9a-zA-Z]+;
