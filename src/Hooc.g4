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
CLASS: 'class';
INTERFACE: 'interface';
IMPLEMENTS: 'implements';
EXTENDS: 'extends';
IMPORT: 'import';
FROM: 'from';
NEW: 'new';
VAR: 'var';
SCOPE: 'scope';
FINAL: 'final';
SINGLETON: 'singleton';
IMMUTABLE: 'immutable';
FACTORY: 'factory';
OBSERVABLE: 'observable';
EVENT: 'event';
SERVICE: 'service';
STRATEGY: 'strategy';
ACTOR: 'actor';
AS: 'as';
TRUE: 'true';
FALSE: 'false';

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
LAMBDA_ARROW: '=>';
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
PIPE: '|';

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
compilationUnit: declaration* EOF;

// Import Statements
importStatement
    : IMPORT LBRACE importItem (COMMA importItem)* RBRACE FROM STRING_LITERAL SEMICOLON     # namedImports
    | IMPORT MULTIPLY AS IDENTIFIER FROM STRING_LITERAL SEMICOLON                          # namespaceImport
    | IMPORT STRING_LITERAL SEMICOLON                                                      # sideEffectImport
    ;

importItem: IDENTIFIER (AS IDENTIFIER)?;

// Top-level Declarations
declaration
    : functionDeclaration
    | classDeclaration
    | interfaceDeclaration
    ;

// Function Declaration
functionDeclaration
    : FUNC IDENTIFIER LPAREN parameterList? RPAREN ARROW type block
    ;

parameterList: parameter (COMMA parameter)*;
parameter: type IDENTIFIER;

// Class Declaration
classDeclaration
    : classModifier* CLASS IDENTIFIER primaryConstructor? (EXTENDS IDENTIFIER)? (IMPLEMENTS interfaceList)? classBody
    ;

classModifier: SINGLETON | IMMUTABLE | FACTORY | OBSERVABLE | SERVICE | STRATEGY | ACTOR | FINAL;

primaryConstructor: LPAREN parameterList? RPAREN;

interfaceList: IDENTIFIER (COMMA IDENTIFIER)*;

classBody: LBRACE classMember* RBRACE;

classMember
    : functionDeclaration
    | eventDeclaration SEMICOLON
    ;

eventDeclaration: EVENT IDENTIFIER;

// Interface Declaration
interfaceDeclaration
    : INTERFACE IDENTIFIER LBRACE interfaceMember* RBRACE
    ;

interfaceMember: functionSignature SEMICOLON;

functionSignature: FUNC IDENTIFIER LPAREN parameterList? RPAREN ARROW type;

// Variable Declaration
variableDeclaration
    : VAR IDENTIFIER ASSIGN expression
    | VAR IDENTIFIER COLON type (ASSIGN expression)?
    ;

// Types
type: unionType;

unionType: optionalType (PIPE optionalType)*;

optionalType: arrayType QUESTION?;

arrayType: baseType (LBRACKET RBRACKET)*;

baseType
    : primitiveType
    | IDENTIFIER
    ;

primitiveType: BYTE | UINT8 | INT64 | FLOAT | DOUBLE | F64 | BOOL | CHAR | STRING | VOID;

// Statements
statement
    : block
    | variableDeclaration SEMICOLON
    | expressionStatement SEMICOLON
    | returnStatement SEMICOLON
    | ifStatement
    | whileStatement
    | forStatement
    | scopeStatement
    ;

block: LBRACE statement* RBRACE;

expressionStatement: expression;

ifStatement: IF expression block (ELSE block)?;

forStatement
    : FOR IDENTIFIER IN expression block                    # forInStatement
    | FOR IDENTIFIER IN expression RANGE expression block   # forRangeStatement
    ;

whileStatement: WHILE expression block;

returnStatement: RETURN expression?;

scopeStatement: SCOPE block;

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
    : primary (
        DOT IDENTIFIER
      | LBRACKET expression RBRACKET
      | LPAREN argumentList? RPAREN
    )*
    ;

primary
    : IDENTIFIER
    | INTEGER_LITERAL
    | FLOATING_LITERAL
    | STRING_LITERAL
    | CHAR_LITERAL
    | TRUE
    | FALSE
    | LBRACKET expressionList? RBRACKET
    | LPAREN expression RPAREN
    ;

// String Interpolation (simplified - would need custom lexer handling for full implementation)
interpolatedString: STRING_LITERAL; // Placeholder for interpolated strings with ${...}

argumentList: expression (COMMA expression)*;
expressionList: expression (COMMA expression)*;