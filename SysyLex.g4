
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
// INT_OCT_LIT : 0[1-7]+;
// INT_DEC_LIT : [1-9][0-9]*;
// INT_HEX_LIT : (0x|0X)[0-9a-zA-Z]+;
// INT_LIT : INT_OCT_LIT|INT_DEC_LIT|INT_HEX_LIT;

// float literal
FLOAT_

// fragment for float literal


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


