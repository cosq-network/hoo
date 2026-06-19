grammar Hooc;

// ===== LEXER RULES =====

// Keywords
FUNC: 'func';
PUBLIC: 'public';
PRIVATE: 'private';
RETURN: 'return';
IF: 'if';
ELSE: 'else';
FOR: 'for';
WHILE: 'while';
IN: 'in';
BREAK: 'break';
CONTINUE: 'continue';
CLASS: 'class';
EXTENDS: 'extends';
IMPORT: 'import';
FROM: 'from';
NEW: 'new';
VAR: 'var';
CONST: 'const';

FINAL: 'final';
SINGLETON: 'singleton';
IMMUTABLE: 'immutable';
CONSTRUCTOR: 'constructor';
SERVICE: 'service';
HOO_INIT: '__hoo_init';
AS: 'as';
BY: 'by';
THIS: 'this';
TRUE: 'true';
FALSE: 'false';
NULL: 'null';
TRY: 'try';
CATCH: 'catch';
FINALLY: 'finally';
THROW: 'throw';
RETHROW: 'rethrow';
MAP: 'map';
HASHMAP: 'HashMap';
ANYARRAY: 'AnyArray';
ANY: 'any';
FUNCTION: 'function';
TENSOR: 'tensor';


// Primitive Types
INT8: 'int8';
BYTE: 'byte';
INT64: 'int64';
FLOAT: 'float';
DOUBLE: 'double';
F64: 'f64';
F8: 'f8';
BIT: 'bit';
BOOL: 'bool';
CHAR: 'char';
STRING: 'string';
VOID: 'void';

// Operators
PLUS: '+';
MINUS: '-';
ELEMENT_MULTIPLY: '.*';
ELEMENT_DIVIDE: './';
MULTIPLY: '*';
DIVIDE: '/';
MODULO: '%';
ASSIGN: '=';
COMPOUND_PLUS: '+=';
COMPOUND_MINUS: '-=';
COMPOUND_MULTIPLY: '*=';
COMPOUND_DIVIDE: '/=';
COMPOUND_MODULO: '%=';
COMPOUND_LEFT_SHIFT: '<<=';
COMPOUND_RIGHT_SHIFT: '>>=';
INCREMENT: '++';
DECREMENT: '--';
EQUALS: '==';
NOT_EQUALS: '!=';
LESS: '<';
LESS_EQUALS: '<=';
GREATER: '>';
GREATER_EQUALS: '>=';
AND: '&&';
OR: '||';
NOT: '!';
QUESTION: '?';
RANGE: '..';

// Delimiters
SEMICOLON: ';';
COMMA: ',';
DOT: '.';
COLON: ':';
LPAREN: '(';
RPAREN: ')';
LBRACE: '{';
RBRACE: '}';
LBRACKET: '[';
RBRACKET: ']';

// String Literals
MULTILINE_STRING: '"""' .*? '"""';
STRING_LITERAL: '"' (~["\\\r\n] | '\\' .)* '"';

// Character Literal
CHAR_LITERAL: '\'' (~['\\\r\n] | '\\' .) '\'';

// Number Literals
BIT_LITERAL: [01] 'b';
F8_LITERAL: [0-9]+ '.' [0-9]+ 'f8';
INTEGER_LITERAL: [0-9]+;
FLOATING_LITERAL: [0-9]+ '.' [0-9]+;

// Identifier
IDENTIFIER: [a-zA-Z_][a-zA-Z0-9_]*;

// Whitespace
WS: [ \t\r\n]+ -> skip;

// Comments
SINGLE_LINE_COMMENT: '//' ~[\r\n]* -> skip;
MULTI_LINE_COMMENT: '/*' .*? '*/' -> skip;

// ===== PARSER RULES =====

// Compilation Unit
compilationUnit: importStatement* ((declaration SEMICOLON?))* EOF;

// Import Statements (Python-style)
importStatement
    : IMPORT modulePath (AS IDENTIFIER)? SEMICOLON              # basicImport
    | FROM modulePath IMPORT importItem (COMMA importItem)* SEMICOLON  # fromImport
    ;

modulePath: IDENTIFIER (DOT IDENTIFIER)*;

// Qualified identifier (for module.Type syntax in type and constructor contexts)
qualifiedIdentifier: IDENTIFIER (DOT IDENTIFIER)*;

importItem: IDENTIFIER (AS IDENTIFIER)?;

// Top-level Declarations
declaration
    : functionDeclaration
    | classDeclaration
    | variableDeclaration
    | constantDeclaration
    ;

// Function Declaration
functionDeclaration
    : FUNC (COLON type)? IDENTIFIER LPAREN parameterList? RPAREN block
    ;

parameterList: parameter (COMMA parameter)*;
parameter: IDENTIFIER COLON type;

// Class Declaration
classDeclaration
    : classModifier* CLASS IDENTIFIER (EXTENDS IDENTIFIER)? classBody
    ;

classModifier: SINGLETON | IMMUTABLE | SERVICE | FINAL;

classBody: LBRACE classMember* RBRACE;

classMember
    : functionModifier* (variableDeclaration SEMICOLON | functionDeclaration)
    | constructorDeclaration
    ;

constructorDeclaration: CONSTRUCTOR LPAREN parameterList? RPAREN block;

functionModifier: PUBLIC | PRIVATE;

// Variable Declaration
variableDeclaration
    : VAR IDENTIFIER ASSIGN expression
    | VAR IDENTIFIER COLON type (ASSIGN expression)?
    ;

// Constant Declaration
constantDeclaration
    : CONST IDENTIFIER (COLON type)? ASSIGN expression
    ;

// Types
type: tensorType | hashMapType | mapType | anyType | anyArrayType | optionalType;

optionalType: arrayType QUESTION?;

arrayType: baseType (LBRACKET RBRACKET)*;

baseType
    : primitiveType
    | qualifiedIdentifier
    ;

mapType: MAP LESS mapKeyType COMMA type GREATER;
hashMapType: HASHMAP LESS hashMapKeyType COMMA type GREATER;
anyType: ANY;
anyArrayType: ANYARRAY;
tensorType: TENSOR LESS baseType GREATER LBRACKET INTEGER_LITERAL (COMMA INTEGER_LITERAL)* RBRACKET;

mapKeyType: BYTE | INT8 | INT64 | CHAR | STRING;
hashMapKeyType: BYTE | INT8 | INT64;

primitiveType: INT8 | BYTE | INT64 | FLOAT | DOUBLE | F64 | F8 | BIT | BOOL | CHAR | STRING | VOID;


// Statements
statement
    : block
    | variableDeclarationStatement
    | expressionStatement
    | returnStatement
    | ifStatement
    | whileStatement
    | forStatement
    | breakStatement
    | continueStatement
    | tryCatchStatement
    | throwStatement
    ;

tryCatchStatement
    : TRY block (CATCH LPAREN IDENTIFIER COLON type RPAREN block)* (FINALLY block)?
    ;

throwStatement: THROW expression SEMICOLON
    | RETHROW SEMICOLON
    ;

block: LBRACE statement* RBRACE;

variableDeclarationStatement: variableDeclaration SEMICOLON;

expressionStatement: expression SEMICOLON;

ifStatement: IF expression block (ELSE block)?;

forStatement
    : FOR IDENTIFIER IN expression (RANGE expression (BY expression)?)? block
    ;

whileStatement: WHILE expression block;

returnStatement: RETURN expression? SEMICOLON;

breakStatement: BREAK SEMICOLON;

continueStatement: CONTINUE SEMICOLON;

// Expressions (with operator precedence)
expression
    : assignmentExpression
    ;

assignmentExpression
    : logicalOrExpression (ASSIGN logicalOrExpression | compoundAssignment)?
    ;

compoundAssignment
    : COMPOUND_PLUS logicalOrExpression
    | COMPOUND_MINUS logicalOrExpression
    | COMPOUND_MULTIPLY logicalOrExpression
    | COMPOUND_DIVIDE logicalOrExpression
    | COMPOUND_MODULO logicalOrExpression
    | COMPOUND_LEFT_SHIFT logicalOrExpression
    | COMPOUND_RIGHT_SHIFT logicalOrExpression
    ;

logicalOrExpression
    : logicalAndExpression (OR logicalAndExpression)*
    ;

logicalAndExpression
    : relationalExpression (AND relationalExpression)*
    ;

relationalExpression
    : additiveExpression ((EQUALS | NOT_EQUALS | LESS | LESS_EQUALS | GREATER | GREATER_EQUALS) additiveExpression)*
    ;

additiveExpression
    : multiplicativeExpression ((PLUS | MINUS) multiplicativeExpression)*
    ;

multiplicativeExpression
    : unaryExpression ((ELEMENT_MULTIPLY | ELEMENT_DIVIDE | MULTIPLY | DIVIDE | MODULO) unaryExpression)*
    ;

unaryExpression
    : (MINUS | NOT)? postfixExpression
    ;

postfixExpression
    : primary (postfixSuffix | augmentedAssignment)*
    ;

postfixSuffix
    : DOT IDENTIFIER                                         // Member access
    | DOT NEW                                                // .new() (new is a keyword)
    | LBRACKET expression RBRACKET                           // Array/object index
    | LPAREN argumentList? RPAREN                            // Function call
    ;

augmentedAssignment
    : INCREMENT
    | DECREMENT
    ;

primary
    : IDENTIFIER                                             // Simple identifier or function call
    | THIS                                                   // Current object instance
    | BIT_LITERAL
    | F8_LITERAL
    | INTEGER_LITERAL
    | FLOATING_LITERAL
    | STRING_LITERAL
    | MULTILINE_STRING
    | CHAR_LITERAL
    | TRUE
    | FALSE
    | NULL
    | LBRACKET expressionList? RBRACKET (IDENTIFIER | ANY)?
    | LPAREN expression RPAREN
    | newExpression
    ;

// Object creation expression
newExpression
    : NEW (hashMapType | anyArrayType | qualifiedIdentifier) LPAREN argumentList? RPAREN
    ;

// String Interpolation (simplified - would need custom lexer handling for full implementation)
interpolatedString: STRING_LITERAL; // Placeholder for interpolated strings with ${...}

argumentList: expression (COMMA expression)*;
expressionList: expression (COMMA expression)*;
