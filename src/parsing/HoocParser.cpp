
// Generated from Hooc.g4 by ANTLR 4.13.2


#include "HoocListener.h"
#include "HoocVisitor.h"

#include "HoocParser.h"


using namespace antlrcpp;
using namespace hooc;

using namespace antlr4;

namespace {

struct HoocParserStaticData final {
  HoocParserStaticData(std::vector<std::string> ruleNames,
                        std::vector<std::string> literalNames,
                        std::vector<std::string> symbolicNames)
      : ruleNames(std::move(ruleNames)), literalNames(std::move(literalNames)),
        symbolicNames(std::move(symbolicNames)),
        vocabulary(this->literalNames, this->symbolicNames) {}

  HoocParserStaticData(const HoocParserStaticData&) = delete;
  HoocParserStaticData(HoocParserStaticData&&) = delete;
  HoocParserStaticData& operator=(const HoocParserStaticData&) = delete;
  HoocParserStaticData& operator=(HoocParserStaticData&&) = delete;

  std::vector<antlr4::dfa::DFA> decisionToDFA;
  antlr4::atn::PredictionContextCache sharedContextCache;
  const std::vector<std::string> ruleNames;
  const std::vector<std::string> literalNames;
  const std::vector<std::string> symbolicNames;
  const antlr4::dfa::Vocabulary vocabulary;
  antlr4::atn::SerializedATNView serializedATN;
  std::unique_ptr<antlr4::atn::ATN> atn;
};

::antlr4::internal::OnceFlag hoocParserOnceFlag;
#if ANTLR4_USE_THREAD_LOCAL_CACHE
static thread_local
#endif
std::unique_ptr<HoocParserStaticData> hoocParserStaticData = nullptr;

void hoocParserInitialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  if (hoocParserStaticData != nullptr) {
    return;
  }
#else
  assert(hoocParserStaticData == nullptr);
#endif
  auto staticData = std::make_unique<HoocParserStaticData>(
    std::vector<std::string>{
      "compilationUnit", "importStatement", "modulePath", "qualifiedIdentifier", 
      "importItem", "declaration", "functionDeclaration", "parameterList", 
      "parameter", "classDeclaration", "classModifier", "classBody", "classMember", 
      "constructorDeclaration", "functionModifier", "variableDeclaration", 
      "constantDeclaration", "type", "optionalType", "arrayType", "baseType", 
      "mapType", "mapKeyType", "primitiveType", "ffiDeclaration", "ffiImportDeclaration", 
      "ffiLinkDeclaration", "ffiNativeFunction", "ffiNativeDeclaration", 
      "ffiParameterList", "ffiParameter", "ffiType", "librarySearchPaths", 
      "versionRange", "statement", "tryCatchStatement", "throwStatement", 
      "block", "variableDeclarationStatement", "expressionStatement", "ifStatement", 
      "forStatement", "whileStatement", "returnStatement", "scopeStatement", 
      "breakStatement", "continueStatement", "expression", "assignmentExpression", 
      "compoundAssignment", "logicalOrExpression", "logicalAndExpression", 
      "relationalExpression", "additiveExpression", "multiplicativeExpression", 
      "unaryExpression", "postfixExpression", "postfixSuffix", "augmentedAssignment", 
      "primary", "newExpression", "interpolatedString", "argumentList", 
      "expressionList"
    },
    std::vector<std::string>{
      "", "'func'", "'public'", "'private'", "'async'", "'return'", "'if'", 
      "'else'", "'for'", "'while'", "'in'", "'break'", "'continue'", "'class'", 
      "'extends'", "'import'", "'from'", "'new'", "'var'", "'const'", "'scope'", 
      "'final'", "'singleton'", "'immutable'", "'factory'", "'observable'", 
      "'constructor'", "'service'", "'strategy'", "'actor'", "'__hoo_init'", 
      "'as'", "'by'", "'this'", "'true'", "'false'", "'null'", "'try'", 
      "'catch'", "'finally'", "'throw'", "'rethrow'", "'map'", "'function'", 
      "'native'", "'extern'", "'pointer'", "'array'", "'at'", "'library'", 
      "'link'", "'dynamic'", "'int8'", "'byte'", "'int64'", "'float'", "'double'", 
      "'f64'", "'bool'", "'char'", "'string'", "'void'", "'+'", "'-'", "'*'", 
      "'/'", "'%'", "'='", "'+='", "'-='", "'*='", "'/='", "'%='", "'<<='", 
      "'>>='", "'++'", "'--'", "'=='", "'!='", "'<'", "'<='", "'>'", "'>='", 
      "'&&'", "'||'", "'!'", "'\\u003F'", "'->'", "'..'", "';'", "','", 
      "'.'", "':'", "'('", "')'", "'{'", "'}'", "'['", "']'"
    },
    std::vector<std::string>{
      "", "FUNC", "PUBLIC", "PRIVATE", "ASYNC", "RETURN", "IF", "ELSE", 
      "FOR", "WHILE", "IN", "BREAK", "CONTINUE", "CLASS", "EXTENDS", "IMPORT", 
      "FROM", "NEW", "VAR", "CONST", "SCOPE", "FINAL", "SINGLETON", "IMMUTABLE", 
      "FACTORY", "OBSERVABLE", "CONSTRUCTOR", "SERVICE", "STRATEGY", "ACTOR", 
      "HOO_INIT", "AS", "BY", "THIS", "TRUE", "FALSE", "NULL", "TRY", "CATCH", 
      "FINALLY", "THROW", "RETHROW", "MAP", "FUNCTION", "NATIVE", "EXTERN", 
      "POINTER", "ARRAY", "AT", "LIBRARY", "LINK", "DYNAMIC", "INT8", "BYTE", 
      "INT64", "FLOAT", "DOUBLE", "F64", "BOOL", "CHAR", "STRING", "VOID", 
      "PLUS", "MINUS", "MULTIPLY", "DIVIDE", "MODULO", "ASSIGN", "COMPOUND_PLUS", 
      "COMPOUND_MINUS", "COMPOUND_MULTIPLY", "COMPOUND_DIVIDE", "COMPOUND_MODULO", 
      "COMPOUND_LEFT_SHIFT", "COMPOUND_RIGHT_SHIFT", "INCREMENT", "DECREMENT", 
      "EQUALS", "NOT_EQUALS", "LESS", "LESS_EQUALS", "GREATER", "GREATER_EQUALS", 
      "AND", "OR", "NOT", "QUESTION", "ARROW", "RANGE", "SEMICOLON", "COMMA", 
      "DOT", "COLON", "LPAREN", "RPAREN", "LBRACE", "RBRACE", "LBRACKET", 
      "RBRACKET", "MULTILINE_STRING", "STRING_LITERAL", "CHAR_LITERAL", 
      "INTEGER_LITERAL", "FLOATING_LITERAL", "IDENTIFIER", "WS", "SINGLE_LINE_COMMENT", 
      "MULTI_LINE_COMMENT"
    }
  );
  static const int32_t serializedATNSegment[] = {
  	4,1,107,692,2,0,7,0,2,1,7,1,2,2,7,2,2,3,7,3,2,4,7,4,2,5,7,5,2,6,7,6,2,
  	7,7,7,2,8,7,8,2,9,7,9,2,10,7,10,2,11,7,11,2,12,7,12,2,13,7,13,2,14,7,
  	14,2,15,7,15,2,16,7,16,2,17,7,17,2,18,7,18,2,19,7,19,2,20,7,20,2,21,7,
  	21,2,22,7,22,2,23,7,23,2,24,7,24,2,25,7,25,2,26,7,26,2,27,7,27,2,28,7,
  	28,2,29,7,29,2,30,7,30,2,31,7,31,2,32,7,32,2,33,7,33,2,34,7,34,2,35,7,
  	35,2,36,7,36,2,37,7,37,2,38,7,38,2,39,7,39,2,40,7,40,2,41,7,41,2,42,7,
  	42,2,43,7,43,2,44,7,44,2,45,7,45,2,46,7,46,2,47,7,47,2,48,7,48,2,49,7,
  	49,2,50,7,50,2,51,7,51,2,52,7,52,2,53,7,53,2,54,7,54,2,55,7,55,2,56,7,
  	56,2,57,7,57,2,58,7,58,2,59,7,59,2,60,7,60,2,61,7,61,2,62,7,62,2,63,7,
  	63,1,0,5,0,130,8,0,10,0,12,0,133,9,0,1,0,1,0,3,0,137,8,0,1,0,5,0,140,
  	8,0,10,0,12,0,143,9,0,1,0,1,0,1,1,1,1,1,1,1,1,3,1,151,8,1,1,1,1,1,1,1,
  	1,1,1,1,1,1,1,1,1,1,5,1,161,8,1,10,1,12,1,164,9,1,1,1,1,1,3,1,168,8,1,
  	1,2,1,2,1,2,5,2,173,8,2,10,2,12,2,176,9,2,1,3,1,3,1,3,5,3,181,8,3,10,
  	3,12,3,184,9,3,1,4,1,4,1,4,3,4,189,8,4,1,5,1,5,1,5,1,5,3,5,195,8,5,1,
  	6,1,6,1,6,3,6,200,8,6,1,6,1,6,1,6,3,6,205,8,6,1,6,1,6,1,6,1,7,1,7,1,7,
  	5,7,213,8,7,10,7,12,7,216,9,7,1,8,1,8,1,8,1,8,1,9,5,9,223,8,9,10,9,12,
  	9,226,9,9,1,9,1,9,1,9,1,9,3,9,232,8,9,1,9,1,9,1,10,1,10,1,11,1,11,5,11,
  	240,8,11,10,11,12,11,243,9,11,1,11,1,11,1,12,1,12,1,12,1,12,1,12,5,12,
  	252,8,12,10,12,12,12,255,9,12,1,12,3,12,258,8,12,1,13,1,13,1,13,3,13,
  	263,8,13,1,13,1,13,1,13,1,14,1,14,1,15,1,15,1,15,1,15,1,15,1,15,1,15,
  	1,15,1,15,1,15,3,15,280,8,15,3,15,282,8,15,1,16,1,16,1,16,1,16,3,16,288,
  	8,16,1,16,1,16,1,16,1,17,1,17,3,17,295,8,17,1,18,1,18,3,18,299,8,18,1,
  	19,1,19,1,19,5,19,304,8,19,10,19,12,19,307,9,19,1,20,1,20,3,20,311,8,
  	20,1,21,1,21,1,21,1,21,1,21,1,21,1,21,1,22,1,22,1,23,1,23,1,24,1,24,1,
  	24,1,24,3,24,328,8,24,1,25,1,25,1,25,1,25,3,25,334,8,25,1,25,1,25,1,26,
  	1,26,1,26,1,26,1,26,3,26,343,8,26,1,26,3,26,346,8,26,1,26,1,26,1,27,1,
  	27,1,27,1,27,5,27,354,8,27,10,27,12,27,357,9,27,1,27,1,27,1,27,1,27,1,
  	27,3,27,364,8,27,1,27,1,27,1,27,3,27,369,8,27,1,27,1,27,3,27,373,8,27,
  	1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,1,28,3,28,384,8,28,1,29,1,29,
  	1,29,5,29,389,8,29,10,29,12,29,392,9,29,1,30,1,30,1,30,1,30,1,31,1,31,
  	1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,1,31,
  	1,31,5,31,415,8,31,10,31,12,31,418,9,31,1,31,1,31,1,31,3,31,423,8,31,
  	3,31,425,8,31,1,32,1,32,1,32,1,32,5,32,431,8,32,10,32,12,32,434,9,32,
  	1,32,1,32,1,33,1,33,3,33,440,8,33,1,33,1,33,1,33,3,33,445,8,33,1,33,1,
  	33,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,1,34,3,34,461,
  	8,34,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,1,35,5,35,473,8,35,
  	10,35,12,35,476,9,35,1,35,1,35,3,35,480,8,35,1,35,1,35,1,35,1,35,1,35,
  	3,35,487,8,35,1,36,1,36,1,36,1,36,1,36,1,36,3,36,495,8,36,1,37,1,37,5,
  	37,499,8,37,10,37,12,37,502,9,37,1,37,1,37,1,38,1,38,1,38,1,39,1,39,1,
  	39,1,40,1,40,1,40,1,40,1,40,3,40,517,8,40,1,41,1,41,1,41,1,41,1,41,1,
  	41,1,41,1,41,3,41,527,8,41,3,41,529,8,41,1,41,1,41,1,42,1,42,1,42,1,42,
  	1,43,1,43,3,43,539,8,43,1,43,1,43,1,44,1,44,1,44,1,45,1,45,1,45,1,46,
  	1,46,1,46,1,47,1,47,1,48,1,48,1,48,1,48,3,48,558,8,48,1,49,1,49,1,49,
  	1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,1,49,3,49,574,8,49,
  	1,50,1,50,1,50,5,50,579,8,50,10,50,12,50,582,9,50,1,51,1,51,1,51,5,51,
  	587,8,51,10,51,12,51,590,9,51,1,52,1,52,1,52,5,52,595,8,52,10,52,12,52,
  	598,9,52,1,53,1,53,1,53,5,53,603,8,53,10,53,12,53,606,9,53,1,54,1,54,
  	1,54,5,54,611,8,54,10,54,12,54,614,9,54,1,55,3,55,617,8,55,1,55,1,55,
  	1,56,1,56,1,56,5,56,624,8,56,10,56,12,56,627,9,56,1,57,1,57,1,57,1,57,
  	1,57,1,57,1,57,1,57,3,57,637,8,57,1,57,3,57,640,8,57,1,58,1,58,1,59,1,
  	59,1,59,1,59,1,59,1,59,1,59,1,59,1,59,1,59,1,59,1,59,3,59,656,8,59,1,
  	59,1,59,1,59,1,59,1,59,1,59,3,59,664,8,59,1,60,1,60,1,60,1,60,3,60,670,
  	8,60,1,60,1,60,1,61,1,61,1,62,1,62,1,62,5,62,679,8,62,10,62,12,62,682,
  	9,62,1,63,1,63,1,63,5,63,687,8,63,10,63,12,63,690,9,63,1,63,0,0,64,0,
  	2,4,6,8,10,12,14,16,18,20,22,24,26,28,30,32,34,36,38,40,42,44,46,48,50,
  	52,54,56,58,60,62,64,66,68,70,72,74,76,78,80,82,84,86,88,90,92,94,96,
  	98,100,102,104,106,108,110,112,114,116,118,120,122,124,126,0,9,2,0,21,
  	25,27,29,1,0,2,4,2,0,52,54,59,60,1,0,52,61,1,0,77,82,1,0,62,63,1,0,64,
  	66,2,0,63,63,85,85,1,0,75,76,733,0,131,1,0,0,0,2,167,1,0,0,0,4,169,1,
  	0,0,0,6,177,1,0,0,0,8,185,1,0,0,0,10,194,1,0,0,0,12,196,1,0,0,0,14,209,
  	1,0,0,0,16,217,1,0,0,0,18,224,1,0,0,0,20,235,1,0,0,0,22,237,1,0,0,0,24,
  	257,1,0,0,0,26,259,1,0,0,0,28,267,1,0,0,0,30,281,1,0,0,0,32,283,1,0,0,
  	0,34,294,1,0,0,0,36,296,1,0,0,0,38,300,1,0,0,0,40,310,1,0,0,0,42,312,
  	1,0,0,0,44,319,1,0,0,0,46,321,1,0,0,0,48,327,1,0,0,0,50,329,1,0,0,0,52,
  	337,1,0,0,0,54,372,1,0,0,0,56,383,1,0,0,0,58,385,1,0,0,0,60,393,1,0,0,
  	0,62,424,1,0,0,0,64,426,1,0,0,0,66,437,1,0,0,0,68,460,1,0,0,0,70,486,
  	1,0,0,0,72,494,1,0,0,0,74,496,1,0,0,0,76,505,1,0,0,0,78,508,1,0,0,0,80,
  	511,1,0,0,0,82,518,1,0,0,0,84,532,1,0,0,0,86,536,1,0,0,0,88,542,1,0,0,
  	0,90,545,1,0,0,0,92,548,1,0,0,0,94,551,1,0,0,0,96,553,1,0,0,0,98,573,
  	1,0,0,0,100,575,1,0,0,0,102,583,1,0,0,0,104,591,1,0,0,0,106,599,1,0,0,
  	0,108,607,1,0,0,0,110,616,1,0,0,0,112,620,1,0,0,0,114,639,1,0,0,0,116,
  	641,1,0,0,0,118,663,1,0,0,0,120,665,1,0,0,0,122,673,1,0,0,0,124,675,1,
  	0,0,0,126,683,1,0,0,0,128,130,3,2,1,0,129,128,1,0,0,0,130,133,1,0,0,0,
  	131,129,1,0,0,0,131,132,1,0,0,0,132,141,1,0,0,0,133,131,1,0,0,0,134,136,
  	3,10,5,0,135,137,5,89,0,0,136,135,1,0,0,0,136,137,1,0,0,0,137,140,1,0,
  	0,0,138,140,3,48,24,0,139,134,1,0,0,0,139,138,1,0,0,0,140,143,1,0,0,0,
  	141,139,1,0,0,0,141,142,1,0,0,0,142,144,1,0,0,0,143,141,1,0,0,0,144,145,
  	5,0,0,1,145,1,1,0,0,0,146,147,5,15,0,0,147,150,3,4,2,0,148,149,5,31,0,
  	0,149,151,5,104,0,0,150,148,1,0,0,0,150,151,1,0,0,0,151,152,1,0,0,0,152,
  	153,5,89,0,0,153,168,1,0,0,0,154,155,5,16,0,0,155,156,3,4,2,0,156,157,
  	5,15,0,0,157,162,3,8,4,0,158,159,5,90,0,0,159,161,3,8,4,0,160,158,1,0,
  	0,0,161,164,1,0,0,0,162,160,1,0,0,0,162,163,1,0,0,0,163,165,1,0,0,0,164,
  	162,1,0,0,0,165,166,5,89,0,0,166,168,1,0,0,0,167,146,1,0,0,0,167,154,
  	1,0,0,0,168,3,1,0,0,0,169,174,5,104,0,0,170,171,5,91,0,0,171,173,5,104,
  	0,0,172,170,1,0,0,0,173,176,1,0,0,0,174,172,1,0,0,0,174,175,1,0,0,0,175,
  	5,1,0,0,0,176,174,1,0,0,0,177,182,5,104,0,0,178,179,5,91,0,0,179,181,
  	5,104,0,0,180,178,1,0,0,0,181,184,1,0,0,0,182,180,1,0,0,0,182,183,1,0,
  	0,0,183,7,1,0,0,0,184,182,1,0,0,0,185,188,5,104,0,0,186,187,5,31,0,0,
  	187,189,5,104,0,0,188,186,1,0,0,0,188,189,1,0,0,0,189,9,1,0,0,0,190,195,
  	3,12,6,0,191,195,3,18,9,0,192,195,3,30,15,0,193,195,3,32,16,0,194,190,
  	1,0,0,0,194,191,1,0,0,0,194,192,1,0,0,0,194,193,1,0,0,0,195,11,1,0,0,
  	0,196,199,5,1,0,0,197,198,5,92,0,0,198,200,3,34,17,0,199,197,1,0,0,0,
  	199,200,1,0,0,0,200,201,1,0,0,0,201,202,5,104,0,0,202,204,5,93,0,0,203,
  	205,3,14,7,0,204,203,1,0,0,0,204,205,1,0,0,0,205,206,1,0,0,0,206,207,
  	5,94,0,0,207,208,3,74,37,0,208,13,1,0,0,0,209,214,3,16,8,0,210,211,5,
  	90,0,0,211,213,3,16,8,0,212,210,1,0,0,0,213,216,1,0,0,0,214,212,1,0,0,
  	0,214,215,1,0,0,0,215,15,1,0,0,0,216,214,1,0,0,0,217,218,5,104,0,0,218,
  	219,5,92,0,0,219,220,3,34,17,0,220,17,1,0,0,0,221,223,3,20,10,0,222,221,
  	1,0,0,0,223,226,1,0,0,0,224,222,1,0,0,0,224,225,1,0,0,0,225,227,1,0,0,
  	0,226,224,1,0,0,0,227,228,5,13,0,0,228,231,5,104,0,0,229,230,5,14,0,0,
  	230,232,5,104,0,0,231,229,1,0,0,0,231,232,1,0,0,0,232,233,1,0,0,0,233,
  	234,3,22,11,0,234,19,1,0,0,0,235,236,7,0,0,0,236,21,1,0,0,0,237,241,5,
  	95,0,0,238,240,3,24,12,0,239,238,1,0,0,0,240,243,1,0,0,0,241,239,1,0,
  	0,0,241,242,1,0,0,0,242,244,1,0,0,0,243,241,1,0,0,0,244,245,5,96,0,0,
  	245,23,1,0,0,0,246,247,3,30,15,0,247,248,5,89,0,0,248,258,1,0,0,0,249,
  	258,3,26,13,0,250,252,3,28,14,0,251,250,1,0,0,0,252,255,1,0,0,0,253,251,
  	1,0,0,0,253,254,1,0,0,0,254,256,1,0,0,0,255,253,1,0,0,0,256,258,3,12,
  	6,0,257,246,1,0,0,0,257,249,1,0,0,0,257,253,1,0,0,0,258,25,1,0,0,0,259,
  	260,5,26,0,0,260,262,5,93,0,0,261,263,3,14,7,0,262,261,1,0,0,0,262,263,
  	1,0,0,0,263,264,1,0,0,0,264,265,5,94,0,0,265,266,3,74,37,0,266,27,1,0,
  	0,0,267,268,7,1,0,0,268,29,1,0,0,0,269,270,5,18,0,0,270,271,5,104,0,0,
  	271,272,5,67,0,0,272,282,3,94,47,0,273,274,5,18,0,0,274,275,5,104,0,0,
  	275,276,5,92,0,0,276,279,3,34,17,0,277,278,5,67,0,0,278,280,3,94,47,0,
  	279,277,1,0,0,0,279,280,1,0,0,0,280,282,1,0,0,0,281,269,1,0,0,0,281,273,
  	1,0,0,0,282,31,1,0,0,0,283,284,5,19,0,0,284,287,5,104,0,0,285,286,5,92,
  	0,0,286,288,3,34,17,0,287,285,1,0,0,0,287,288,1,0,0,0,288,289,1,0,0,0,
  	289,290,5,67,0,0,290,291,3,94,47,0,291,33,1,0,0,0,292,295,3,36,18,0,293,
  	295,3,42,21,0,294,292,1,0,0,0,294,293,1,0,0,0,295,35,1,0,0,0,296,298,
  	3,38,19,0,297,299,5,86,0,0,298,297,1,0,0,0,298,299,1,0,0,0,299,37,1,0,
  	0,0,300,305,3,40,20,0,301,302,5,97,0,0,302,304,5,98,0,0,303,301,1,0,0,
  	0,304,307,1,0,0,0,305,303,1,0,0,0,305,306,1,0,0,0,306,39,1,0,0,0,307,
  	305,1,0,0,0,308,311,3,46,23,0,309,311,3,6,3,0,310,308,1,0,0,0,310,309,
  	1,0,0,0,311,41,1,0,0,0,312,313,5,42,0,0,313,314,5,97,0,0,314,315,3,44,
  	22,0,315,316,5,90,0,0,316,317,3,34,17,0,317,318,5,98,0,0,318,43,1,0,0,
  	0,319,320,7,2,0,0,320,45,1,0,0,0,321,322,7,3,0,0,322,47,1,0,0,0,323,328,
  	3,50,25,0,324,328,3,52,26,0,325,328,3,54,27,0,326,328,3,56,28,0,327,323,
  	1,0,0,0,327,324,1,0,0,0,327,325,1,0,0,0,327,326,1,0,0,0,328,49,1,0,0,
  	0,329,330,5,49,0,0,330,333,5,100,0,0,331,332,5,31,0,0,332,334,5,104,0,
  	0,333,331,1,0,0,0,333,334,1,0,0,0,334,335,1,0,0,0,335,336,5,89,0,0,336,
  	51,1,0,0,0,337,338,5,50,0,0,338,339,5,51,0,0,339,342,3,4,2,0,340,341,
  	5,48,0,0,341,343,3,66,33,0,342,340,1,0,0,0,342,343,1,0,0,0,343,345,1,
  	0,0,0,344,346,3,64,32,0,345,344,1,0,0,0,345,346,1,0,0,0,346,347,1,0,0,
  	0,347,348,5,89,0,0,348,53,1,0,0,0,349,350,5,44,0,0,350,373,3,12,6,0,351,
  	355,5,45,0,0,352,354,3,28,14,0,353,352,1,0,0,0,354,357,1,0,0,0,355,353,
  	1,0,0,0,355,356,1,0,0,0,356,358,1,0,0,0,357,355,1,0,0,0,358,359,5,44,
  	0,0,359,360,3,34,17,0,360,361,5,104,0,0,361,363,5,93,0,0,362,364,3,58,
  	29,0,363,362,1,0,0,0,363,364,1,0,0,0,364,365,1,0,0,0,365,368,5,94,0,0,
  	366,367,5,87,0,0,367,369,3,34,17,0,368,366,1,0,0,0,368,369,1,0,0,0,369,
  	370,1,0,0,0,370,371,5,89,0,0,371,373,1,0,0,0,372,349,1,0,0,0,372,351,
  	1,0,0,0,373,55,1,0,0,0,374,375,5,44,0,0,375,376,3,30,15,0,376,377,5,89,
  	0,0,377,384,1,0,0,0,378,379,5,45,0,0,379,380,5,44,0,0,380,381,3,30,15,
  	0,381,382,5,89,0,0,382,384,1,0,0,0,383,374,1,0,0,0,383,378,1,0,0,0,384,
  	57,1,0,0,0,385,390,3,60,30,0,386,387,5,90,0,0,387,389,3,60,30,0,388,386,
  	1,0,0,0,389,392,1,0,0,0,390,388,1,0,0,0,390,391,1,0,0,0,391,59,1,0,0,
  	0,392,390,1,0,0,0,393,394,5,104,0,0,394,395,5,92,0,0,395,396,3,62,31,
  	0,396,61,1,0,0,0,397,425,3,46,23,0,398,425,3,6,3,0,399,400,5,46,0,0,400,
  	401,5,97,0,0,401,402,3,62,31,0,402,403,5,98,0,0,403,425,1,0,0,0,404,405,
  	5,47,0,0,405,406,5,97,0,0,406,407,5,102,0,0,407,408,5,98,0,0,408,425,
  	3,62,31,0,409,410,5,43,0,0,410,411,5,93,0,0,411,416,3,62,31,0,412,413,
  	5,90,0,0,413,415,3,62,31,0,414,412,1,0,0,0,415,418,1,0,0,0,416,414,1,
  	0,0,0,416,417,1,0,0,0,417,419,1,0,0,0,418,416,1,0,0,0,419,422,5,94,0,
  	0,420,421,5,87,0,0,421,423,3,62,31,0,422,420,1,0,0,0,422,423,1,0,0,0,
  	423,425,1,0,0,0,424,397,1,0,0,0,424,398,1,0,0,0,424,399,1,0,0,0,424,404,
  	1,0,0,0,424,409,1,0,0,0,425,63,1,0,0,0,426,427,5,97,0,0,427,432,5,100,
  	0,0,428,429,5,90,0,0,429,431,5,100,0,0,430,428,1,0,0,0,431,434,1,0,0,
  	0,432,430,1,0,0,0,432,433,1,0,0,0,433,435,1,0,0,0,434,432,1,0,0,0,435,
  	436,5,98,0,0,436,65,1,0,0,0,437,439,5,97,0,0,438,440,5,102,0,0,439,438,
  	1,0,0,0,439,440,1,0,0,0,440,441,1,0,0,0,441,442,5,91,0,0,442,444,5,91,
  	0,0,443,445,5,102,0,0,444,443,1,0,0,0,444,445,1,0,0,0,445,446,1,0,0,0,
  	446,447,5,98,0,0,447,67,1,0,0,0,448,461,3,74,37,0,449,461,3,76,38,0,450,
  	461,3,78,39,0,451,461,3,86,43,0,452,461,3,80,40,0,453,461,3,84,42,0,454,
  	461,3,82,41,0,455,461,3,88,44,0,456,461,3,90,45,0,457,461,3,92,46,0,458,
  	461,3,70,35,0,459,461,3,72,36,0,460,448,1,0,0,0,460,449,1,0,0,0,460,450,
  	1,0,0,0,460,451,1,0,0,0,460,452,1,0,0,0,460,453,1,0,0,0,460,454,1,0,0,
  	0,460,455,1,0,0,0,460,456,1,0,0,0,460,457,1,0,0,0,460,458,1,0,0,0,460,
  	459,1,0,0,0,461,69,1,0,0,0,462,463,5,37,0,0,463,474,3,74,37,0,464,465,
  	5,38,0,0,465,466,5,93,0,0,466,467,5,104,0,0,467,468,5,92,0,0,468,469,
  	3,34,17,0,469,470,5,94,0,0,470,471,3,74,37,0,471,473,1,0,0,0,472,464,
  	1,0,0,0,473,476,1,0,0,0,474,472,1,0,0,0,474,475,1,0,0,0,475,479,1,0,0,
  	0,476,474,1,0,0,0,477,478,5,39,0,0,478,480,3,74,37,0,479,477,1,0,0,0,
  	479,480,1,0,0,0,480,487,1,0,0,0,481,482,5,37,0,0,482,483,3,74,37,0,483,
  	484,5,39,0,0,484,485,3,74,37,0,485,487,1,0,0,0,486,462,1,0,0,0,486,481,
  	1,0,0,0,487,71,1,0,0,0,488,489,5,40,0,0,489,490,3,94,47,0,490,491,5,89,
  	0,0,491,495,1,0,0,0,492,493,5,41,0,0,493,495,5,89,0,0,494,488,1,0,0,0,
  	494,492,1,0,0,0,495,73,1,0,0,0,496,500,5,95,0,0,497,499,3,68,34,0,498,
  	497,1,0,0,0,499,502,1,0,0,0,500,498,1,0,0,0,500,501,1,0,0,0,501,503,1,
  	0,0,0,502,500,1,0,0,0,503,504,5,96,0,0,504,75,1,0,0,0,505,506,3,30,15,
  	0,506,507,5,89,0,0,507,77,1,0,0,0,508,509,3,94,47,0,509,510,5,89,0,0,
  	510,79,1,0,0,0,511,512,5,6,0,0,512,513,3,94,47,0,513,516,3,74,37,0,514,
  	515,5,7,0,0,515,517,3,74,37,0,516,514,1,0,0,0,516,517,1,0,0,0,517,81,
  	1,0,0,0,518,519,5,8,0,0,519,520,5,104,0,0,520,521,5,10,0,0,521,528,3,
  	94,47,0,522,523,5,88,0,0,523,526,3,94,47,0,524,525,5,32,0,0,525,527,3,
  	94,47,0,526,524,1,0,0,0,526,527,1,0,0,0,527,529,1,0,0,0,528,522,1,0,0,
  	0,528,529,1,0,0,0,529,530,1,0,0,0,530,531,3,74,37,0,531,83,1,0,0,0,532,
  	533,5,9,0,0,533,534,3,94,47,0,534,535,3,74,37,0,535,85,1,0,0,0,536,538,
  	5,5,0,0,537,539,3,94,47,0,538,537,1,0,0,0,538,539,1,0,0,0,539,540,1,0,
  	0,0,540,541,5,89,0,0,541,87,1,0,0,0,542,543,5,20,0,0,543,544,3,74,37,
  	0,544,89,1,0,0,0,545,546,5,11,0,0,546,547,5,89,0,0,547,91,1,0,0,0,548,
  	549,5,12,0,0,549,550,5,89,0,0,550,93,1,0,0,0,551,552,3,96,48,0,552,95,
  	1,0,0,0,553,557,3,100,50,0,554,555,5,67,0,0,555,558,3,100,50,0,556,558,
  	3,98,49,0,557,554,1,0,0,0,557,556,1,0,0,0,557,558,1,0,0,0,558,97,1,0,
  	0,0,559,560,5,68,0,0,560,574,3,100,50,0,561,562,5,69,0,0,562,574,3,100,
  	50,0,563,564,5,70,0,0,564,574,3,100,50,0,565,566,5,71,0,0,566,574,3,100,
  	50,0,567,568,5,72,0,0,568,574,3,100,50,0,569,570,5,73,0,0,570,574,3,100,
  	50,0,571,572,5,74,0,0,572,574,3,100,50,0,573,559,1,0,0,0,573,561,1,0,
  	0,0,573,563,1,0,0,0,573,565,1,0,0,0,573,567,1,0,0,0,573,569,1,0,0,0,573,
  	571,1,0,0,0,574,99,1,0,0,0,575,580,3,102,51,0,576,577,5,84,0,0,577,579,
  	3,102,51,0,578,576,1,0,0,0,579,582,1,0,0,0,580,578,1,0,0,0,580,581,1,
  	0,0,0,581,101,1,0,0,0,582,580,1,0,0,0,583,588,3,104,52,0,584,585,5,83,
  	0,0,585,587,3,104,52,0,586,584,1,0,0,0,587,590,1,0,0,0,588,586,1,0,0,
  	0,588,589,1,0,0,0,589,103,1,0,0,0,590,588,1,0,0,0,591,596,3,106,53,0,
  	592,593,7,4,0,0,593,595,3,106,53,0,594,592,1,0,0,0,595,598,1,0,0,0,596,
  	594,1,0,0,0,596,597,1,0,0,0,597,105,1,0,0,0,598,596,1,0,0,0,599,604,3,
  	108,54,0,600,601,7,5,0,0,601,603,3,108,54,0,602,600,1,0,0,0,603,606,1,
  	0,0,0,604,602,1,0,0,0,604,605,1,0,0,0,605,107,1,0,0,0,606,604,1,0,0,0,
  	607,612,3,110,55,0,608,609,7,6,0,0,609,611,3,110,55,0,610,608,1,0,0,0,
  	611,614,1,0,0,0,612,610,1,0,0,0,612,613,1,0,0,0,613,109,1,0,0,0,614,612,
  	1,0,0,0,615,617,7,7,0,0,616,615,1,0,0,0,616,617,1,0,0,0,617,618,1,0,0,
  	0,618,619,3,112,56,0,619,111,1,0,0,0,620,625,3,118,59,0,621,624,3,114,
  	57,0,622,624,3,116,58,0,623,621,1,0,0,0,623,622,1,0,0,0,624,627,1,0,0,
  	0,625,623,1,0,0,0,625,626,1,0,0,0,626,113,1,0,0,0,627,625,1,0,0,0,628,
  	629,5,91,0,0,629,640,5,104,0,0,630,631,5,97,0,0,631,632,3,94,47,0,632,
  	633,5,98,0,0,633,640,1,0,0,0,634,636,5,93,0,0,635,637,3,124,62,0,636,
  	635,1,0,0,0,636,637,1,0,0,0,637,638,1,0,0,0,638,640,5,94,0,0,639,628,
  	1,0,0,0,639,630,1,0,0,0,639,634,1,0,0,0,640,115,1,0,0,0,641,642,7,8,0,
  	0,642,117,1,0,0,0,643,664,5,104,0,0,644,664,5,33,0,0,645,664,5,102,0,
  	0,646,664,5,103,0,0,647,664,5,100,0,0,648,664,5,99,0,0,649,664,5,101,
  	0,0,650,664,5,34,0,0,651,664,5,35,0,0,652,664,5,36,0,0,653,655,5,97,0,
  	0,654,656,3,126,63,0,655,654,1,0,0,0,655,656,1,0,0,0,656,657,1,0,0,0,
  	657,664,5,98,0,0,658,659,5,93,0,0,659,660,3,94,47,0,660,661,5,94,0,0,
  	661,664,1,0,0,0,662,664,3,120,60,0,663,643,1,0,0,0,663,644,1,0,0,0,663,
  	645,1,0,0,0,663,646,1,0,0,0,663,647,1,0,0,0,663,648,1,0,0,0,663,649,1,
  	0,0,0,663,650,1,0,0,0,663,651,1,0,0,0,663,652,1,0,0,0,663,653,1,0,0,0,
  	663,658,1,0,0,0,663,662,1,0,0,0,664,119,1,0,0,0,665,666,5,17,0,0,666,
  	667,3,6,3,0,667,669,5,93,0,0,668,670,3,124,62,0,669,668,1,0,0,0,669,670,
  	1,0,0,0,670,671,1,0,0,0,671,672,5,94,0,0,672,121,1,0,0,0,673,674,5,100,
  	0,0,674,123,1,0,0,0,675,680,3,94,47,0,676,677,5,90,0,0,677,679,3,94,47,
  	0,678,676,1,0,0,0,679,682,1,0,0,0,680,678,1,0,0,0,680,681,1,0,0,0,681,
  	125,1,0,0,0,682,680,1,0,0,0,683,688,3,94,47,0,684,685,5,90,0,0,685,687,
  	3,94,47,0,686,684,1,0,0,0,687,690,1,0,0,0,688,686,1,0,0,0,688,689,1,0,
  	0,0,689,127,1,0,0,0,690,688,1,0,0,0,70,131,136,139,141,150,162,167,174,
  	182,188,194,199,204,214,224,231,241,253,257,262,279,281,287,294,298,305,
  	310,327,333,342,345,355,363,368,372,383,390,416,422,424,432,439,444,460,
  	474,479,486,494,500,516,526,528,538,557,573,580,588,596,604,612,616,623,
  	625,636,639,655,663,669,680,688
  };
  staticData->serializedATN = antlr4::atn::SerializedATNView(serializedATNSegment, sizeof(serializedATNSegment) / sizeof(serializedATNSegment[0]));

  antlr4::atn::ATNDeserializer deserializer;
  staticData->atn = deserializer.deserialize(staticData->serializedATN);

  const size_t count = staticData->atn->getNumberOfDecisions();
  staticData->decisionToDFA.reserve(count);
  for (size_t i = 0; i < count; i++) { 
    staticData->decisionToDFA.emplace_back(staticData->atn->getDecisionState(i), i);
  }
  hoocParserStaticData = std::move(staticData);
}

}

HoocParser::HoocParser(TokenStream *input) : HoocParser(input, antlr4::atn::ParserATNSimulatorOptions()) {}

HoocParser::HoocParser(TokenStream *input, const antlr4::atn::ParserATNSimulatorOptions &options) : Parser(input) {
  HoocParser::initialize();
  _interpreter = new atn::ParserATNSimulator(this, *hoocParserStaticData->atn, hoocParserStaticData->decisionToDFA, hoocParserStaticData->sharedContextCache, options);
}

HoocParser::~HoocParser() {
  delete _interpreter;
}

const atn::ATN& HoocParser::getATN() const {
  return *hoocParserStaticData->atn;
}

std::string HoocParser::getGrammarFileName() const {
  return "Hooc.g4";
}

const std::vector<std::string>& HoocParser::getRuleNames() const {
  return hoocParserStaticData->ruleNames;
}

const dfa::Vocabulary& HoocParser::getVocabulary() const {
  return hoocParserStaticData->vocabulary;
}

antlr4::atn::SerializedATNView HoocParser::getSerializedATN() const {
  return hoocParserStaticData->serializedATN;
}


//----------------- CompilationUnitContext ------------------------------------------------------------------

HoocParser::CompilationUnitContext::CompilationUnitContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::CompilationUnitContext::EOF() {
  return getToken(HoocParser::EOF, 0);
}

std::vector<HoocParser::ImportStatementContext *> HoocParser::CompilationUnitContext::importStatement() {
  return getRuleContexts<HoocParser::ImportStatementContext>();
}

HoocParser::ImportStatementContext* HoocParser::CompilationUnitContext::importStatement(size_t i) {
  return getRuleContext<HoocParser::ImportStatementContext>(i);
}

std::vector<HoocParser::FfiDeclarationContext *> HoocParser::CompilationUnitContext::ffiDeclaration() {
  return getRuleContexts<HoocParser::FfiDeclarationContext>();
}

HoocParser::FfiDeclarationContext* HoocParser::CompilationUnitContext::ffiDeclaration(size_t i) {
  return getRuleContext<HoocParser::FfiDeclarationContext>(i);
}

std::vector<HoocParser::DeclarationContext *> HoocParser::CompilationUnitContext::declaration() {
  return getRuleContexts<HoocParser::DeclarationContext>();
}

HoocParser::DeclarationContext* HoocParser::CompilationUnitContext::declaration(size_t i) {
  return getRuleContext<HoocParser::DeclarationContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::CompilationUnitContext::SEMICOLON() {
  return getTokens(HoocParser::SEMICOLON);
}

tree::TerminalNode* HoocParser::CompilationUnitContext::SEMICOLON(size_t i) {
  return getToken(HoocParser::SEMICOLON, i);
}


size_t HoocParser::CompilationUnitContext::getRuleIndex() const {
  return HoocParser::RuleCompilationUnit;
}

void HoocParser::CompilationUnitContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompilationUnit(this);
}

void HoocParser::CompilationUnitContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompilationUnit(this);
}


std::any HoocParser::CompilationUnitContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitCompilationUnit(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::CompilationUnitContext* HoocParser::compilationUnit() {
  CompilationUnitContext *_localctx = _tracker.createInstance<CompilationUnitContext>(_ctx, getState());
  enterRule(_localctx, 0, HoocParser::RuleCompilationUnit);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(131);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::IMPORT

    || _la == HoocParser::FROM) {
      setState(128);
      importStatement();
      setState(133);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(141);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1741627423727618) != 0)) {
      setState(139);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case HoocParser::FUNC:
        case HoocParser::CLASS:
        case HoocParser::VAR:
        case HoocParser::CONST:
        case HoocParser::FINAL:
        case HoocParser::SINGLETON:
        case HoocParser::IMMUTABLE:
        case HoocParser::FACTORY:
        case HoocParser::OBSERVABLE:
        case HoocParser::SERVICE:
        case HoocParser::STRATEGY:
        case HoocParser::ACTOR: {
          setState(134);
          declaration();
          setState(136);
          _errHandler->sync(this);

          _la = _input->LA(1);
          if (_la == HoocParser::SEMICOLON) {
            setState(135);
            match(HoocParser::SEMICOLON);
          }
          break;
        }

        case HoocParser::NATIVE:
        case HoocParser::EXTERN:
        case HoocParser::LIBRARY:
        case HoocParser::LINK: {
          setState(138);
          ffiDeclaration();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(143);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(144);
    match(HoocParser::EOF);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ImportStatementContext ------------------------------------------------------------------

HoocParser::ImportStatementContext::ImportStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}


size_t HoocParser::ImportStatementContext::getRuleIndex() const {
  return HoocParser::RuleImportStatement;
}

void HoocParser::ImportStatementContext::copyFrom(ImportStatementContext *ctx) {
  ParserRuleContext::copyFrom(ctx);
}

//----------------- FromImportContext ------------------------------------------------------------------

tree::TerminalNode* HoocParser::FromImportContext::FROM() {
  return getToken(HoocParser::FROM, 0);
}

HoocParser::ModulePathContext* HoocParser::FromImportContext::modulePath() {
  return getRuleContext<HoocParser::ModulePathContext>(0);
}

tree::TerminalNode* HoocParser::FromImportContext::IMPORT() {
  return getToken(HoocParser::IMPORT, 0);
}

std::vector<HoocParser::ImportItemContext *> HoocParser::FromImportContext::importItem() {
  return getRuleContexts<HoocParser::ImportItemContext>();
}

HoocParser::ImportItemContext* HoocParser::FromImportContext::importItem(size_t i) {
  return getRuleContext<HoocParser::ImportItemContext>(i);
}

tree::TerminalNode* HoocParser::FromImportContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

std::vector<tree::TerminalNode *> HoocParser::FromImportContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::FromImportContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}

HoocParser::FromImportContext::FromImportContext(ImportStatementContext *ctx) { copyFrom(ctx); }

void HoocParser::FromImportContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFromImport(this);
}
void HoocParser::FromImportContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFromImport(this);
}

std::any HoocParser::FromImportContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFromImport(this);
  else
    return visitor->visitChildren(this);
}
//----------------- BasicImportContext ------------------------------------------------------------------

tree::TerminalNode* HoocParser::BasicImportContext::IMPORT() {
  return getToken(HoocParser::IMPORT, 0);
}

HoocParser::ModulePathContext* HoocParser::BasicImportContext::modulePath() {
  return getRuleContext<HoocParser::ModulePathContext>(0);
}

tree::TerminalNode* HoocParser::BasicImportContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

tree::TerminalNode* HoocParser::BasicImportContext::AS() {
  return getToken(HoocParser::AS, 0);
}

tree::TerminalNode* HoocParser::BasicImportContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

HoocParser::BasicImportContext::BasicImportContext(ImportStatementContext *ctx) { copyFrom(ctx); }

void HoocParser::BasicImportContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBasicImport(this);
}
void HoocParser::BasicImportContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBasicImport(this);
}

std::any HoocParser::BasicImportContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitBasicImport(this);
  else
    return visitor->visitChildren(this);
}
HoocParser::ImportStatementContext* HoocParser::importStatement() {
  ImportStatementContext *_localctx = _tracker.createInstance<ImportStatementContext>(_ctx, getState());
  enterRule(_localctx, 2, HoocParser::RuleImportStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(167);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::IMPORT: {
        _localctx = _tracker.createInstance<HoocParser::BasicImportContext>(_localctx);
        enterOuterAlt(_localctx, 1);
        setState(146);
        match(HoocParser::IMPORT);
        setState(147);
        modulePath();
        setState(150);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == HoocParser::AS) {
          setState(148);
          match(HoocParser::AS);
          setState(149);
          match(HoocParser::IDENTIFIER);
        }
        setState(152);
        match(HoocParser::SEMICOLON);
        break;
      }

      case HoocParser::FROM: {
        _localctx = _tracker.createInstance<HoocParser::FromImportContext>(_localctx);
        enterOuterAlt(_localctx, 2);
        setState(154);
        match(HoocParser::FROM);
        setState(155);
        modulePath();
        setState(156);
        match(HoocParser::IMPORT);
        setState(157);
        importItem();
        setState(162);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == HoocParser::COMMA) {
          setState(158);
          match(HoocParser::COMMA);
          setState(159);
          importItem();
          setState(164);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(165);
        match(HoocParser::SEMICOLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ModulePathContext ------------------------------------------------------------------

HoocParser::ModulePathContext::ModulePathContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> HoocParser::ModulePathContext::IDENTIFIER() {
  return getTokens(HoocParser::IDENTIFIER);
}

tree::TerminalNode* HoocParser::ModulePathContext::IDENTIFIER(size_t i) {
  return getToken(HoocParser::IDENTIFIER, i);
}

std::vector<tree::TerminalNode *> HoocParser::ModulePathContext::DOT() {
  return getTokens(HoocParser::DOT);
}

tree::TerminalNode* HoocParser::ModulePathContext::DOT(size_t i) {
  return getToken(HoocParser::DOT, i);
}


size_t HoocParser::ModulePathContext::getRuleIndex() const {
  return HoocParser::RuleModulePath;
}

void HoocParser::ModulePathContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterModulePath(this);
}

void HoocParser::ModulePathContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitModulePath(this);
}


std::any HoocParser::ModulePathContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitModulePath(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ModulePathContext* HoocParser::modulePath() {
  ModulePathContext *_localctx = _tracker.createInstance<ModulePathContext>(_ctx, getState());
  enterRule(_localctx, 4, HoocParser::RuleModulePath);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(169);
    match(HoocParser::IDENTIFIER);
    setState(174);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::DOT) {
      setState(170);
      match(HoocParser::DOT);
      setState(171);
      match(HoocParser::IDENTIFIER);
      setState(176);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- QualifiedIdentifierContext ------------------------------------------------------------------

HoocParser::QualifiedIdentifierContext::QualifiedIdentifierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> HoocParser::QualifiedIdentifierContext::IDENTIFIER() {
  return getTokens(HoocParser::IDENTIFIER);
}

tree::TerminalNode* HoocParser::QualifiedIdentifierContext::IDENTIFIER(size_t i) {
  return getToken(HoocParser::IDENTIFIER, i);
}

std::vector<tree::TerminalNode *> HoocParser::QualifiedIdentifierContext::DOT() {
  return getTokens(HoocParser::DOT);
}

tree::TerminalNode* HoocParser::QualifiedIdentifierContext::DOT(size_t i) {
  return getToken(HoocParser::DOT, i);
}


size_t HoocParser::QualifiedIdentifierContext::getRuleIndex() const {
  return HoocParser::RuleQualifiedIdentifier;
}

void HoocParser::QualifiedIdentifierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterQualifiedIdentifier(this);
}

void HoocParser::QualifiedIdentifierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitQualifiedIdentifier(this);
}


std::any HoocParser::QualifiedIdentifierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitQualifiedIdentifier(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::QualifiedIdentifierContext* HoocParser::qualifiedIdentifier() {
  QualifiedIdentifierContext *_localctx = _tracker.createInstance<QualifiedIdentifierContext>(_ctx, getState());
  enterRule(_localctx, 6, HoocParser::RuleQualifiedIdentifier);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(177);
    match(HoocParser::IDENTIFIER);
    setState(182);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::DOT) {
      setState(178);
      match(HoocParser::DOT);
      setState(179);
      match(HoocParser::IDENTIFIER);
      setState(184);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ImportItemContext ------------------------------------------------------------------

HoocParser::ImportItemContext::ImportItemContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<tree::TerminalNode *> HoocParser::ImportItemContext::IDENTIFIER() {
  return getTokens(HoocParser::IDENTIFIER);
}

tree::TerminalNode* HoocParser::ImportItemContext::IDENTIFIER(size_t i) {
  return getToken(HoocParser::IDENTIFIER, i);
}

tree::TerminalNode* HoocParser::ImportItemContext::AS() {
  return getToken(HoocParser::AS, 0);
}


size_t HoocParser::ImportItemContext::getRuleIndex() const {
  return HoocParser::RuleImportItem;
}

void HoocParser::ImportItemContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterImportItem(this);
}

void HoocParser::ImportItemContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitImportItem(this);
}


std::any HoocParser::ImportItemContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitImportItem(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ImportItemContext* HoocParser::importItem() {
  ImportItemContext *_localctx = _tracker.createInstance<ImportItemContext>(_ctx, getState());
  enterRule(_localctx, 8, HoocParser::RuleImportItem);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(185);
    match(HoocParser::IDENTIFIER);
    setState(188);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::AS) {
      setState(186);
      match(HoocParser::AS);
      setState(187);
      match(HoocParser::IDENTIFIER);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- DeclarationContext ------------------------------------------------------------------

HoocParser::DeclarationContext::DeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::FunctionDeclarationContext* HoocParser::DeclarationContext::functionDeclaration() {
  return getRuleContext<HoocParser::FunctionDeclarationContext>(0);
}

HoocParser::ClassDeclarationContext* HoocParser::DeclarationContext::classDeclaration() {
  return getRuleContext<HoocParser::ClassDeclarationContext>(0);
}

HoocParser::VariableDeclarationContext* HoocParser::DeclarationContext::variableDeclaration() {
  return getRuleContext<HoocParser::VariableDeclarationContext>(0);
}

HoocParser::ConstantDeclarationContext* HoocParser::DeclarationContext::constantDeclaration() {
  return getRuleContext<HoocParser::ConstantDeclarationContext>(0);
}


size_t HoocParser::DeclarationContext::getRuleIndex() const {
  return HoocParser::RuleDeclaration;
}

void HoocParser::DeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterDeclaration(this);
}

void HoocParser::DeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitDeclaration(this);
}


std::any HoocParser::DeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::DeclarationContext* HoocParser::declaration() {
  DeclarationContext *_localctx = _tracker.createInstance<DeclarationContext>(_ctx, getState());
  enterRule(_localctx, 10, HoocParser::RuleDeclaration);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(194);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::FUNC: {
        enterOuterAlt(_localctx, 1);
        setState(190);
        functionDeclaration();
        break;
      }

      case HoocParser::CLASS:
      case HoocParser::FINAL:
      case HoocParser::SINGLETON:
      case HoocParser::IMMUTABLE:
      case HoocParser::FACTORY:
      case HoocParser::OBSERVABLE:
      case HoocParser::SERVICE:
      case HoocParser::STRATEGY:
      case HoocParser::ACTOR: {
        enterOuterAlt(_localctx, 2);
        setState(191);
        classDeclaration();
        break;
      }

      case HoocParser::VAR: {
        enterOuterAlt(_localctx, 3);
        setState(192);
        variableDeclaration();
        break;
      }

      case HoocParser::CONST: {
        enterOuterAlt(_localctx, 4);
        setState(193);
        constantDeclaration();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FunctionDeclarationContext ------------------------------------------------------------------

HoocParser::FunctionDeclarationContext::FunctionDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FunctionDeclarationContext::FUNC() {
  return getToken(HoocParser::FUNC, 0);
}

tree::TerminalNode* HoocParser::FunctionDeclarationContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::FunctionDeclarationContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::FunctionDeclarationContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

HoocParser::BlockContext* HoocParser::FunctionDeclarationContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}

tree::TerminalNode* HoocParser::FunctionDeclarationContext::COLON() {
  return getToken(HoocParser::COLON, 0);
}

HoocParser::TypeContext* HoocParser::FunctionDeclarationContext::type() {
  return getRuleContext<HoocParser::TypeContext>(0);
}

HoocParser::ParameterListContext* HoocParser::FunctionDeclarationContext::parameterList() {
  return getRuleContext<HoocParser::ParameterListContext>(0);
}


size_t HoocParser::FunctionDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleFunctionDeclaration;
}

void HoocParser::FunctionDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFunctionDeclaration(this);
}

void HoocParser::FunctionDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFunctionDeclaration(this);
}


std::any HoocParser::FunctionDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFunctionDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FunctionDeclarationContext* HoocParser::functionDeclaration() {
  FunctionDeclarationContext *_localctx = _tracker.createInstance<FunctionDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 12, HoocParser::RuleFunctionDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(196);
    match(HoocParser::FUNC);
    setState(199);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::COLON) {
      setState(197);
      match(HoocParser::COLON);
      setState(198);
      type();
    }
    setState(201);
    match(HoocParser::IDENTIFIER);
    setState(202);
    match(HoocParser::LPAREN);
    setState(204);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::IDENTIFIER) {
      setState(203);
      parameterList();
    }
    setState(206);
    match(HoocParser::RPAREN);
    setState(207);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParameterListContext ------------------------------------------------------------------

HoocParser::ParameterListContext::ParameterListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::ParameterContext *> HoocParser::ParameterListContext::parameter() {
  return getRuleContexts<HoocParser::ParameterContext>();
}

HoocParser::ParameterContext* HoocParser::ParameterListContext::parameter(size_t i) {
  return getRuleContext<HoocParser::ParameterContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::ParameterListContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::ParameterListContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}


size_t HoocParser::ParameterListContext::getRuleIndex() const {
  return HoocParser::RuleParameterList;
}

void HoocParser::ParameterListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterParameterList(this);
}

void HoocParser::ParameterListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitParameterList(this);
}


std::any HoocParser::ParameterListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitParameterList(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ParameterListContext* HoocParser::parameterList() {
  ParameterListContext *_localctx = _tracker.createInstance<ParameterListContext>(_ctx, getState());
  enterRule(_localctx, 14, HoocParser::RuleParameterList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(209);
    parameter();
    setState(214);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::COMMA) {
      setState(210);
      match(HoocParser::COMMA);
      setState(211);
      parameter();
      setState(216);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ParameterContext ------------------------------------------------------------------

HoocParser::ParameterContext::ParameterContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ParameterContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::ParameterContext::COLON() {
  return getToken(HoocParser::COLON, 0);
}

HoocParser::TypeContext* HoocParser::ParameterContext::type() {
  return getRuleContext<HoocParser::TypeContext>(0);
}


size_t HoocParser::ParameterContext::getRuleIndex() const {
  return HoocParser::RuleParameter;
}

void HoocParser::ParameterContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterParameter(this);
}

void HoocParser::ParameterContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitParameter(this);
}


std::any HoocParser::ParameterContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitParameter(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ParameterContext* HoocParser::parameter() {
  ParameterContext *_localctx = _tracker.createInstance<ParameterContext>(_ctx, getState());
  enterRule(_localctx, 16, HoocParser::RuleParameter);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(217);
    match(HoocParser::IDENTIFIER);
    setState(218);
    match(HoocParser::COLON);
    setState(219);
    type();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassDeclarationContext ------------------------------------------------------------------

HoocParser::ClassDeclarationContext::ClassDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ClassDeclarationContext::CLASS() {
  return getToken(HoocParser::CLASS, 0);
}

std::vector<tree::TerminalNode *> HoocParser::ClassDeclarationContext::IDENTIFIER() {
  return getTokens(HoocParser::IDENTIFIER);
}

tree::TerminalNode* HoocParser::ClassDeclarationContext::IDENTIFIER(size_t i) {
  return getToken(HoocParser::IDENTIFIER, i);
}

HoocParser::ClassBodyContext* HoocParser::ClassDeclarationContext::classBody() {
  return getRuleContext<HoocParser::ClassBodyContext>(0);
}

std::vector<HoocParser::ClassModifierContext *> HoocParser::ClassDeclarationContext::classModifier() {
  return getRuleContexts<HoocParser::ClassModifierContext>();
}

HoocParser::ClassModifierContext* HoocParser::ClassDeclarationContext::classModifier(size_t i) {
  return getRuleContext<HoocParser::ClassModifierContext>(i);
}

tree::TerminalNode* HoocParser::ClassDeclarationContext::EXTENDS() {
  return getToken(HoocParser::EXTENDS, 0);
}


size_t HoocParser::ClassDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleClassDeclaration;
}

void HoocParser::ClassDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassDeclaration(this);
}

void HoocParser::ClassDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassDeclaration(this);
}


std::any HoocParser::ClassDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitClassDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ClassDeclarationContext* HoocParser::classDeclaration() {
  ClassDeclarationContext *_localctx = _tracker.createInstance<ClassDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 18, HoocParser::RuleClassDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(224);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1004535808) != 0)) {
      setState(221);
      classModifier();
      setState(226);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(227);
    match(HoocParser::CLASS);
    setState(228);
    match(HoocParser::IDENTIFIER);
    setState(231);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::EXTENDS) {
      setState(229);
      match(HoocParser::EXTENDS);
      setState(230);
      match(HoocParser::IDENTIFIER);
    }
    setState(233);
    classBody();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassModifierContext ------------------------------------------------------------------

HoocParser::ClassModifierContext::ClassModifierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ClassModifierContext::SINGLETON() {
  return getToken(HoocParser::SINGLETON, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::IMMUTABLE() {
  return getToken(HoocParser::IMMUTABLE, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::FACTORY() {
  return getToken(HoocParser::FACTORY, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::OBSERVABLE() {
  return getToken(HoocParser::OBSERVABLE, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::SERVICE() {
  return getToken(HoocParser::SERVICE, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::STRATEGY() {
  return getToken(HoocParser::STRATEGY, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::ACTOR() {
  return getToken(HoocParser::ACTOR, 0);
}

tree::TerminalNode* HoocParser::ClassModifierContext::FINAL() {
  return getToken(HoocParser::FINAL, 0);
}


size_t HoocParser::ClassModifierContext::getRuleIndex() const {
  return HoocParser::RuleClassModifier;
}

void HoocParser::ClassModifierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassModifier(this);
}

void HoocParser::ClassModifierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassModifier(this);
}


std::any HoocParser::ClassModifierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitClassModifier(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ClassModifierContext* HoocParser::classModifier() {
  ClassModifierContext *_localctx = _tracker.createInstance<ClassModifierContext>(_ctx, getState());
  enterRule(_localctx, 20, HoocParser::RuleClassModifier);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(235);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1004535808) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassBodyContext ------------------------------------------------------------------

HoocParser::ClassBodyContext::ClassBodyContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ClassBodyContext::LBRACE() {
  return getToken(HoocParser::LBRACE, 0);
}

tree::TerminalNode* HoocParser::ClassBodyContext::RBRACE() {
  return getToken(HoocParser::RBRACE, 0);
}

std::vector<HoocParser::ClassMemberContext *> HoocParser::ClassBodyContext::classMember() {
  return getRuleContexts<HoocParser::ClassMemberContext>();
}

HoocParser::ClassMemberContext* HoocParser::ClassBodyContext::classMember(size_t i) {
  return getRuleContext<HoocParser::ClassMemberContext>(i);
}


size_t HoocParser::ClassBodyContext::getRuleIndex() const {
  return HoocParser::RuleClassBody;
}

void HoocParser::ClassBodyContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassBody(this);
}

void HoocParser::ClassBodyContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassBody(this);
}


std::any HoocParser::ClassBodyContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitClassBody(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ClassBodyContext* HoocParser::classBody() {
  ClassBodyContext *_localctx = _tracker.createInstance<ClassBodyContext>(_ctx, getState());
  enterRule(_localctx, 22, HoocParser::RuleClassBody);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(237);
    match(HoocParser::LBRACE);
    setState(241);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 67371038) != 0)) {
      setState(238);
      classMember();
      setState(243);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(244);
    match(HoocParser::RBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ClassMemberContext ------------------------------------------------------------------

HoocParser::ClassMemberContext::ClassMemberContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::VariableDeclarationContext* HoocParser::ClassMemberContext::variableDeclaration() {
  return getRuleContext<HoocParser::VariableDeclarationContext>(0);
}

tree::TerminalNode* HoocParser::ClassMemberContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

HoocParser::ConstructorDeclarationContext* HoocParser::ClassMemberContext::constructorDeclaration() {
  return getRuleContext<HoocParser::ConstructorDeclarationContext>(0);
}

HoocParser::FunctionDeclarationContext* HoocParser::ClassMemberContext::functionDeclaration() {
  return getRuleContext<HoocParser::FunctionDeclarationContext>(0);
}

std::vector<HoocParser::FunctionModifierContext *> HoocParser::ClassMemberContext::functionModifier() {
  return getRuleContexts<HoocParser::FunctionModifierContext>();
}

HoocParser::FunctionModifierContext* HoocParser::ClassMemberContext::functionModifier(size_t i) {
  return getRuleContext<HoocParser::FunctionModifierContext>(i);
}


size_t HoocParser::ClassMemberContext::getRuleIndex() const {
  return HoocParser::RuleClassMember;
}

void HoocParser::ClassMemberContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterClassMember(this);
}

void HoocParser::ClassMemberContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitClassMember(this);
}


std::any HoocParser::ClassMemberContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitClassMember(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ClassMemberContext* HoocParser::classMember() {
  ClassMemberContext *_localctx = _tracker.createInstance<ClassMemberContext>(_ctx, getState());
  enterRule(_localctx, 24, HoocParser::RuleClassMember);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(257);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::VAR: {
        enterOuterAlt(_localctx, 1);
        setState(246);
        variableDeclaration();
        setState(247);
        match(HoocParser::SEMICOLON);
        break;
      }

      case HoocParser::CONSTRUCTOR: {
        enterOuterAlt(_localctx, 2);
        setState(249);
        constructorDeclaration();
        break;
      }

      case HoocParser::FUNC:
      case HoocParser::PUBLIC:
      case HoocParser::PRIVATE:
      case HoocParser::ASYNC: {
        enterOuterAlt(_localctx, 3);
        setState(253);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 28) != 0)) {
          setState(250);
          functionModifier();
          setState(255);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(256);
        functionDeclaration();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstructorDeclarationContext ------------------------------------------------------------------

HoocParser::ConstructorDeclarationContext::ConstructorDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ConstructorDeclarationContext::CONSTRUCTOR() {
  return getToken(HoocParser::CONSTRUCTOR, 0);
}

tree::TerminalNode* HoocParser::ConstructorDeclarationContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::ConstructorDeclarationContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

HoocParser::BlockContext* HoocParser::ConstructorDeclarationContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}

HoocParser::ParameterListContext* HoocParser::ConstructorDeclarationContext::parameterList() {
  return getRuleContext<HoocParser::ParameterListContext>(0);
}


size_t HoocParser::ConstructorDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleConstructorDeclaration;
}

void HoocParser::ConstructorDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstructorDeclaration(this);
}

void HoocParser::ConstructorDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstructorDeclaration(this);
}


std::any HoocParser::ConstructorDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitConstructorDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ConstructorDeclarationContext* HoocParser::constructorDeclaration() {
  ConstructorDeclarationContext *_localctx = _tracker.createInstance<ConstructorDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 26, HoocParser::RuleConstructorDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(259);
    match(HoocParser::CONSTRUCTOR);
    setState(260);
    match(HoocParser::LPAREN);
    setState(262);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::IDENTIFIER) {
      setState(261);
      parameterList();
    }
    setState(264);
    match(HoocParser::RPAREN);
    setState(265);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FunctionModifierContext ------------------------------------------------------------------

HoocParser::FunctionModifierContext::FunctionModifierContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FunctionModifierContext::PUBLIC() {
  return getToken(HoocParser::PUBLIC, 0);
}

tree::TerminalNode* HoocParser::FunctionModifierContext::PRIVATE() {
  return getToken(HoocParser::PRIVATE, 0);
}

tree::TerminalNode* HoocParser::FunctionModifierContext::ASYNC() {
  return getToken(HoocParser::ASYNC, 0);
}


size_t HoocParser::FunctionModifierContext::getRuleIndex() const {
  return HoocParser::RuleFunctionModifier;
}

void HoocParser::FunctionModifierContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFunctionModifier(this);
}

void HoocParser::FunctionModifierContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFunctionModifier(this);
}


std::any HoocParser::FunctionModifierContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFunctionModifier(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FunctionModifierContext* HoocParser::functionModifier() {
  FunctionModifierContext *_localctx = _tracker.createInstance<FunctionModifierContext>(_ctx, getState());
  enterRule(_localctx, 28, HoocParser::RuleFunctionModifier);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(267);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 28) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VariableDeclarationContext ------------------------------------------------------------------

HoocParser::VariableDeclarationContext::VariableDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::VariableDeclarationContext::VAR() {
  return getToken(HoocParser::VAR, 0);
}

tree::TerminalNode* HoocParser::VariableDeclarationContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::VariableDeclarationContext::ASSIGN() {
  return getToken(HoocParser::ASSIGN, 0);
}

HoocParser::ExpressionContext* HoocParser::VariableDeclarationContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::VariableDeclarationContext::COLON() {
  return getToken(HoocParser::COLON, 0);
}

HoocParser::TypeContext* HoocParser::VariableDeclarationContext::type() {
  return getRuleContext<HoocParser::TypeContext>(0);
}


size_t HoocParser::VariableDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleVariableDeclaration;
}

void HoocParser::VariableDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVariableDeclaration(this);
}

void HoocParser::VariableDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVariableDeclaration(this);
}


std::any HoocParser::VariableDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitVariableDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::VariableDeclarationContext* HoocParser::variableDeclaration() {
  VariableDeclarationContext *_localctx = _tracker.createInstance<VariableDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 30, HoocParser::RuleVariableDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(281);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 21, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(269);
      match(HoocParser::VAR);
      setState(270);
      match(HoocParser::IDENTIFIER);
      setState(271);
      match(HoocParser::ASSIGN);
      setState(272);
      expression();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(273);
      match(HoocParser::VAR);
      setState(274);
      match(HoocParser::IDENTIFIER);
      setState(275);
      match(HoocParser::COLON);
      setState(276);
      type();
      setState(279);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == HoocParser::ASSIGN) {
        setState(277);
        match(HoocParser::ASSIGN);
        setState(278);
        expression();
      }
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ConstantDeclarationContext ------------------------------------------------------------------

HoocParser::ConstantDeclarationContext::ConstantDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ConstantDeclarationContext::CONST() {
  return getToken(HoocParser::CONST, 0);
}

tree::TerminalNode* HoocParser::ConstantDeclarationContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::ConstantDeclarationContext::ASSIGN() {
  return getToken(HoocParser::ASSIGN, 0);
}

HoocParser::ExpressionContext* HoocParser::ConstantDeclarationContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::ConstantDeclarationContext::COLON() {
  return getToken(HoocParser::COLON, 0);
}

HoocParser::TypeContext* HoocParser::ConstantDeclarationContext::type() {
  return getRuleContext<HoocParser::TypeContext>(0);
}


size_t HoocParser::ConstantDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleConstantDeclaration;
}

void HoocParser::ConstantDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterConstantDeclaration(this);
}

void HoocParser::ConstantDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitConstantDeclaration(this);
}


std::any HoocParser::ConstantDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitConstantDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ConstantDeclarationContext* HoocParser::constantDeclaration() {
  ConstantDeclarationContext *_localctx = _tracker.createInstance<ConstantDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 32, HoocParser::RuleConstantDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(283);
    match(HoocParser::CONST);
    setState(284);
    match(HoocParser::IDENTIFIER);
    setState(287);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::COLON) {
      setState(285);
      match(HoocParser::COLON);
      setState(286);
      type();
    }
    setState(289);
    match(HoocParser::ASSIGN);
    setState(290);
    expression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TypeContext ------------------------------------------------------------------

HoocParser::TypeContext::TypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::OptionalTypeContext* HoocParser::TypeContext::optionalType() {
  return getRuleContext<HoocParser::OptionalTypeContext>(0);
}

HoocParser::MapTypeContext* HoocParser::TypeContext::mapType() {
  return getRuleContext<HoocParser::MapTypeContext>(0);
}


size_t HoocParser::TypeContext::getRuleIndex() const {
  return HoocParser::RuleType;
}

void HoocParser::TypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterType(this);
}

void HoocParser::TypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitType(this);
}


std::any HoocParser::TypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::TypeContext* HoocParser::type() {
  TypeContext *_localctx = _tracker.createInstance<TypeContext>(_ctx, getState());
  enterRule(_localctx, 34, HoocParser::RuleType);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(294);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::INT8:
      case HoocParser::BYTE:
      case HoocParser::INT64:
      case HoocParser::FLOAT:
      case HoocParser::DOUBLE:
      case HoocParser::F64:
      case HoocParser::BOOL:
      case HoocParser::CHAR:
      case HoocParser::STRING:
      case HoocParser::VOID:
      case HoocParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 1);
        setState(292);
        optionalType();
        break;
      }

      case HoocParser::MAP: {
        enterOuterAlt(_localctx, 2);
        setState(293);
        mapType();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- OptionalTypeContext ------------------------------------------------------------------

HoocParser::OptionalTypeContext::OptionalTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::ArrayTypeContext* HoocParser::OptionalTypeContext::arrayType() {
  return getRuleContext<HoocParser::ArrayTypeContext>(0);
}

tree::TerminalNode* HoocParser::OptionalTypeContext::QUESTION() {
  return getToken(HoocParser::QUESTION, 0);
}


size_t HoocParser::OptionalTypeContext::getRuleIndex() const {
  return HoocParser::RuleOptionalType;
}

void HoocParser::OptionalTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterOptionalType(this);
}

void HoocParser::OptionalTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitOptionalType(this);
}


std::any HoocParser::OptionalTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitOptionalType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::OptionalTypeContext* HoocParser::optionalType() {
  OptionalTypeContext *_localctx = _tracker.createInstance<OptionalTypeContext>(_ctx, getState());
  enterRule(_localctx, 36, HoocParser::RuleOptionalType);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(296);
    arrayType();
    setState(298);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::QUESTION) {
      setState(297);
      match(HoocParser::QUESTION);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArrayTypeContext ------------------------------------------------------------------

HoocParser::ArrayTypeContext::ArrayTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::BaseTypeContext* HoocParser::ArrayTypeContext::baseType() {
  return getRuleContext<HoocParser::BaseTypeContext>(0);
}

std::vector<tree::TerminalNode *> HoocParser::ArrayTypeContext::LBRACKET() {
  return getTokens(HoocParser::LBRACKET);
}

tree::TerminalNode* HoocParser::ArrayTypeContext::LBRACKET(size_t i) {
  return getToken(HoocParser::LBRACKET, i);
}

std::vector<tree::TerminalNode *> HoocParser::ArrayTypeContext::RBRACKET() {
  return getTokens(HoocParser::RBRACKET);
}

tree::TerminalNode* HoocParser::ArrayTypeContext::RBRACKET(size_t i) {
  return getToken(HoocParser::RBRACKET, i);
}


size_t HoocParser::ArrayTypeContext::getRuleIndex() const {
  return HoocParser::RuleArrayType;
}

void HoocParser::ArrayTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArrayType(this);
}

void HoocParser::ArrayTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArrayType(this);
}


std::any HoocParser::ArrayTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitArrayType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ArrayTypeContext* HoocParser::arrayType() {
  ArrayTypeContext *_localctx = _tracker.createInstance<ArrayTypeContext>(_ctx, getState());
  enterRule(_localctx, 38, HoocParser::RuleArrayType);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(300);
    baseType();
    setState(305);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::LBRACKET) {
      setState(301);
      match(HoocParser::LBRACKET);
      setState(302);
      match(HoocParser::RBRACKET);
      setState(307);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BaseTypeContext ------------------------------------------------------------------

HoocParser::BaseTypeContext::BaseTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::PrimitiveTypeContext* HoocParser::BaseTypeContext::primitiveType() {
  return getRuleContext<HoocParser::PrimitiveTypeContext>(0);
}

HoocParser::QualifiedIdentifierContext* HoocParser::BaseTypeContext::qualifiedIdentifier() {
  return getRuleContext<HoocParser::QualifiedIdentifierContext>(0);
}


size_t HoocParser::BaseTypeContext::getRuleIndex() const {
  return HoocParser::RuleBaseType;
}

void HoocParser::BaseTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBaseType(this);
}

void HoocParser::BaseTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBaseType(this);
}


std::any HoocParser::BaseTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitBaseType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::BaseTypeContext* HoocParser::baseType() {
  BaseTypeContext *_localctx = _tracker.createInstance<BaseTypeContext>(_ctx, getState());
  enterRule(_localctx, 40, HoocParser::RuleBaseType);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(310);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::INT8:
      case HoocParser::BYTE:
      case HoocParser::INT64:
      case HoocParser::FLOAT:
      case HoocParser::DOUBLE:
      case HoocParser::F64:
      case HoocParser::BOOL:
      case HoocParser::CHAR:
      case HoocParser::STRING:
      case HoocParser::VOID: {
        enterOuterAlt(_localctx, 1);
        setState(308);
        primitiveType();
        break;
      }

      case HoocParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 2);
        setState(309);
        qualifiedIdentifier();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MapTypeContext ------------------------------------------------------------------

HoocParser::MapTypeContext::MapTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::MapTypeContext::MAP() {
  return getToken(HoocParser::MAP, 0);
}

tree::TerminalNode* HoocParser::MapTypeContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

HoocParser::MapKeyTypeContext* HoocParser::MapTypeContext::mapKeyType() {
  return getRuleContext<HoocParser::MapKeyTypeContext>(0);
}

tree::TerminalNode* HoocParser::MapTypeContext::COMMA() {
  return getToken(HoocParser::COMMA, 0);
}

HoocParser::TypeContext* HoocParser::MapTypeContext::type() {
  return getRuleContext<HoocParser::TypeContext>(0);
}

tree::TerminalNode* HoocParser::MapTypeContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}


size_t HoocParser::MapTypeContext::getRuleIndex() const {
  return HoocParser::RuleMapType;
}

void HoocParser::MapTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMapType(this);
}

void HoocParser::MapTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMapType(this);
}


std::any HoocParser::MapTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitMapType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::MapTypeContext* HoocParser::mapType() {
  MapTypeContext *_localctx = _tracker.createInstance<MapTypeContext>(_ctx, getState());
  enterRule(_localctx, 42, HoocParser::RuleMapType);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(312);
    match(HoocParser::MAP);
    setState(313);
    match(HoocParser::LBRACKET);
    setState(314);
    mapKeyType();
    setState(315);
    match(HoocParser::COMMA);
    setState(316);
    type();
    setState(317);
    match(HoocParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MapKeyTypeContext ------------------------------------------------------------------

HoocParser::MapKeyTypeContext::MapKeyTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::MapKeyTypeContext::BYTE() {
  return getToken(HoocParser::BYTE, 0);
}

tree::TerminalNode* HoocParser::MapKeyTypeContext::INT8() {
  return getToken(HoocParser::INT8, 0);
}

tree::TerminalNode* HoocParser::MapKeyTypeContext::INT64() {
  return getToken(HoocParser::INT64, 0);
}

tree::TerminalNode* HoocParser::MapKeyTypeContext::CHAR() {
  return getToken(HoocParser::CHAR, 0);
}

tree::TerminalNode* HoocParser::MapKeyTypeContext::STRING() {
  return getToken(HoocParser::STRING, 0);
}


size_t HoocParser::MapKeyTypeContext::getRuleIndex() const {
  return HoocParser::RuleMapKeyType;
}

void HoocParser::MapKeyTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMapKeyType(this);
}

void HoocParser::MapKeyTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMapKeyType(this);
}


std::any HoocParser::MapKeyTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitMapKeyType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::MapKeyTypeContext* HoocParser::mapKeyType() {
  MapKeyTypeContext *_localctx = _tracker.createInstance<MapKeyTypeContext>(_ctx, getState());
  enterRule(_localctx, 44, HoocParser::RuleMapKeyType);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(319);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 1760907454301863936) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PrimitiveTypeContext ------------------------------------------------------------------

HoocParser::PrimitiveTypeContext::PrimitiveTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::INT8() {
  return getToken(HoocParser::INT8, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::BYTE() {
  return getToken(HoocParser::BYTE, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::INT64() {
  return getToken(HoocParser::INT64, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::FLOAT() {
  return getToken(HoocParser::FLOAT, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::DOUBLE() {
  return getToken(HoocParser::DOUBLE, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::F64() {
  return getToken(HoocParser::F64, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::BOOL() {
  return getToken(HoocParser::BOOL, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::CHAR() {
  return getToken(HoocParser::CHAR, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::STRING() {
  return getToken(HoocParser::STRING, 0);
}

tree::TerminalNode* HoocParser::PrimitiveTypeContext::VOID() {
  return getToken(HoocParser::VOID, 0);
}


size_t HoocParser::PrimitiveTypeContext::getRuleIndex() const {
  return HoocParser::RulePrimitiveType;
}

void HoocParser::PrimitiveTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimitiveType(this);
}

void HoocParser::PrimitiveTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimitiveType(this);
}


std::any HoocParser::PrimitiveTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitPrimitiveType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::PrimitiveTypeContext* HoocParser::primitiveType() {
  PrimitiveTypeContext *_localctx = _tracker.createInstance<PrimitiveTypeContext>(_ctx, getState());
  enterRule(_localctx, 46, HoocParser::RulePrimitiveType);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(321);
    _la = _input->LA(1);
    if (!((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & 4607182418800017408) != 0))) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiDeclarationContext ------------------------------------------------------------------

HoocParser::FfiDeclarationContext::FfiDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::FfiImportDeclarationContext* HoocParser::FfiDeclarationContext::ffiImportDeclaration() {
  return getRuleContext<HoocParser::FfiImportDeclarationContext>(0);
}

HoocParser::FfiLinkDeclarationContext* HoocParser::FfiDeclarationContext::ffiLinkDeclaration() {
  return getRuleContext<HoocParser::FfiLinkDeclarationContext>(0);
}

HoocParser::FfiNativeFunctionContext* HoocParser::FfiDeclarationContext::ffiNativeFunction() {
  return getRuleContext<HoocParser::FfiNativeFunctionContext>(0);
}

HoocParser::FfiNativeDeclarationContext* HoocParser::FfiDeclarationContext::ffiNativeDeclaration() {
  return getRuleContext<HoocParser::FfiNativeDeclarationContext>(0);
}


size_t HoocParser::FfiDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleFfiDeclaration;
}

void HoocParser::FfiDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiDeclaration(this);
}

void HoocParser::FfiDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiDeclaration(this);
}


std::any HoocParser::FfiDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiDeclarationContext* HoocParser::ffiDeclaration() {
  FfiDeclarationContext *_localctx = _tracker.createInstance<FfiDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 48, HoocParser::RuleFfiDeclaration);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(327);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 27, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(323);
      ffiImportDeclaration();
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(324);
      ffiLinkDeclaration();
      break;
    }

    case 3: {
      enterOuterAlt(_localctx, 3);
      setState(325);
      ffiNativeFunction();
      break;
    }

    case 4: {
      enterOuterAlt(_localctx, 4);
      setState(326);
      ffiNativeDeclaration();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiImportDeclarationContext ------------------------------------------------------------------

HoocParser::FfiImportDeclarationContext::FfiImportDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FfiImportDeclarationContext::LIBRARY() {
  return getToken(HoocParser::LIBRARY, 0);
}

tree::TerminalNode* HoocParser::FfiImportDeclarationContext::STRING_LITERAL() {
  return getToken(HoocParser::STRING_LITERAL, 0);
}

tree::TerminalNode* HoocParser::FfiImportDeclarationContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

tree::TerminalNode* HoocParser::FfiImportDeclarationContext::AS() {
  return getToken(HoocParser::AS, 0);
}

tree::TerminalNode* HoocParser::FfiImportDeclarationContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}


size_t HoocParser::FfiImportDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleFfiImportDeclaration;
}

void HoocParser::FfiImportDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiImportDeclaration(this);
}

void HoocParser::FfiImportDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiImportDeclaration(this);
}


std::any HoocParser::FfiImportDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiImportDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiImportDeclarationContext* HoocParser::ffiImportDeclaration() {
  FfiImportDeclarationContext *_localctx = _tracker.createInstance<FfiImportDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 50, HoocParser::RuleFfiImportDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(329);
    match(HoocParser::LIBRARY);
    setState(330);
    match(HoocParser::STRING_LITERAL);
    setState(333);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::AS) {
      setState(331);
      match(HoocParser::AS);
      setState(332);
      match(HoocParser::IDENTIFIER);
    }
    setState(335);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiLinkDeclarationContext ------------------------------------------------------------------

HoocParser::FfiLinkDeclarationContext::FfiLinkDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FfiLinkDeclarationContext::LINK() {
  return getToken(HoocParser::LINK, 0);
}

tree::TerminalNode* HoocParser::FfiLinkDeclarationContext::DYNAMIC() {
  return getToken(HoocParser::DYNAMIC, 0);
}

HoocParser::ModulePathContext* HoocParser::FfiLinkDeclarationContext::modulePath() {
  return getRuleContext<HoocParser::ModulePathContext>(0);
}

tree::TerminalNode* HoocParser::FfiLinkDeclarationContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

tree::TerminalNode* HoocParser::FfiLinkDeclarationContext::AT() {
  return getToken(HoocParser::AT, 0);
}

HoocParser::VersionRangeContext* HoocParser::FfiLinkDeclarationContext::versionRange() {
  return getRuleContext<HoocParser::VersionRangeContext>(0);
}

HoocParser::LibrarySearchPathsContext* HoocParser::FfiLinkDeclarationContext::librarySearchPaths() {
  return getRuleContext<HoocParser::LibrarySearchPathsContext>(0);
}


size_t HoocParser::FfiLinkDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleFfiLinkDeclaration;
}

void HoocParser::FfiLinkDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiLinkDeclaration(this);
}

void HoocParser::FfiLinkDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiLinkDeclaration(this);
}


std::any HoocParser::FfiLinkDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiLinkDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiLinkDeclarationContext* HoocParser::ffiLinkDeclaration() {
  FfiLinkDeclarationContext *_localctx = _tracker.createInstance<FfiLinkDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 52, HoocParser::RuleFfiLinkDeclaration);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(337);
    match(HoocParser::LINK);
    setState(338);
    match(HoocParser::DYNAMIC);
    setState(339);
    modulePath();
    setState(342);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::AT) {
      setState(340);
      match(HoocParser::AT);
      setState(341);
      versionRange();
    }
    setState(345);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::LBRACKET) {
      setState(344);
      librarySearchPaths();
    }
    setState(347);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiNativeFunctionContext ------------------------------------------------------------------

HoocParser::FfiNativeFunctionContext::FfiNativeFunctionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::NATIVE() {
  return getToken(HoocParser::NATIVE, 0);
}

HoocParser::FunctionDeclarationContext* HoocParser::FfiNativeFunctionContext::functionDeclaration() {
  return getRuleContext<HoocParser::FunctionDeclarationContext>(0);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::EXTERN() {
  return getToken(HoocParser::EXTERN, 0);
}

std::vector<HoocParser::TypeContext *> HoocParser::FfiNativeFunctionContext::type() {
  return getRuleContexts<HoocParser::TypeContext>();
}

HoocParser::TypeContext* HoocParser::FfiNativeFunctionContext::type(size_t i) {
  return getRuleContext<HoocParser::TypeContext>(i);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

std::vector<HoocParser::FunctionModifierContext *> HoocParser::FfiNativeFunctionContext::functionModifier() {
  return getRuleContexts<HoocParser::FunctionModifierContext>();
}

HoocParser::FunctionModifierContext* HoocParser::FfiNativeFunctionContext::functionModifier(size_t i) {
  return getRuleContext<HoocParser::FunctionModifierContext>(i);
}

HoocParser::FfiParameterListContext* HoocParser::FfiNativeFunctionContext::ffiParameterList() {
  return getRuleContext<HoocParser::FfiParameterListContext>(0);
}

tree::TerminalNode* HoocParser::FfiNativeFunctionContext::ARROW() {
  return getToken(HoocParser::ARROW, 0);
}


size_t HoocParser::FfiNativeFunctionContext::getRuleIndex() const {
  return HoocParser::RuleFfiNativeFunction;
}

void HoocParser::FfiNativeFunctionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiNativeFunction(this);
}

void HoocParser::FfiNativeFunctionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiNativeFunction(this);
}


std::any HoocParser::FfiNativeFunctionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiNativeFunction(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiNativeFunctionContext* HoocParser::ffiNativeFunction() {
  FfiNativeFunctionContext *_localctx = _tracker.createInstance<FfiNativeFunctionContext>(_ctx, getState());
  enterRule(_localctx, 54, HoocParser::RuleFfiNativeFunction);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(372);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::NATIVE: {
        enterOuterAlt(_localctx, 1);
        setState(349);
        match(HoocParser::NATIVE);
        setState(350);
        functionDeclaration();
        break;
      }

      case HoocParser::EXTERN: {
        enterOuterAlt(_localctx, 2);
        setState(351);
        match(HoocParser::EXTERN);
        setState(355);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & 28) != 0)) {
          setState(352);
          functionModifier();
          setState(357);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(358);
        match(HoocParser::NATIVE);
        setState(359);
        type();
        setState(360);
        match(HoocParser::IDENTIFIER);
        setState(361);
        match(HoocParser::LPAREN);
        setState(363);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == HoocParser::IDENTIFIER) {
          setState(362);
          ffiParameterList();
        }
        setState(365);
        match(HoocParser::RPAREN);
        setState(368);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == HoocParser::ARROW) {
          setState(366);
          match(HoocParser::ARROW);
          setState(367);
          type();
        }
        setState(370);
        match(HoocParser::SEMICOLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiNativeDeclarationContext ------------------------------------------------------------------

HoocParser::FfiNativeDeclarationContext::FfiNativeDeclarationContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FfiNativeDeclarationContext::NATIVE() {
  return getToken(HoocParser::NATIVE, 0);
}

HoocParser::VariableDeclarationContext* HoocParser::FfiNativeDeclarationContext::variableDeclaration() {
  return getRuleContext<HoocParser::VariableDeclarationContext>(0);
}

tree::TerminalNode* HoocParser::FfiNativeDeclarationContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

tree::TerminalNode* HoocParser::FfiNativeDeclarationContext::EXTERN() {
  return getToken(HoocParser::EXTERN, 0);
}


size_t HoocParser::FfiNativeDeclarationContext::getRuleIndex() const {
  return HoocParser::RuleFfiNativeDeclaration;
}

void HoocParser::FfiNativeDeclarationContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiNativeDeclaration(this);
}

void HoocParser::FfiNativeDeclarationContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiNativeDeclaration(this);
}


std::any HoocParser::FfiNativeDeclarationContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiNativeDeclaration(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiNativeDeclarationContext* HoocParser::ffiNativeDeclaration() {
  FfiNativeDeclarationContext *_localctx = _tracker.createInstance<FfiNativeDeclarationContext>(_ctx, getState());
  enterRule(_localctx, 56, HoocParser::RuleFfiNativeDeclaration);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(383);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::NATIVE: {
        enterOuterAlt(_localctx, 1);
        setState(374);
        match(HoocParser::NATIVE);
        setState(375);
        variableDeclaration();
        setState(376);
        match(HoocParser::SEMICOLON);
        break;
      }

      case HoocParser::EXTERN: {
        enterOuterAlt(_localctx, 2);
        setState(378);
        match(HoocParser::EXTERN);
        setState(379);
        match(HoocParser::NATIVE);
        setState(380);
        variableDeclaration();
        setState(381);
        match(HoocParser::SEMICOLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiParameterListContext ------------------------------------------------------------------

HoocParser::FfiParameterListContext::FfiParameterListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::FfiParameterContext *> HoocParser::FfiParameterListContext::ffiParameter() {
  return getRuleContexts<HoocParser::FfiParameterContext>();
}

HoocParser::FfiParameterContext* HoocParser::FfiParameterListContext::ffiParameter(size_t i) {
  return getRuleContext<HoocParser::FfiParameterContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::FfiParameterListContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::FfiParameterListContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}


size_t HoocParser::FfiParameterListContext::getRuleIndex() const {
  return HoocParser::RuleFfiParameterList;
}

void HoocParser::FfiParameterListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiParameterList(this);
}

void HoocParser::FfiParameterListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiParameterList(this);
}


std::any HoocParser::FfiParameterListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiParameterList(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiParameterListContext* HoocParser::ffiParameterList() {
  FfiParameterListContext *_localctx = _tracker.createInstance<FfiParameterListContext>(_ctx, getState());
  enterRule(_localctx, 58, HoocParser::RuleFfiParameterList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(385);
    ffiParameter();
    setState(390);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::COMMA) {
      setState(386);
      match(HoocParser::COMMA);
      setState(387);
      ffiParameter();
      setState(392);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiParameterContext ------------------------------------------------------------------

HoocParser::FfiParameterContext::FfiParameterContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::FfiParameterContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::FfiParameterContext::COLON() {
  return getToken(HoocParser::COLON, 0);
}

HoocParser::FfiTypeContext* HoocParser::FfiParameterContext::ffiType() {
  return getRuleContext<HoocParser::FfiTypeContext>(0);
}


size_t HoocParser::FfiParameterContext::getRuleIndex() const {
  return HoocParser::RuleFfiParameter;
}

void HoocParser::FfiParameterContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiParameter(this);
}

void HoocParser::FfiParameterContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiParameter(this);
}


std::any HoocParser::FfiParameterContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiParameter(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiParameterContext* HoocParser::ffiParameter() {
  FfiParameterContext *_localctx = _tracker.createInstance<FfiParameterContext>(_ctx, getState());
  enterRule(_localctx, 60, HoocParser::RuleFfiParameter);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(393);
    match(HoocParser::IDENTIFIER);
    setState(394);
    match(HoocParser::COLON);
    setState(395);
    ffiType();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- FfiTypeContext ------------------------------------------------------------------

HoocParser::FfiTypeContext::FfiTypeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::PrimitiveTypeContext* HoocParser::FfiTypeContext::primitiveType() {
  return getRuleContext<HoocParser::PrimitiveTypeContext>(0);
}

HoocParser::QualifiedIdentifierContext* HoocParser::FfiTypeContext::qualifiedIdentifier() {
  return getRuleContext<HoocParser::QualifiedIdentifierContext>(0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::POINTER() {
  return getToken(HoocParser::POINTER, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

std::vector<HoocParser::FfiTypeContext *> HoocParser::FfiTypeContext::ffiType() {
  return getRuleContexts<HoocParser::FfiTypeContext>();
}

HoocParser::FfiTypeContext* HoocParser::FfiTypeContext::ffiType(size_t i) {
  return getRuleContext<HoocParser::FfiTypeContext>(i);
}

tree::TerminalNode* HoocParser::FfiTypeContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::ARRAY() {
  return getToken(HoocParser::ARRAY, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::INTEGER_LITERAL() {
  return getToken(HoocParser::INTEGER_LITERAL, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::FUNCTION() {
  return getToken(HoocParser::FUNCTION, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::FfiTypeContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

std::vector<tree::TerminalNode *> HoocParser::FfiTypeContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::FfiTypeContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}

tree::TerminalNode* HoocParser::FfiTypeContext::ARROW() {
  return getToken(HoocParser::ARROW, 0);
}


size_t HoocParser::FfiTypeContext::getRuleIndex() const {
  return HoocParser::RuleFfiType;
}

void HoocParser::FfiTypeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterFfiType(this);
}

void HoocParser::FfiTypeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitFfiType(this);
}


std::any HoocParser::FfiTypeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitFfiType(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::FfiTypeContext* HoocParser::ffiType() {
  FfiTypeContext *_localctx = _tracker.createInstance<FfiTypeContext>(_ctx, getState());
  enterRule(_localctx, 62, HoocParser::RuleFfiType);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(424);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::INT8:
      case HoocParser::BYTE:
      case HoocParser::INT64:
      case HoocParser::FLOAT:
      case HoocParser::DOUBLE:
      case HoocParser::F64:
      case HoocParser::BOOL:
      case HoocParser::CHAR:
      case HoocParser::STRING:
      case HoocParser::VOID: {
        enterOuterAlt(_localctx, 1);
        setState(397);
        primitiveType();
        break;
      }

      case HoocParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 2);
        setState(398);
        qualifiedIdentifier();
        break;
      }

      case HoocParser::POINTER: {
        enterOuterAlt(_localctx, 3);
        setState(399);
        match(HoocParser::POINTER);
        setState(400);
        match(HoocParser::LBRACKET);
        setState(401);
        ffiType();
        setState(402);
        match(HoocParser::RBRACKET);
        break;
      }

      case HoocParser::ARRAY: {
        enterOuterAlt(_localctx, 4);
        setState(404);
        match(HoocParser::ARRAY);
        setState(405);
        match(HoocParser::LBRACKET);
        setState(406);
        match(HoocParser::INTEGER_LITERAL);
        setState(407);
        match(HoocParser::RBRACKET);
        setState(408);
        ffiType();
        break;
      }

      case HoocParser::FUNCTION: {
        enterOuterAlt(_localctx, 5);
        setState(409);
        match(HoocParser::FUNCTION);
        setState(410);
        match(HoocParser::LPAREN);
        setState(411);
        ffiType();
        setState(416);
        _errHandler->sync(this);
        _la = _input->LA(1);
        while (_la == HoocParser::COMMA) {
          setState(412);
          match(HoocParser::COMMA);
          setState(413);
          ffiType();
          setState(418);
          _errHandler->sync(this);
          _la = _input->LA(1);
        }
        setState(419);
        match(HoocParser::RPAREN);
        setState(422);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if (_la == HoocParser::ARROW) {
          setState(420);
          match(HoocParser::ARROW);
          setState(421);
          ffiType();
        }
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LibrarySearchPathsContext ------------------------------------------------------------------

HoocParser::LibrarySearchPathsContext::LibrarySearchPathsContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::LibrarySearchPathsContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

std::vector<tree::TerminalNode *> HoocParser::LibrarySearchPathsContext::STRING_LITERAL() {
  return getTokens(HoocParser::STRING_LITERAL);
}

tree::TerminalNode* HoocParser::LibrarySearchPathsContext::STRING_LITERAL(size_t i) {
  return getToken(HoocParser::STRING_LITERAL, i);
}

tree::TerminalNode* HoocParser::LibrarySearchPathsContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}

std::vector<tree::TerminalNode *> HoocParser::LibrarySearchPathsContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::LibrarySearchPathsContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}


size_t HoocParser::LibrarySearchPathsContext::getRuleIndex() const {
  return HoocParser::RuleLibrarySearchPaths;
}

void HoocParser::LibrarySearchPathsContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLibrarySearchPaths(this);
}

void HoocParser::LibrarySearchPathsContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLibrarySearchPaths(this);
}


std::any HoocParser::LibrarySearchPathsContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitLibrarySearchPaths(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::LibrarySearchPathsContext* HoocParser::librarySearchPaths() {
  LibrarySearchPathsContext *_localctx = _tracker.createInstance<LibrarySearchPathsContext>(_ctx, getState());
  enterRule(_localctx, 64, HoocParser::RuleLibrarySearchPaths);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(426);
    match(HoocParser::LBRACKET);
    setState(427);
    match(HoocParser::STRING_LITERAL);
    setState(432);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::COMMA) {
      setState(428);
      match(HoocParser::COMMA);
      setState(429);
      match(HoocParser::STRING_LITERAL);
      setState(434);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(435);
    match(HoocParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VersionRangeContext ------------------------------------------------------------------

HoocParser::VersionRangeContext::VersionRangeContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::VersionRangeContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

std::vector<tree::TerminalNode *> HoocParser::VersionRangeContext::DOT() {
  return getTokens(HoocParser::DOT);
}

tree::TerminalNode* HoocParser::VersionRangeContext::DOT(size_t i) {
  return getToken(HoocParser::DOT, i);
}

tree::TerminalNode* HoocParser::VersionRangeContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}

std::vector<tree::TerminalNode *> HoocParser::VersionRangeContext::INTEGER_LITERAL() {
  return getTokens(HoocParser::INTEGER_LITERAL);
}

tree::TerminalNode* HoocParser::VersionRangeContext::INTEGER_LITERAL(size_t i) {
  return getToken(HoocParser::INTEGER_LITERAL, i);
}


size_t HoocParser::VersionRangeContext::getRuleIndex() const {
  return HoocParser::RuleVersionRange;
}

void HoocParser::VersionRangeContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVersionRange(this);
}

void HoocParser::VersionRangeContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVersionRange(this);
}


std::any HoocParser::VersionRangeContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitVersionRange(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::VersionRangeContext* HoocParser::versionRange() {
  VersionRangeContext *_localctx = _tracker.createInstance<VersionRangeContext>(_ctx, getState());
  enterRule(_localctx, 66, HoocParser::RuleVersionRange);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(437);
    match(HoocParser::LBRACKET);
    setState(439);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::INTEGER_LITERAL) {
      setState(438);
      match(HoocParser::INTEGER_LITERAL);
    }
    setState(441);
    match(HoocParser::DOT);
    setState(442);
    match(HoocParser::DOT);
    setState(444);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::INTEGER_LITERAL) {
      setState(443);
      match(HoocParser::INTEGER_LITERAL);
    }
    setState(446);
    match(HoocParser::RBRACKET);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- StatementContext ------------------------------------------------------------------

HoocParser::StatementContext::StatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::BlockContext* HoocParser::StatementContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}

HoocParser::VariableDeclarationStatementContext* HoocParser::StatementContext::variableDeclarationStatement() {
  return getRuleContext<HoocParser::VariableDeclarationStatementContext>(0);
}

HoocParser::ExpressionStatementContext* HoocParser::StatementContext::expressionStatement() {
  return getRuleContext<HoocParser::ExpressionStatementContext>(0);
}

HoocParser::ReturnStatementContext* HoocParser::StatementContext::returnStatement() {
  return getRuleContext<HoocParser::ReturnStatementContext>(0);
}

HoocParser::IfStatementContext* HoocParser::StatementContext::ifStatement() {
  return getRuleContext<HoocParser::IfStatementContext>(0);
}

HoocParser::WhileStatementContext* HoocParser::StatementContext::whileStatement() {
  return getRuleContext<HoocParser::WhileStatementContext>(0);
}

HoocParser::ForStatementContext* HoocParser::StatementContext::forStatement() {
  return getRuleContext<HoocParser::ForStatementContext>(0);
}

HoocParser::ScopeStatementContext* HoocParser::StatementContext::scopeStatement() {
  return getRuleContext<HoocParser::ScopeStatementContext>(0);
}

HoocParser::BreakStatementContext* HoocParser::StatementContext::breakStatement() {
  return getRuleContext<HoocParser::BreakStatementContext>(0);
}

HoocParser::ContinueStatementContext* HoocParser::StatementContext::continueStatement() {
  return getRuleContext<HoocParser::ContinueStatementContext>(0);
}

HoocParser::TryCatchStatementContext* HoocParser::StatementContext::tryCatchStatement() {
  return getRuleContext<HoocParser::TryCatchStatementContext>(0);
}

HoocParser::ThrowStatementContext* HoocParser::StatementContext::throwStatement() {
  return getRuleContext<HoocParser::ThrowStatementContext>(0);
}


size_t HoocParser::StatementContext::getRuleIndex() const {
  return HoocParser::RuleStatement;
}

void HoocParser::StatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterStatement(this);
}

void HoocParser::StatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitStatement(this);
}


std::any HoocParser::StatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::StatementContext* HoocParser::statement() {
  StatementContext *_localctx = _tracker.createInstance<StatementContext>(_ctx, getState());
  enterRule(_localctx, 68, HoocParser::RuleStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(460);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::LBRACE: {
        enterOuterAlt(_localctx, 1);
        setState(448);
        block();
        break;
      }

      case HoocParser::VAR: {
        enterOuterAlt(_localctx, 2);
        setState(449);
        variableDeclarationStatement();
        break;
      }

      case HoocParser::NEW:
      case HoocParser::THIS:
      case HoocParser::TRUE:
      case HoocParser::FALSE:
      case HoocParser::NULL_:
      case HoocParser::MINUS:
      case HoocParser::NOT:
      case HoocParser::LPAREN:
      case HoocParser::LBRACKET:
      case HoocParser::MULTILINE_STRING:
      case HoocParser::STRING_LITERAL:
      case HoocParser::CHAR_LITERAL:
      case HoocParser::INTEGER_LITERAL:
      case HoocParser::FLOATING_LITERAL:
      case HoocParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 3);
        setState(450);
        expressionStatement();
        break;
      }

      case HoocParser::RETURN: {
        enterOuterAlt(_localctx, 4);
        setState(451);
        returnStatement();
        break;
      }

      case HoocParser::IF: {
        enterOuterAlt(_localctx, 5);
        setState(452);
        ifStatement();
        break;
      }

      case HoocParser::WHILE: {
        enterOuterAlt(_localctx, 6);
        setState(453);
        whileStatement();
        break;
      }

      case HoocParser::FOR: {
        enterOuterAlt(_localctx, 7);
        setState(454);
        forStatement();
        break;
      }

      case HoocParser::SCOPE: {
        enterOuterAlt(_localctx, 8);
        setState(455);
        scopeStatement();
        break;
      }

      case HoocParser::BREAK: {
        enterOuterAlt(_localctx, 9);
        setState(456);
        breakStatement();
        break;
      }

      case HoocParser::CONTINUE: {
        enterOuterAlt(_localctx, 10);
        setState(457);
        continueStatement();
        break;
      }

      case HoocParser::TRY: {
        enterOuterAlt(_localctx, 11);
        setState(458);
        tryCatchStatement();
        break;
      }

      case HoocParser::THROW:
      case HoocParser::RETHROW: {
        enterOuterAlt(_localctx, 12);
        setState(459);
        throwStatement();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- TryCatchStatementContext ------------------------------------------------------------------

HoocParser::TryCatchStatementContext::TryCatchStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::TRY() {
  return getToken(HoocParser::TRY, 0);
}

std::vector<HoocParser::BlockContext *> HoocParser::TryCatchStatementContext::block() {
  return getRuleContexts<HoocParser::BlockContext>();
}

HoocParser::BlockContext* HoocParser::TryCatchStatementContext::block(size_t i) {
  return getRuleContext<HoocParser::BlockContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::TryCatchStatementContext::CATCH() {
  return getTokens(HoocParser::CATCH);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::CATCH(size_t i) {
  return getToken(HoocParser::CATCH, i);
}

std::vector<tree::TerminalNode *> HoocParser::TryCatchStatementContext::LPAREN() {
  return getTokens(HoocParser::LPAREN);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::LPAREN(size_t i) {
  return getToken(HoocParser::LPAREN, i);
}

std::vector<tree::TerminalNode *> HoocParser::TryCatchStatementContext::IDENTIFIER() {
  return getTokens(HoocParser::IDENTIFIER);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::IDENTIFIER(size_t i) {
  return getToken(HoocParser::IDENTIFIER, i);
}

std::vector<tree::TerminalNode *> HoocParser::TryCatchStatementContext::COLON() {
  return getTokens(HoocParser::COLON);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::COLON(size_t i) {
  return getToken(HoocParser::COLON, i);
}

std::vector<HoocParser::TypeContext *> HoocParser::TryCatchStatementContext::type() {
  return getRuleContexts<HoocParser::TypeContext>();
}

HoocParser::TypeContext* HoocParser::TryCatchStatementContext::type(size_t i) {
  return getRuleContext<HoocParser::TypeContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::TryCatchStatementContext::RPAREN() {
  return getTokens(HoocParser::RPAREN);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::RPAREN(size_t i) {
  return getToken(HoocParser::RPAREN, i);
}

tree::TerminalNode* HoocParser::TryCatchStatementContext::FINALLY() {
  return getToken(HoocParser::FINALLY, 0);
}


size_t HoocParser::TryCatchStatementContext::getRuleIndex() const {
  return HoocParser::RuleTryCatchStatement;
}

void HoocParser::TryCatchStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterTryCatchStatement(this);
}

void HoocParser::TryCatchStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitTryCatchStatement(this);
}


std::any HoocParser::TryCatchStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitTryCatchStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::TryCatchStatementContext* HoocParser::tryCatchStatement() {
  TryCatchStatementContext *_localctx = _tracker.createInstance<TryCatchStatementContext>(_ctx, getState());
  enterRule(_localctx, 70, HoocParser::RuleTryCatchStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(486);
    _errHandler->sync(this);
    switch (getInterpreter<atn::ParserATNSimulator>()->adaptivePredict(_input, 46, _ctx)) {
    case 1: {
      enterOuterAlt(_localctx, 1);
      setState(462);
      match(HoocParser::TRY);
      setState(463);
      block();
      setState(474);
      _errHandler->sync(this);
      _la = _input->LA(1);
      while (_la == HoocParser::CATCH) {
        setState(464);
        match(HoocParser::CATCH);
        setState(465);
        match(HoocParser::LPAREN);
        setState(466);
        match(HoocParser::IDENTIFIER);
        setState(467);
        match(HoocParser::COLON);
        setState(468);
        type();
        setState(469);
        match(HoocParser::RPAREN);
        setState(470);
        block();
        setState(476);
        _errHandler->sync(this);
        _la = _input->LA(1);
      }
      setState(479);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == HoocParser::FINALLY) {
        setState(477);
        match(HoocParser::FINALLY);
        setState(478);
        block();
      }
      break;
    }

    case 2: {
      enterOuterAlt(_localctx, 2);
      setState(481);
      match(HoocParser::TRY);
      setState(482);
      block();
      setState(483);
      match(HoocParser::FINALLY);
      setState(484);
      block();
      break;
    }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ThrowStatementContext ------------------------------------------------------------------

HoocParser::ThrowStatementContext::ThrowStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ThrowStatementContext::THROW() {
  return getToken(HoocParser::THROW, 0);
}

HoocParser::ExpressionContext* HoocParser::ThrowStatementContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::ThrowStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

tree::TerminalNode* HoocParser::ThrowStatementContext::RETHROW() {
  return getToken(HoocParser::RETHROW, 0);
}


size_t HoocParser::ThrowStatementContext::getRuleIndex() const {
  return HoocParser::RuleThrowStatement;
}

void HoocParser::ThrowStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterThrowStatement(this);
}

void HoocParser::ThrowStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitThrowStatement(this);
}


std::any HoocParser::ThrowStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitThrowStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ThrowStatementContext* HoocParser::throwStatement() {
  ThrowStatementContext *_localctx = _tracker.createInstance<ThrowStatementContext>(_ctx, getState());
  enterRule(_localctx, 72, HoocParser::RuleThrowStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(494);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::THROW: {
        enterOuterAlt(_localctx, 1);
        setState(488);
        match(HoocParser::THROW);
        setState(489);
        expression();
        setState(490);
        match(HoocParser::SEMICOLON);
        break;
      }

      case HoocParser::RETHROW: {
        enterOuterAlt(_localctx, 2);
        setState(492);
        match(HoocParser::RETHROW);
        setState(493);
        match(HoocParser::SEMICOLON);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BlockContext ------------------------------------------------------------------

HoocParser::BlockContext::BlockContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::BlockContext::LBRACE() {
  return getToken(HoocParser::LBRACE, 0);
}

tree::TerminalNode* HoocParser::BlockContext::RBRACE() {
  return getToken(HoocParser::RBRACE, 0);
}

std::vector<HoocParser::StatementContext *> HoocParser::BlockContext::statement() {
  return getRuleContexts<HoocParser::StatementContext>();
}

HoocParser::StatementContext* HoocParser::BlockContext::statement(size_t i) {
  return getRuleContext<HoocParser::StatementContext>(i);
}


size_t HoocParser::BlockContext::getRuleIndex() const {
  return HoocParser::RuleBlock;
}

void HoocParser::BlockContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBlock(this);
}

void HoocParser::BlockContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBlock(this);
}


std::any HoocParser::BlockContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitBlock(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::BlockContext* HoocParser::block() {
  BlockContext *_localctx = _tracker.createInstance<BlockContext>(_ctx, getState());
  enterRule(_localctx, 74, HoocParser::RuleBlock);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(496);
    match(HoocParser::LBRACE);
    setState(500);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & -9223368472030471328) != 0) || ((((_la - 85) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 85)) & 1037569) != 0)) {
      setState(497);
      statement();
      setState(502);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
    setState(503);
    match(HoocParser::RBRACE);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- VariableDeclarationStatementContext ------------------------------------------------------------------

HoocParser::VariableDeclarationStatementContext::VariableDeclarationStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::VariableDeclarationContext* HoocParser::VariableDeclarationStatementContext::variableDeclaration() {
  return getRuleContext<HoocParser::VariableDeclarationContext>(0);
}

tree::TerminalNode* HoocParser::VariableDeclarationStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}


size_t HoocParser::VariableDeclarationStatementContext::getRuleIndex() const {
  return HoocParser::RuleVariableDeclarationStatement;
}

void HoocParser::VariableDeclarationStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterVariableDeclarationStatement(this);
}

void HoocParser::VariableDeclarationStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitVariableDeclarationStatement(this);
}


std::any HoocParser::VariableDeclarationStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitVariableDeclarationStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::VariableDeclarationStatementContext* HoocParser::variableDeclarationStatement() {
  VariableDeclarationStatementContext *_localctx = _tracker.createInstance<VariableDeclarationStatementContext>(_ctx, getState());
  enterRule(_localctx, 76, HoocParser::RuleVariableDeclarationStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(505);
    variableDeclaration();
    setState(506);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionStatementContext ------------------------------------------------------------------

HoocParser::ExpressionStatementContext::ExpressionStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::ExpressionContext* HoocParser::ExpressionStatementContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::ExpressionStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}


size_t HoocParser::ExpressionStatementContext::getRuleIndex() const {
  return HoocParser::RuleExpressionStatement;
}

void HoocParser::ExpressionStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpressionStatement(this);
}

void HoocParser::ExpressionStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpressionStatement(this);
}


std::any HoocParser::ExpressionStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitExpressionStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ExpressionStatementContext* HoocParser::expressionStatement() {
  ExpressionStatementContext *_localctx = _tracker.createInstance<ExpressionStatementContext>(_ctx, getState());
  enterRule(_localctx, 78, HoocParser::RuleExpressionStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(508);
    expression();
    setState(509);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- IfStatementContext ------------------------------------------------------------------

HoocParser::IfStatementContext::IfStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::IfStatementContext::IF() {
  return getToken(HoocParser::IF, 0);
}

HoocParser::ExpressionContext* HoocParser::IfStatementContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

std::vector<HoocParser::BlockContext *> HoocParser::IfStatementContext::block() {
  return getRuleContexts<HoocParser::BlockContext>();
}

HoocParser::BlockContext* HoocParser::IfStatementContext::block(size_t i) {
  return getRuleContext<HoocParser::BlockContext>(i);
}

tree::TerminalNode* HoocParser::IfStatementContext::ELSE() {
  return getToken(HoocParser::ELSE, 0);
}


size_t HoocParser::IfStatementContext::getRuleIndex() const {
  return HoocParser::RuleIfStatement;
}

void HoocParser::IfStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterIfStatement(this);
}

void HoocParser::IfStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitIfStatement(this);
}


std::any HoocParser::IfStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitIfStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::IfStatementContext* HoocParser::ifStatement() {
  IfStatementContext *_localctx = _tracker.createInstance<IfStatementContext>(_ctx, getState());
  enterRule(_localctx, 80, HoocParser::RuleIfStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(511);
    match(HoocParser::IF);
    setState(512);
    expression();
    setState(513);
    block();
    setState(516);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::ELSE) {
      setState(514);
      match(HoocParser::ELSE);
      setState(515);
      block();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ForStatementContext ------------------------------------------------------------------

HoocParser::ForStatementContext::ForStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ForStatementContext::FOR() {
  return getToken(HoocParser::FOR, 0);
}

tree::TerminalNode* HoocParser::ForStatementContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::ForStatementContext::IN() {
  return getToken(HoocParser::IN, 0);
}

std::vector<HoocParser::ExpressionContext *> HoocParser::ForStatementContext::expression() {
  return getRuleContexts<HoocParser::ExpressionContext>();
}

HoocParser::ExpressionContext* HoocParser::ForStatementContext::expression(size_t i) {
  return getRuleContext<HoocParser::ExpressionContext>(i);
}

HoocParser::BlockContext* HoocParser::ForStatementContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}

tree::TerminalNode* HoocParser::ForStatementContext::RANGE() {
  return getToken(HoocParser::RANGE, 0);
}

tree::TerminalNode* HoocParser::ForStatementContext::BY() {
  return getToken(HoocParser::BY, 0);
}


size_t HoocParser::ForStatementContext::getRuleIndex() const {
  return HoocParser::RuleForStatement;
}

void HoocParser::ForStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterForStatement(this);
}

void HoocParser::ForStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitForStatement(this);
}


std::any HoocParser::ForStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitForStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ForStatementContext* HoocParser::forStatement() {
  ForStatementContext *_localctx = _tracker.createInstance<ForStatementContext>(_ctx, getState());
  enterRule(_localctx, 82, HoocParser::RuleForStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(518);
    match(HoocParser::FOR);
    setState(519);
    match(HoocParser::IDENTIFIER);
    setState(520);
    match(HoocParser::IN);
    setState(521);
    expression();
    setState(528);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::RANGE) {
      setState(522);
      match(HoocParser::RANGE);
      setState(523);
      expression();
      setState(526);
      _errHandler->sync(this);

      _la = _input->LA(1);
      if (_la == HoocParser::BY) {
        setState(524);
        match(HoocParser::BY);
        setState(525);
        expression();
      }
    }
    setState(530);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- WhileStatementContext ------------------------------------------------------------------

HoocParser::WhileStatementContext::WhileStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::WhileStatementContext::WHILE() {
  return getToken(HoocParser::WHILE, 0);
}

HoocParser::ExpressionContext* HoocParser::WhileStatementContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

HoocParser::BlockContext* HoocParser::WhileStatementContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}


size_t HoocParser::WhileStatementContext::getRuleIndex() const {
  return HoocParser::RuleWhileStatement;
}

void HoocParser::WhileStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterWhileStatement(this);
}

void HoocParser::WhileStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitWhileStatement(this);
}


std::any HoocParser::WhileStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitWhileStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::WhileStatementContext* HoocParser::whileStatement() {
  WhileStatementContext *_localctx = _tracker.createInstance<WhileStatementContext>(_ctx, getState());
  enterRule(_localctx, 84, HoocParser::RuleWhileStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(532);
    match(HoocParser::WHILE);
    setState(533);
    expression();
    setState(534);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ReturnStatementContext ------------------------------------------------------------------

HoocParser::ReturnStatementContext::ReturnStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ReturnStatementContext::RETURN() {
  return getToken(HoocParser::RETURN, 0);
}

tree::TerminalNode* HoocParser::ReturnStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}

HoocParser::ExpressionContext* HoocParser::ReturnStatementContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}


size_t HoocParser::ReturnStatementContext::getRuleIndex() const {
  return HoocParser::RuleReturnStatement;
}

void HoocParser::ReturnStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterReturnStatement(this);
}

void HoocParser::ReturnStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitReturnStatement(this);
}


std::any HoocParser::ReturnStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitReturnStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ReturnStatementContext* HoocParser::returnStatement() {
  ReturnStatementContext *_localctx = _tracker.createInstance<ReturnStatementContext>(_ctx, getState());
  enterRule(_localctx, 86, HoocParser::RuleReturnStatement);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(536);
    match(HoocParser::RETURN);
    setState(538);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & -9223371908005625856) != 0) || ((((_la - 85) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 85)) & 1036545) != 0)) {
      setState(537);
      expression();
    }
    setState(540);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ScopeStatementContext ------------------------------------------------------------------

HoocParser::ScopeStatementContext::ScopeStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ScopeStatementContext::SCOPE() {
  return getToken(HoocParser::SCOPE, 0);
}

HoocParser::BlockContext* HoocParser::ScopeStatementContext::block() {
  return getRuleContext<HoocParser::BlockContext>(0);
}


size_t HoocParser::ScopeStatementContext::getRuleIndex() const {
  return HoocParser::RuleScopeStatement;
}

void HoocParser::ScopeStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterScopeStatement(this);
}

void HoocParser::ScopeStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitScopeStatement(this);
}


std::any HoocParser::ScopeStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitScopeStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ScopeStatementContext* HoocParser::scopeStatement() {
  ScopeStatementContext *_localctx = _tracker.createInstance<ScopeStatementContext>(_ctx, getState());
  enterRule(_localctx, 88, HoocParser::RuleScopeStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(542);
    match(HoocParser::SCOPE);
    setState(543);
    block();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- BreakStatementContext ------------------------------------------------------------------

HoocParser::BreakStatementContext::BreakStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::BreakStatementContext::BREAK() {
  return getToken(HoocParser::BREAK, 0);
}

tree::TerminalNode* HoocParser::BreakStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}


size_t HoocParser::BreakStatementContext::getRuleIndex() const {
  return HoocParser::RuleBreakStatement;
}

void HoocParser::BreakStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterBreakStatement(this);
}

void HoocParser::BreakStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitBreakStatement(this);
}


std::any HoocParser::BreakStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitBreakStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::BreakStatementContext* HoocParser::breakStatement() {
  BreakStatementContext *_localctx = _tracker.createInstance<BreakStatementContext>(_ctx, getState());
  enterRule(_localctx, 90, HoocParser::RuleBreakStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(545);
    match(HoocParser::BREAK);
    setState(546);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ContinueStatementContext ------------------------------------------------------------------

HoocParser::ContinueStatementContext::ContinueStatementContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::ContinueStatementContext::CONTINUE() {
  return getToken(HoocParser::CONTINUE, 0);
}

tree::TerminalNode* HoocParser::ContinueStatementContext::SEMICOLON() {
  return getToken(HoocParser::SEMICOLON, 0);
}


size_t HoocParser::ContinueStatementContext::getRuleIndex() const {
  return HoocParser::RuleContinueStatement;
}

void HoocParser::ContinueStatementContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterContinueStatement(this);
}

void HoocParser::ContinueStatementContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitContinueStatement(this);
}


std::any HoocParser::ContinueStatementContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitContinueStatement(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ContinueStatementContext* HoocParser::continueStatement() {
  ContinueStatementContext *_localctx = _tracker.createInstance<ContinueStatementContext>(_ctx, getState());
  enterRule(_localctx, 92, HoocParser::RuleContinueStatement);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(548);
    match(HoocParser::CONTINUE);
    setState(549);
    match(HoocParser::SEMICOLON);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionContext ------------------------------------------------------------------

HoocParser::ExpressionContext::ExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::AssignmentExpressionContext* HoocParser::ExpressionContext::assignmentExpression() {
  return getRuleContext<HoocParser::AssignmentExpressionContext>(0);
}


size_t HoocParser::ExpressionContext::getRuleIndex() const {
  return HoocParser::RuleExpression;
}

void HoocParser::ExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpression(this);
}

void HoocParser::ExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpression(this);
}


std::any HoocParser::ExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ExpressionContext* HoocParser::expression() {
  ExpressionContext *_localctx = _tracker.createInstance<ExpressionContext>(_ctx, getState());
  enterRule(_localctx, 94, HoocParser::RuleExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(551);
    assignmentExpression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AssignmentExpressionContext ------------------------------------------------------------------

HoocParser::AssignmentExpressionContext::AssignmentExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::LogicalOrExpressionContext *> HoocParser::AssignmentExpressionContext::logicalOrExpression() {
  return getRuleContexts<HoocParser::LogicalOrExpressionContext>();
}

HoocParser::LogicalOrExpressionContext* HoocParser::AssignmentExpressionContext::logicalOrExpression(size_t i) {
  return getRuleContext<HoocParser::LogicalOrExpressionContext>(i);
}

tree::TerminalNode* HoocParser::AssignmentExpressionContext::ASSIGN() {
  return getToken(HoocParser::ASSIGN, 0);
}

HoocParser::CompoundAssignmentContext* HoocParser::AssignmentExpressionContext::compoundAssignment() {
  return getRuleContext<HoocParser::CompoundAssignmentContext>(0);
}


size_t HoocParser::AssignmentExpressionContext::getRuleIndex() const {
  return HoocParser::RuleAssignmentExpression;
}

void HoocParser::AssignmentExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAssignmentExpression(this);
}

void HoocParser::AssignmentExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAssignmentExpression(this);
}


std::any HoocParser::AssignmentExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitAssignmentExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::AssignmentExpressionContext* HoocParser::assignmentExpression() {
  AssignmentExpressionContext *_localctx = _tracker.createInstance<AssignmentExpressionContext>(_ctx, getState());
  enterRule(_localctx, 96, HoocParser::RuleAssignmentExpression);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(553);
    logicalOrExpression();
    setState(557);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::ASSIGN: {
        setState(554);
        match(HoocParser::ASSIGN);
        setState(555);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_PLUS:
      case HoocParser::COMPOUND_MINUS:
      case HoocParser::COMPOUND_MULTIPLY:
      case HoocParser::COMPOUND_DIVIDE:
      case HoocParser::COMPOUND_MODULO:
      case HoocParser::COMPOUND_LEFT_SHIFT:
      case HoocParser::COMPOUND_RIGHT_SHIFT: {
        setState(556);
        compoundAssignment();
        break;
      }

      case HoocParser::EOF:
      case HoocParser::FUNC:
      case HoocParser::CLASS:
      case HoocParser::VAR:
      case HoocParser::CONST:
      case HoocParser::FINAL:
      case HoocParser::SINGLETON:
      case HoocParser::IMMUTABLE:
      case HoocParser::FACTORY:
      case HoocParser::OBSERVABLE:
      case HoocParser::SERVICE:
      case HoocParser::STRATEGY:
      case HoocParser::ACTOR:
      case HoocParser::BY:
      case HoocParser::NATIVE:
      case HoocParser::EXTERN:
      case HoocParser::LIBRARY:
      case HoocParser::LINK:
      case HoocParser::RANGE:
      case HoocParser::SEMICOLON:
      case HoocParser::COMMA:
      case HoocParser::RPAREN:
      case HoocParser::LBRACE:
      case HoocParser::RBRACKET: {
        break;
      }

    default:
      break;
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- CompoundAssignmentContext ------------------------------------------------------------------

HoocParser::CompoundAssignmentContext::CompoundAssignmentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_PLUS() {
  return getToken(HoocParser::COMPOUND_PLUS, 0);
}

HoocParser::LogicalOrExpressionContext* HoocParser::CompoundAssignmentContext::logicalOrExpression() {
  return getRuleContext<HoocParser::LogicalOrExpressionContext>(0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_MINUS() {
  return getToken(HoocParser::COMPOUND_MINUS, 0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_MULTIPLY() {
  return getToken(HoocParser::COMPOUND_MULTIPLY, 0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_DIVIDE() {
  return getToken(HoocParser::COMPOUND_DIVIDE, 0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_MODULO() {
  return getToken(HoocParser::COMPOUND_MODULO, 0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_LEFT_SHIFT() {
  return getToken(HoocParser::COMPOUND_LEFT_SHIFT, 0);
}

tree::TerminalNode* HoocParser::CompoundAssignmentContext::COMPOUND_RIGHT_SHIFT() {
  return getToken(HoocParser::COMPOUND_RIGHT_SHIFT, 0);
}


size_t HoocParser::CompoundAssignmentContext::getRuleIndex() const {
  return HoocParser::RuleCompoundAssignment;
}

void HoocParser::CompoundAssignmentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterCompoundAssignment(this);
}

void HoocParser::CompoundAssignmentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitCompoundAssignment(this);
}


std::any HoocParser::CompoundAssignmentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitCompoundAssignment(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::CompoundAssignmentContext* HoocParser::compoundAssignment() {
  CompoundAssignmentContext *_localctx = _tracker.createInstance<CompoundAssignmentContext>(_ctx, getState());
  enterRule(_localctx, 98, HoocParser::RuleCompoundAssignment);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(573);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::COMPOUND_PLUS: {
        enterOuterAlt(_localctx, 1);
        setState(559);
        match(HoocParser::COMPOUND_PLUS);
        setState(560);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_MINUS: {
        enterOuterAlt(_localctx, 2);
        setState(561);
        match(HoocParser::COMPOUND_MINUS);
        setState(562);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_MULTIPLY: {
        enterOuterAlt(_localctx, 3);
        setState(563);
        match(HoocParser::COMPOUND_MULTIPLY);
        setState(564);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_DIVIDE: {
        enterOuterAlt(_localctx, 4);
        setState(565);
        match(HoocParser::COMPOUND_DIVIDE);
        setState(566);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_MODULO: {
        enterOuterAlt(_localctx, 5);
        setState(567);
        match(HoocParser::COMPOUND_MODULO);
        setState(568);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_LEFT_SHIFT: {
        enterOuterAlt(_localctx, 6);
        setState(569);
        match(HoocParser::COMPOUND_LEFT_SHIFT);
        setState(570);
        logicalOrExpression();
        break;
      }

      case HoocParser::COMPOUND_RIGHT_SHIFT: {
        enterOuterAlt(_localctx, 7);
        setState(571);
        match(HoocParser::COMPOUND_RIGHT_SHIFT);
        setState(572);
        logicalOrExpression();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LogicalOrExpressionContext ------------------------------------------------------------------

HoocParser::LogicalOrExpressionContext::LogicalOrExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::LogicalAndExpressionContext *> HoocParser::LogicalOrExpressionContext::logicalAndExpression() {
  return getRuleContexts<HoocParser::LogicalAndExpressionContext>();
}

HoocParser::LogicalAndExpressionContext* HoocParser::LogicalOrExpressionContext::logicalAndExpression(size_t i) {
  return getRuleContext<HoocParser::LogicalAndExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::LogicalOrExpressionContext::OR() {
  return getTokens(HoocParser::OR);
}

tree::TerminalNode* HoocParser::LogicalOrExpressionContext::OR(size_t i) {
  return getToken(HoocParser::OR, i);
}


size_t HoocParser::LogicalOrExpressionContext::getRuleIndex() const {
  return HoocParser::RuleLogicalOrExpression;
}

void HoocParser::LogicalOrExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLogicalOrExpression(this);
}

void HoocParser::LogicalOrExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLogicalOrExpression(this);
}


std::any HoocParser::LogicalOrExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitLogicalOrExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::LogicalOrExpressionContext* HoocParser::logicalOrExpression() {
  LogicalOrExpressionContext *_localctx = _tracker.createInstance<LogicalOrExpressionContext>(_ctx, getState());
  enterRule(_localctx, 100, HoocParser::RuleLogicalOrExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(575);
    logicalAndExpression();
    setState(580);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::OR) {
      setState(576);
      match(HoocParser::OR);
      setState(577);
      logicalAndExpression();
      setState(582);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- LogicalAndExpressionContext ------------------------------------------------------------------

HoocParser::LogicalAndExpressionContext::LogicalAndExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::RelationalExpressionContext *> HoocParser::LogicalAndExpressionContext::relationalExpression() {
  return getRuleContexts<HoocParser::RelationalExpressionContext>();
}

HoocParser::RelationalExpressionContext* HoocParser::LogicalAndExpressionContext::relationalExpression(size_t i) {
  return getRuleContext<HoocParser::RelationalExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::LogicalAndExpressionContext::AND() {
  return getTokens(HoocParser::AND);
}

tree::TerminalNode* HoocParser::LogicalAndExpressionContext::AND(size_t i) {
  return getToken(HoocParser::AND, i);
}


size_t HoocParser::LogicalAndExpressionContext::getRuleIndex() const {
  return HoocParser::RuleLogicalAndExpression;
}

void HoocParser::LogicalAndExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterLogicalAndExpression(this);
}

void HoocParser::LogicalAndExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitLogicalAndExpression(this);
}


std::any HoocParser::LogicalAndExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitLogicalAndExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::LogicalAndExpressionContext* HoocParser::logicalAndExpression() {
  LogicalAndExpressionContext *_localctx = _tracker.createInstance<LogicalAndExpressionContext>(_ctx, getState());
  enterRule(_localctx, 102, HoocParser::RuleLogicalAndExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(583);
    relationalExpression();
    setState(588);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::AND) {
      setState(584);
      match(HoocParser::AND);
      setState(585);
      relationalExpression();
      setState(590);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- RelationalExpressionContext ------------------------------------------------------------------

HoocParser::RelationalExpressionContext::RelationalExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::AdditiveExpressionContext *> HoocParser::RelationalExpressionContext::additiveExpression() {
  return getRuleContexts<HoocParser::AdditiveExpressionContext>();
}

HoocParser::AdditiveExpressionContext* HoocParser::RelationalExpressionContext::additiveExpression(size_t i) {
  return getRuleContext<HoocParser::AdditiveExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::EQUALS() {
  return getTokens(HoocParser::EQUALS);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::EQUALS(size_t i) {
  return getToken(HoocParser::EQUALS, i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::NOT_EQUALS() {
  return getTokens(HoocParser::NOT_EQUALS);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::NOT_EQUALS(size_t i) {
  return getToken(HoocParser::NOT_EQUALS, i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::LESS() {
  return getTokens(HoocParser::LESS);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::LESS(size_t i) {
  return getToken(HoocParser::LESS, i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::LESS_EQUALS() {
  return getTokens(HoocParser::LESS_EQUALS);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::LESS_EQUALS(size_t i) {
  return getToken(HoocParser::LESS_EQUALS, i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::GREATER() {
  return getTokens(HoocParser::GREATER);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::GREATER(size_t i) {
  return getToken(HoocParser::GREATER, i);
}

std::vector<tree::TerminalNode *> HoocParser::RelationalExpressionContext::GREATER_EQUALS() {
  return getTokens(HoocParser::GREATER_EQUALS);
}

tree::TerminalNode* HoocParser::RelationalExpressionContext::GREATER_EQUALS(size_t i) {
  return getToken(HoocParser::GREATER_EQUALS, i);
}


size_t HoocParser::RelationalExpressionContext::getRuleIndex() const {
  return HoocParser::RuleRelationalExpression;
}

void HoocParser::RelationalExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterRelationalExpression(this);
}

void HoocParser::RelationalExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitRelationalExpression(this);
}


std::any HoocParser::RelationalExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitRelationalExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::RelationalExpressionContext* HoocParser::relationalExpression() {
  RelationalExpressionContext *_localctx = _tracker.createInstance<RelationalExpressionContext>(_ctx, getState());
  enterRule(_localctx, 104, HoocParser::RuleRelationalExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(591);
    additiveExpression();
    setState(596);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (((((_la - 77) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 77)) & 63) != 0)) {
      setState(592);
      _la = _input->LA(1);
      if (!(((((_la - 77) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 77)) & 63) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(593);
      additiveExpression();
      setState(598);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AdditiveExpressionContext ------------------------------------------------------------------

HoocParser::AdditiveExpressionContext::AdditiveExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::MultiplicativeExpressionContext *> HoocParser::AdditiveExpressionContext::multiplicativeExpression() {
  return getRuleContexts<HoocParser::MultiplicativeExpressionContext>();
}

HoocParser::MultiplicativeExpressionContext* HoocParser::AdditiveExpressionContext::multiplicativeExpression(size_t i) {
  return getRuleContext<HoocParser::MultiplicativeExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::AdditiveExpressionContext::PLUS() {
  return getTokens(HoocParser::PLUS);
}

tree::TerminalNode* HoocParser::AdditiveExpressionContext::PLUS(size_t i) {
  return getToken(HoocParser::PLUS, i);
}

std::vector<tree::TerminalNode *> HoocParser::AdditiveExpressionContext::MINUS() {
  return getTokens(HoocParser::MINUS);
}

tree::TerminalNode* HoocParser::AdditiveExpressionContext::MINUS(size_t i) {
  return getToken(HoocParser::MINUS, i);
}


size_t HoocParser::AdditiveExpressionContext::getRuleIndex() const {
  return HoocParser::RuleAdditiveExpression;
}

void HoocParser::AdditiveExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAdditiveExpression(this);
}

void HoocParser::AdditiveExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAdditiveExpression(this);
}


std::any HoocParser::AdditiveExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitAdditiveExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::AdditiveExpressionContext* HoocParser::additiveExpression() {
  AdditiveExpressionContext *_localctx = _tracker.createInstance<AdditiveExpressionContext>(_ctx, getState());
  enterRule(_localctx, 106, HoocParser::RuleAdditiveExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(599);
    multiplicativeExpression();
    setState(604);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::PLUS

    || _la == HoocParser::MINUS) {
      setState(600);
      _la = _input->LA(1);
      if (!(_la == HoocParser::PLUS

      || _la == HoocParser::MINUS)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(601);
      multiplicativeExpression();
      setState(606);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- MultiplicativeExpressionContext ------------------------------------------------------------------

HoocParser::MultiplicativeExpressionContext::MultiplicativeExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::UnaryExpressionContext *> HoocParser::MultiplicativeExpressionContext::unaryExpression() {
  return getRuleContexts<HoocParser::UnaryExpressionContext>();
}

HoocParser::UnaryExpressionContext* HoocParser::MultiplicativeExpressionContext::unaryExpression(size_t i) {
  return getRuleContext<HoocParser::UnaryExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::MultiplicativeExpressionContext::MULTIPLY() {
  return getTokens(HoocParser::MULTIPLY);
}

tree::TerminalNode* HoocParser::MultiplicativeExpressionContext::MULTIPLY(size_t i) {
  return getToken(HoocParser::MULTIPLY, i);
}

std::vector<tree::TerminalNode *> HoocParser::MultiplicativeExpressionContext::DIVIDE() {
  return getTokens(HoocParser::DIVIDE);
}

tree::TerminalNode* HoocParser::MultiplicativeExpressionContext::DIVIDE(size_t i) {
  return getToken(HoocParser::DIVIDE, i);
}

std::vector<tree::TerminalNode *> HoocParser::MultiplicativeExpressionContext::MODULO() {
  return getTokens(HoocParser::MODULO);
}

tree::TerminalNode* HoocParser::MultiplicativeExpressionContext::MODULO(size_t i) {
  return getToken(HoocParser::MODULO, i);
}


size_t HoocParser::MultiplicativeExpressionContext::getRuleIndex() const {
  return HoocParser::RuleMultiplicativeExpression;
}

void HoocParser::MultiplicativeExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterMultiplicativeExpression(this);
}

void HoocParser::MultiplicativeExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitMultiplicativeExpression(this);
}


std::any HoocParser::MultiplicativeExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitMultiplicativeExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::MultiplicativeExpressionContext* HoocParser::multiplicativeExpression() {
  MultiplicativeExpressionContext *_localctx = _tracker.createInstance<MultiplicativeExpressionContext>(_ctx, getState());
  enterRule(_localctx, 108, HoocParser::RuleMultiplicativeExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(607);
    unaryExpression();
    setState(612);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (((((_la - 64) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 64)) & 7) != 0)) {
      setState(608);
      _la = _input->LA(1);
      if (!(((((_la - 64) & ~ 0x3fULL) == 0) &&
        ((1ULL << (_la - 64)) & 7) != 0))) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
      setState(609);
      unaryExpression();
      setState(614);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- UnaryExpressionContext ------------------------------------------------------------------

HoocParser::UnaryExpressionContext::UnaryExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::PostfixExpressionContext* HoocParser::UnaryExpressionContext::postfixExpression() {
  return getRuleContext<HoocParser::PostfixExpressionContext>(0);
}

tree::TerminalNode* HoocParser::UnaryExpressionContext::MINUS() {
  return getToken(HoocParser::MINUS, 0);
}

tree::TerminalNode* HoocParser::UnaryExpressionContext::NOT() {
  return getToken(HoocParser::NOT, 0);
}


size_t HoocParser::UnaryExpressionContext::getRuleIndex() const {
  return HoocParser::RuleUnaryExpression;
}

void HoocParser::UnaryExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterUnaryExpression(this);
}

void HoocParser::UnaryExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitUnaryExpression(this);
}


std::any HoocParser::UnaryExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitUnaryExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::UnaryExpressionContext* HoocParser::unaryExpression() {
  UnaryExpressionContext *_localctx = _tracker.createInstance<UnaryExpressionContext>(_ctx, getState());
  enterRule(_localctx, 110, HoocParser::RuleUnaryExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(616);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if (_la == HoocParser::MINUS

    || _la == HoocParser::NOT) {
      setState(615);
      _la = _input->LA(1);
      if (!(_la == HoocParser::MINUS

      || _la == HoocParser::NOT)) {
      _errHandler->recoverInline(this);
      }
      else {
        _errHandler->reportMatch(this);
        consume();
      }
    }
    setState(618);
    postfixExpression();
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PostfixExpressionContext ------------------------------------------------------------------

HoocParser::PostfixExpressionContext::PostfixExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

HoocParser::PrimaryContext* HoocParser::PostfixExpressionContext::primary() {
  return getRuleContext<HoocParser::PrimaryContext>(0);
}

std::vector<HoocParser::PostfixSuffixContext *> HoocParser::PostfixExpressionContext::postfixSuffix() {
  return getRuleContexts<HoocParser::PostfixSuffixContext>();
}

HoocParser::PostfixSuffixContext* HoocParser::PostfixExpressionContext::postfixSuffix(size_t i) {
  return getRuleContext<HoocParser::PostfixSuffixContext>(i);
}

std::vector<HoocParser::AugmentedAssignmentContext *> HoocParser::PostfixExpressionContext::augmentedAssignment() {
  return getRuleContexts<HoocParser::AugmentedAssignmentContext>();
}

HoocParser::AugmentedAssignmentContext* HoocParser::PostfixExpressionContext::augmentedAssignment(size_t i) {
  return getRuleContext<HoocParser::AugmentedAssignmentContext>(i);
}


size_t HoocParser::PostfixExpressionContext::getRuleIndex() const {
  return HoocParser::RulePostfixExpression;
}

void HoocParser::PostfixExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPostfixExpression(this);
}

void HoocParser::PostfixExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPostfixExpression(this);
}


std::any HoocParser::PostfixExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitPostfixExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::PostfixExpressionContext* HoocParser::postfixExpression() {
  PostfixExpressionContext *_localctx = _tracker.createInstance<PostfixExpressionContext>(_ctx, getState());
  enterRule(_localctx, 112, HoocParser::RulePostfixExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(620);
    primary();
    setState(625);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (((((_la - 75) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 75)) & 4521987) != 0)) {
      setState(623);
      _errHandler->sync(this);
      switch (_input->LA(1)) {
        case HoocParser::DOT:
        case HoocParser::LPAREN:
        case HoocParser::LBRACKET: {
          setState(621);
          postfixSuffix();
          break;
        }

        case HoocParser::INCREMENT:
        case HoocParser::DECREMENT: {
          setState(622);
          augmentedAssignment();
          break;
        }

      default:
        throw NoViableAltException(this);
      }
      setState(627);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PostfixSuffixContext ------------------------------------------------------------------

HoocParser::PostfixSuffixContext::PostfixSuffixContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::DOT() {
  return getToken(HoocParser::DOT, 0);
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

HoocParser::ExpressionContext* HoocParser::PostfixSuffixContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::PostfixSuffixContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

HoocParser::ArgumentListContext* HoocParser::PostfixSuffixContext::argumentList() {
  return getRuleContext<HoocParser::ArgumentListContext>(0);
}


size_t HoocParser::PostfixSuffixContext::getRuleIndex() const {
  return HoocParser::RulePostfixSuffix;
}

void HoocParser::PostfixSuffixContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPostfixSuffix(this);
}

void HoocParser::PostfixSuffixContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPostfixSuffix(this);
}


std::any HoocParser::PostfixSuffixContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitPostfixSuffix(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::PostfixSuffixContext* HoocParser::postfixSuffix() {
  PostfixSuffixContext *_localctx = _tracker.createInstance<PostfixSuffixContext>(_ctx, getState());
  enterRule(_localctx, 114, HoocParser::RulePostfixSuffix);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(639);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::DOT: {
        enterOuterAlt(_localctx, 1);
        setState(628);
        match(HoocParser::DOT);
        setState(629);
        match(HoocParser::IDENTIFIER);
        break;
      }

      case HoocParser::LBRACKET: {
        enterOuterAlt(_localctx, 2);
        setState(630);
        match(HoocParser::LBRACKET);
        setState(631);
        expression();
        setState(632);
        match(HoocParser::RBRACKET);
        break;
      }

      case HoocParser::LPAREN: {
        enterOuterAlt(_localctx, 3);
        setState(634);
        match(HoocParser::LPAREN);
        setState(636);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & -9223371908005625856) != 0) || ((((_la - 85) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 85)) & 1036545) != 0)) {
          setState(635);
          argumentList();
        }
        setState(638);
        match(HoocParser::RPAREN);
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- AugmentedAssignmentContext ------------------------------------------------------------------

HoocParser::AugmentedAssignmentContext::AugmentedAssignmentContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::AugmentedAssignmentContext::INCREMENT() {
  return getToken(HoocParser::INCREMENT, 0);
}

tree::TerminalNode* HoocParser::AugmentedAssignmentContext::DECREMENT() {
  return getToken(HoocParser::DECREMENT, 0);
}


size_t HoocParser::AugmentedAssignmentContext::getRuleIndex() const {
  return HoocParser::RuleAugmentedAssignment;
}

void HoocParser::AugmentedAssignmentContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterAugmentedAssignment(this);
}

void HoocParser::AugmentedAssignmentContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitAugmentedAssignment(this);
}


std::any HoocParser::AugmentedAssignmentContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitAugmentedAssignment(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::AugmentedAssignmentContext* HoocParser::augmentedAssignment() {
  AugmentedAssignmentContext *_localctx = _tracker.createInstance<AugmentedAssignmentContext>(_ctx, getState());
  enterRule(_localctx, 116, HoocParser::RuleAugmentedAssignment);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(641);
    _la = _input->LA(1);
    if (!(_la == HoocParser::INCREMENT

    || _la == HoocParser::DECREMENT)) {
    _errHandler->recoverInline(this);
    }
    else {
      _errHandler->reportMatch(this);
      consume();
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- PrimaryContext ------------------------------------------------------------------

HoocParser::PrimaryContext::PrimaryContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::PrimaryContext::IDENTIFIER() {
  return getToken(HoocParser::IDENTIFIER, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::THIS() {
  return getToken(HoocParser::THIS, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::INTEGER_LITERAL() {
  return getToken(HoocParser::INTEGER_LITERAL, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::FLOATING_LITERAL() {
  return getToken(HoocParser::FLOATING_LITERAL, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::STRING_LITERAL() {
  return getToken(HoocParser::STRING_LITERAL, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::MULTILINE_STRING() {
  return getToken(HoocParser::MULTILINE_STRING, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::CHAR_LITERAL() {
  return getToken(HoocParser::CHAR_LITERAL, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::TRUE() {
  return getToken(HoocParser::TRUE, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::FALSE() {
  return getToken(HoocParser::FALSE, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::NULL_() {
  return getToken(HoocParser::NULL_, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::LBRACKET() {
  return getToken(HoocParser::LBRACKET, 0);
}

tree::TerminalNode* HoocParser::PrimaryContext::RBRACKET() {
  return getToken(HoocParser::RBRACKET, 0);
}

HoocParser::ExpressionListContext* HoocParser::PrimaryContext::expressionList() {
  return getRuleContext<HoocParser::ExpressionListContext>(0);
}

tree::TerminalNode* HoocParser::PrimaryContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

HoocParser::ExpressionContext* HoocParser::PrimaryContext::expression() {
  return getRuleContext<HoocParser::ExpressionContext>(0);
}

tree::TerminalNode* HoocParser::PrimaryContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

HoocParser::NewExpressionContext* HoocParser::PrimaryContext::newExpression() {
  return getRuleContext<HoocParser::NewExpressionContext>(0);
}


size_t HoocParser::PrimaryContext::getRuleIndex() const {
  return HoocParser::RulePrimary;
}

void HoocParser::PrimaryContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterPrimary(this);
}

void HoocParser::PrimaryContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitPrimary(this);
}


std::any HoocParser::PrimaryContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitPrimary(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::PrimaryContext* HoocParser::primary() {
  PrimaryContext *_localctx = _tracker.createInstance<PrimaryContext>(_ctx, getState());
  enterRule(_localctx, 118, HoocParser::RulePrimary);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    setState(663);
    _errHandler->sync(this);
    switch (_input->LA(1)) {
      case HoocParser::IDENTIFIER: {
        enterOuterAlt(_localctx, 1);
        setState(643);
        match(HoocParser::IDENTIFIER);
        break;
      }

      case HoocParser::THIS: {
        enterOuterAlt(_localctx, 2);
        setState(644);
        match(HoocParser::THIS);
        break;
      }

      case HoocParser::INTEGER_LITERAL: {
        enterOuterAlt(_localctx, 3);
        setState(645);
        match(HoocParser::INTEGER_LITERAL);
        break;
      }

      case HoocParser::FLOATING_LITERAL: {
        enterOuterAlt(_localctx, 4);
        setState(646);
        match(HoocParser::FLOATING_LITERAL);
        break;
      }

      case HoocParser::STRING_LITERAL: {
        enterOuterAlt(_localctx, 5);
        setState(647);
        match(HoocParser::STRING_LITERAL);
        break;
      }

      case HoocParser::MULTILINE_STRING: {
        enterOuterAlt(_localctx, 6);
        setState(648);
        match(HoocParser::MULTILINE_STRING);
        break;
      }

      case HoocParser::CHAR_LITERAL: {
        enterOuterAlt(_localctx, 7);
        setState(649);
        match(HoocParser::CHAR_LITERAL);
        break;
      }

      case HoocParser::TRUE: {
        enterOuterAlt(_localctx, 8);
        setState(650);
        match(HoocParser::TRUE);
        break;
      }

      case HoocParser::FALSE: {
        enterOuterAlt(_localctx, 9);
        setState(651);
        match(HoocParser::FALSE);
        break;
      }

      case HoocParser::NULL_: {
        enterOuterAlt(_localctx, 10);
        setState(652);
        match(HoocParser::NULL_);
        break;
      }

      case HoocParser::LBRACKET: {
        enterOuterAlt(_localctx, 11);
        setState(653);
        match(HoocParser::LBRACKET);
        setState(655);
        _errHandler->sync(this);

        _la = _input->LA(1);
        if ((((_la & ~ 0x3fULL) == 0) &&
          ((1ULL << _la) & -9223371908005625856) != 0) || ((((_la - 85) & ~ 0x3fULL) == 0) &&
          ((1ULL << (_la - 85)) & 1036545) != 0)) {
          setState(654);
          expressionList();
        }
        setState(657);
        match(HoocParser::RBRACKET);
        break;
      }

      case HoocParser::LPAREN: {
        enterOuterAlt(_localctx, 12);
        setState(658);
        match(HoocParser::LPAREN);
        setState(659);
        expression();
        setState(660);
        match(HoocParser::RPAREN);
        break;
      }

      case HoocParser::NEW: {
        enterOuterAlt(_localctx, 13);
        setState(662);
        newExpression();
        break;
      }

    default:
      throw NoViableAltException(this);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- NewExpressionContext ------------------------------------------------------------------

HoocParser::NewExpressionContext::NewExpressionContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::NewExpressionContext::NEW() {
  return getToken(HoocParser::NEW, 0);
}

HoocParser::QualifiedIdentifierContext* HoocParser::NewExpressionContext::qualifiedIdentifier() {
  return getRuleContext<HoocParser::QualifiedIdentifierContext>(0);
}

tree::TerminalNode* HoocParser::NewExpressionContext::LPAREN() {
  return getToken(HoocParser::LPAREN, 0);
}

tree::TerminalNode* HoocParser::NewExpressionContext::RPAREN() {
  return getToken(HoocParser::RPAREN, 0);
}

HoocParser::ArgumentListContext* HoocParser::NewExpressionContext::argumentList() {
  return getRuleContext<HoocParser::ArgumentListContext>(0);
}


size_t HoocParser::NewExpressionContext::getRuleIndex() const {
  return HoocParser::RuleNewExpression;
}

void HoocParser::NewExpressionContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterNewExpression(this);
}

void HoocParser::NewExpressionContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitNewExpression(this);
}


std::any HoocParser::NewExpressionContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitNewExpression(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::NewExpressionContext* HoocParser::newExpression() {
  NewExpressionContext *_localctx = _tracker.createInstance<NewExpressionContext>(_ctx, getState());
  enterRule(_localctx, 120, HoocParser::RuleNewExpression);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(665);
    match(HoocParser::NEW);
    setState(666);
    qualifiedIdentifier();
    setState(667);
    match(HoocParser::LPAREN);
    setState(669);
    _errHandler->sync(this);

    _la = _input->LA(1);
    if ((((_la & ~ 0x3fULL) == 0) &&
      ((1ULL << _la) & -9223371908005625856) != 0) || ((((_la - 85) & ~ 0x3fULL) == 0) &&
      ((1ULL << (_la - 85)) & 1036545) != 0)) {
      setState(668);
      argumentList();
    }
    setState(671);
    match(HoocParser::RPAREN);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- InterpolatedStringContext ------------------------------------------------------------------

HoocParser::InterpolatedStringContext::InterpolatedStringContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

tree::TerminalNode* HoocParser::InterpolatedStringContext::STRING_LITERAL() {
  return getToken(HoocParser::STRING_LITERAL, 0);
}


size_t HoocParser::InterpolatedStringContext::getRuleIndex() const {
  return HoocParser::RuleInterpolatedString;
}

void HoocParser::InterpolatedStringContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterInterpolatedString(this);
}

void HoocParser::InterpolatedStringContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitInterpolatedString(this);
}


std::any HoocParser::InterpolatedStringContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitInterpolatedString(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::InterpolatedStringContext* HoocParser::interpolatedString() {
  InterpolatedStringContext *_localctx = _tracker.createInstance<InterpolatedStringContext>(_ctx, getState());
  enterRule(_localctx, 122, HoocParser::RuleInterpolatedString);

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(673);
    match(HoocParser::STRING_LITERAL);
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ArgumentListContext ------------------------------------------------------------------

HoocParser::ArgumentListContext::ArgumentListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::ExpressionContext *> HoocParser::ArgumentListContext::expression() {
  return getRuleContexts<HoocParser::ExpressionContext>();
}

HoocParser::ExpressionContext* HoocParser::ArgumentListContext::expression(size_t i) {
  return getRuleContext<HoocParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::ArgumentListContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::ArgumentListContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}


size_t HoocParser::ArgumentListContext::getRuleIndex() const {
  return HoocParser::RuleArgumentList;
}

void HoocParser::ArgumentListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterArgumentList(this);
}

void HoocParser::ArgumentListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitArgumentList(this);
}


std::any HoocParser::ArgumentListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitArgumentList(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ArgumentListContext* HoocParser::argumentList() {
  ArgumentListContext *_localctx = _tracker.createInstance<ArgumentListContext>(_ctx, getState());
  enterRule(_localctx, 124, HoocParser::RuleArgumentList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(675);
    expression();
    setState(680);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::COMMA) {
      setState(676);
      match(HoocParser::COMMA);
      setState(677);
      expression();
      setState(682);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

//----------------- ExpressionListContext ------------------------------------------------------------------

HoocParser::ExpressionListContext::ExpressionListContext(ParserRuleContext *parent, size_t invokingState)
  : ParserRuleContext(parent, invokingState) {
}

std::vector<HoocParser::ExpressionContext *> HoocParser::ExpressionListContext::expression() {
  return getRuleContexts<HoocParser::ExpressionContext>();
}

HoocParser::ExpressionContext* HoocParser::ExpressionListContext::expression(size_t i) {
  return getRuleContext<HoocParser::ExpressionContext>(i);
}

std::vector<tree::TerminalNode *> HoocParser::ExpressionListContext::COMMA() {
  return getTokens(HoocParser::COMMA);
}

tree::TerminalNode* HoocParser::ExpressionListContext::COMMA(size_t i) {
  return getToken(HoocParser::COMMA, i);
}


size_t HoocParser::ExpressionListContext::getRuleIndex() const {
  return HoocParser::RuleExpressionList;
}

void HoocParser::ExpressionListContext::enterRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->enterExpressionList(this);
}

void HoocParser::ExpressionListContext::exitRule(tree::ParseTreeListener *listener) {
  auto parserListener = dynamic_cast<HoocListener *>(listener);
  if (parserListener != nullptr)
    parserListener->exitExpressionList(this);
}


std::any HoocParser::ExpressionListContext::accept(tree::ParseTreeVisitor *visitor) {
  if (auto parserVisitor = dynamic_cast<HoocVisitor*>(visitor))
    return parserVisitor->visitExpressionList(this);
  else
    return visitor->visitChildren(this);
}

HoocParser::ExpressionListContext* HoocParser::expressionList() {
  ExpressionListContext *_localctx = _tracker.createInstance<ExpressionListContext>(_ctx, getState());
  enterRule(_localctx, 126, HoocParser::RuleExpressionList);
  size_t _la = 0;

#if __cplusplus > 201703L
  auto onExit = finally([=, this] {
#else
  auto onExit = finally([=] {
#endif
    exitRule();
  });
  try {
    enterOuterAlt(_localctx, 1);
    setState(683);
    expression();
    setState(688);
    _errHandler->sync(this);
    _la = _input->LA(1);
    while (_la == HoocParser::COMMA) {
      setState(684);
      match(HoocParser::COMMA);
      setState(685);
      expression();
      setState(690);
      _errHandler->sync(this);
      _la = _input->LA(1);
    }
   
  }
  catch (RecognitionException &e) {
    _errHandler->reportError(this, e);
    _localctx->exception = std::current_exception();
    _errHandler->recover(this, _localctx->exception);
  }

  return _localctx;
}

void HoocParser::initialize() {
#if ANTLR4_USE_THREAD_LOCAL_CACHE
  hoocParserInitialize();
#else
  ::antlr4::internal::call_once(hoocParserOnceFlag, hoocParserInitialize);
#endif
}
