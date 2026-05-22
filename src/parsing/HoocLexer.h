
// Generated from Hooc.g4 by ANTLR 4.13.2

#pragma once


#include "antlr4-runtime.h"


namespace hooc {


class  HoocLexer : public antlr4::Lexer {
public:
  enum {
    FUNC = 1, PUBLIC = 2, PRIVATE = 3, ASYNC = 4, RETURN = 5, IF = 6, ELSE = 7, 
    FOR = 8, WHILE = 9, IN = 10, BREAK = 11, CONTINUE = 12, CLASS = 13, 
    EXTENDS = 14, IMPORT = 15, FROM = 16, NEW = 17, VAR = 18, CONST = 19, 
    SCOPE = 20, FINAL = 21, SINGLETON = 22, IMMUTABLE = 23, FACTORY = 24, 
    OBSERVABLE = 25, CONSTRUCTOR = 26, SERVICE = 27, STRATEGY = 28, ACTOR = 29, 
    HOO_INIT = 30, AS = 31, BY = 32, THIS = 33, TRUE = 34, FALSE = 35, NULL_ = 36, 
    TRY = 37, CATCH = 38, FINALLY = 39, THROW = 40, RETHROW = 41, MAP = 42, 
    FUNCTION = 43, NATIVE = 44, EXTERN = 45, POINTER = 46, ARRAY = 47, AT = 48, 
    LIBRARY = 49, LINK = 50, DYNAMIC = 51, INT8 = 52, BYTE = 53, INT64 = 54, 
    FLOAT = 55, DOUBLE = 56, F64 = 57, BOOL = 58, CHAR = 59, STRING = 60, 
    VOID = 61, PLUS = 62, MINUS = 63, MULTIPLY = 64, DIVIDE = 65, MODULO = 66, 
    ASSIGN = 67, COMPOUND_PLUS = 68, COMPOUND_MINUS = 69, COMPOUND_MULTIPLY = 70, 
    COMPOUND_DIVIDE = 71, COMPOUND_MODULO = 72, COMPOUND_LEFT_SHIFT = 73, 
    COMPOUND_RIGHT_SHIFT = 74, INCREMENT = 75, DECREMENT = 76, EQUALS = 77, 
    NOT_EQUALS = 78, LESS = 79, LESS_EQUALS = 80, GREATER = 81, GREATER_EQUALS = 82, 
    AND = 83, OR = 84, NOT = 85, QUESTION = 86, ARROW = 87, RANGE = 88, 
    SEMICOLON = 89, COMMA = 90, DOT = 91, COLON = 92, LPAREN = 93, RPAREN = 94, 
    LBRACE = 95, RBRACE = 96, LBRACKET = 97, RBRACKET = 98, MULTILINE_STRING = 99, 
    STRING_LITERAL = 100, CHAR_LITERAL = 101, INTEGER_LITERAL = 102, FLOATING_LITERAL = 103, 
    IDENTIFIER = 104, WS = 105, SINGLE_LINE_COMMENT = 106, MULTI_LINE_COMMENT = 107
  };

  explicit HoocLexer(antlr4::CharStream *input);

  ~HoocLexer() override;


  std::string getGrammarFileName() const override;

  const std::vector<std::string>& getRuleNames() const override;

  const std::vector<std::string>& getChannelNames() const override;

  const std::vector<std::string>& getModeNames() const override;

  const antlr4::dfa::Vocabulary& getVocabulary() const override;

  antlr4::atn::SerializedATNView getSerializedATN() const override;

  const antlr4::atn::ATN& getATN() const override;

  // By default the static state used to implement the lexer is lazily initialized during the first
  // call to the constructor. You can call this function if you wish to initialize the static state
  // ahead of time.
  static void initialize();

private:

  // Individual action functions triggered by action() above.

  // Individual semantic predicate functions triggered by sempred() above.

};

}  // namespace hooc
