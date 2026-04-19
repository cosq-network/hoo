grammar Hooc;

// ===== LEXER RULES =====

// Keywords
FUNC: 'func';
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
SCOPE: 'scope';
FINAL: 'final';
SINGLETON: 'singleton';
IMMUTABLE: 'immutable';
FACTORY: 'factory';
OBSERVABLE: 'observable';
EVENT: 'event';
CONSTRUCTOR: 'constructor';
SERVICE: 'service';
STRATEGY: 'strategy';
ACTOR: 'actor';
HOO_INIT: '__hoo_init';
AS: 'as';
BY: 'by';
THIS: 'this';
TRUE: 'true';
FALSE: 'false';
NULL: 'null';

// Primitive Types
BYTE: 'byte';
UINT8: 'uint8';
INT64: 'int64';
FLOAT: 'float';
DOUBLE: 'double';
F64: 'f64';
BOOL: 'bool';
CHAR: 'char';
STRING: 'string';
VOID: 'void';

// Operators
PLUS: '+';
MINUS: '-';
MULTIPLY: '*';
DIVIDE: '/';
MODULO: '%';
ASSIGN: '=';
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
ARROW: '->';
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
STRING_LITERAL: '"' (~["\\\r\n] | '\\' .)* '"';
MULTILINE_STRING: '"""' .*? '"""';

// Character Literal
CHAR_LITERAL: '\'' (~['\\\r\n] | '\\' .) '\'';

// Number Literals
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
compilationUnit: importStatement* (declaration SEMICOLON?)* EOF;

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

classModifier: SINGLETON | IMMUTABLE | FACTORY | OBSERVABLE | SERVICE | STRATEGY | ACTOR | FINAL;

classBody: LBRACE classMember* RBRACE;

classMember
    : variableDeclaration SEMICOLON
    | constructorDeclaration
    | functionDeclaration
    | eventDeclaration SEMICOLON
    ;

constructorDeclaration: CONSTRUCTOR LPAREN parameterList? RPAREN block;

eventDeclaration: EVENT IDENTIFIER;

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
type: optionalType;

optionalType: arrayType QUESTION?;

arrayType: baseType (LBRACKET RBRACKET)*;

baseType
    : primitiveType
    | qualifiedIdentifier
    ;

primitiveType: BYTE | UINT8 | INT64 | FLOAT | DOUBLE | F64 | BOOL | CHAR | STRING | VOID;

// Statements
statement
    : block
    | variableDeclarationStatement
    | expressionStatement
    | returnStatement
    | ifStatement
    | whileStatement
    | forStatement
    | scopeStatement
    | breakStatement
    | continueStatement
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

scopeStatement: SCOPE block;

breakStatement: BREAK SEMICOLON;

continueStatement: CONTINUE SEMICOLON;

// Expressions (with operator precedence)
expression
    : assignmentExpression
    ;

assignmentExpression
    : logicalOrExpression (ASSIGN assignmentExpression)?
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
    : unaryExpression ((MULTIPLY | DIVIDE | MODULO) unaryExpression)*
    ;

unaryExpression
    : (MINUS | NOT)? postfixExpression
    ;

postfixExpression
    : primary postfixSuffix*
    ;

postfixSuffix
    : DOT IDENTIFIER                                         // Member access
    | LBRACKET expression RBRACKET                           // Array/object index
    | LPAREN argumentList? RPAREN                            // Function call
    ;

primary
    : IDENTIFIER                                             // Simple identifier or function call
    | THIS                                                   // Current object instance
    | INTEGER_LITERAL
    | FLOATING_LITERAL
    | STRING_LITERAL
    | CHAR_LITERAL
    | TRUE
    | FALSE
    | NULL
    | LBRACKET expressionList? RBRACKET
    | LPAREN expression RPAREN
    | newExpression
    ;

// Object creation expression
newExpression
    : NEW qualifiedIdentifier LPAREN argumentList? RPAREN
    ;

// String Interpolation (simplified - would need custom lexer handling for full implementation)
interpolatedString: STRING_LITERAL; // Placeholder for interpolated strings with ${...}

argumentList: expression (COMMA expression)*;
expressionList: expression (COMMA expression)*;
