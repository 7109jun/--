#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <memory>
#include <variant>
#include <cctype>
#include <stdexcept>
#include <sstream>
#include <iomanip>
#include <cmath>

// =============================================================================
// 1. LEXER (문맥 인식 x 처리)
// =============================================================================

enum class TokenType {
    KEYWORD_DEF, KEYWORD_VAR_CHANGE, KEYWORD_PRINT, KEYWORD_CONSOLE,
    IDENTIFIER, NUMBER, STRING,
    OP_PLUS, OP_MINUS, OP_MUL, OP_DIV, OP_ASSIGN,
    SYM_LPAREN, SYM_RPAREN,
    EOF_TOKEN, UNKNOWN
};

struct Token {
    TokenType type;
    std::string value;
    int line;
};

class Lexer {
private:
    const std::string& source;
    size_t pos = 0;
    int currentLine = 1;
    TokenType lastTokenType = TokenType::EOF_TOKEN; // 이전 토큰 추적

    bool isEnd() { return pos >= source.length(); }
    char peek() { return isEnd() ? '\0' : source[pos]; }
    char advance() { return source[pos++]; }

    void pushToken(std::vector<Token>& tokens, TokenType type, std::string value) {
        tokens.push_back({type, std::move(value), currentLine});
        lastTokenType = type;
    }

public:
    Lexer(const std::string& src) : source(src) {}

    std::vector<Token> tokenize() {
        std::vector<Token> tokens;

        while (!isEnd()) {
            char c = peek();

            if (std::isspace(c)) { if (c == '\n') currentLine++; advance(); continue; }
            if (c == '#') { while (!isEnd() && peek() != '\n') advance(); continue; }

            if (c == '+') { pushToken(tokens, TokenType::OP_PLUS, "+"); advance(); }
            else if (c == '-') { pushToken(tokens, TokenType::OP_MINUS, "-"); advance(); }
            else if (c == '/') { pushToken(tokens, TokenType::OP_DIV, "/"); advance(); }
            else if (c == '=') { pushToken(tokens, TokenType::OP_ASSIGN, "="); advance(); }
            else if (c == '(') { pushToken(tokens, TokenType::SYM_LPAREN, "("); advance(); }
            else if (c == ')') { pushToken(tokens, TokenType::SYM_RPAREN, ")"); advance(); }

            else if (c == '"') {
                advance();
                std::string strVal;
                while (!isEnd() && peek() != '"') strVal += advance();
                if (!isEnd()) advance();
                else throw std::runtime_error("Unterminated string at line " + std::to_string(currentLine));
                pushToken(tokens, TokenType::STRING, std::move(strVal));
            }

            else if (std::isdigit(c)) {
                std::string num;
                while (!isEnd() && (std::isdigit(peek()) || peek() == '.')) num += advance();
                pushToken(tokens, TokenType::NUMBER, std::move(num));
            }

            else if (std::isalpha(c) || c == '_') {
                std::string id;
                while (!isEnd() && (std::isalnum(peek()) || peek() == '_')) id += advance();

                // 키워드 접두사 분리
                if (id.rfind("def_", 0) == 0 && id.length() > 4) {
                    pushToken(tokens, TokenType::KEYWORD_DEF, "def");
                    pushToken(tokens, TokenType::IDENTIFIER, id.substr(4));
                }
                else if (id.rfind("Variable_Change_", 0) == 0 && id.length() > 16) {
                    pushToken(tokens, TokenType::KEYWORD_VAR_CHANGE, "Variable_Change");
                    pushToken(tokens, TokenType::IDENTIFIER, id.substr(16));
                }
                else if (id == "print") {
                    pushToken(tokens, TokenType::KEYWORD_PRINT, id);
                }
                else if (id == "console") {
                    pushToken(tokens, TokenType::KEYWORD_CONSOLE, id);
                }
                // ★ 핵심 수정: x의 문맥 판단 ★
                else if (id == "x") {
                    // 이전 토큰이 식별자 또는 )이면 → 곱하기 연산자
                    // 이전 토큰이 ( 또는 연산자이면 → 변수명 x
                    if (lastTokenType == TokenType::IDENTIFIER ||
                        lastTokenType == TokenType::SYM_RPAREN ||
                        lastTokenType == TokenType::NUMBER) {
                        pushToken(tokens, TokenType::OP_MUL, "x");
                    } else {
                        pushToken(tokens, TokenType::IDENTIFIER, "x");
                    }
                }
                else {
                    pushToken(tokens, TokenType::IDENTIFIER, std::move(id));
                }
            }
            else {
                advance();
            }
        }
        pushToken(tokens, TokenType::EOF_TOKEN, "");
        return tokens;
    }
};

// =============================================================================
// 2. AST
// =============================================================================

using Value = std::variant<double, std::string>;

struct ASTNode { virtual ~ASTNode() = default; };
struct LiteralNode : public ASTNode { Value value; LiteralNode(Value v) : value(std::move(v)) {} };
struct IdentifierNode : public ASTNode { std::string name; IdentifierNode(std::string n) : name(std::move(n)) {} };
struct VarDefNode : public ASTNode { std::string name; std::unique_ptr<ASTNode> value; VarDefNode(std::string n, std::unique_ptr<ASTNode> v) : name(std::move(n)), value(std::move(v)) {} };
struct VarChangeNode : public ASTNode { std::string name; char op; std::unique_ptr<ASTNode> value; VarChangeNode(std::string n, char o, std::unique_ptr<ASTNode> v) : name(std::move(n)), op(o), value(std::move(v)) {} };
struct PrintNode : public ASTNode { std::unique_ptr<ASTNode> value; bool toConsole; PrintNode(std::unique_ptr<ASTNode> v, bool c) : value(std::move(v)), toConsole(c) {} };

// =============================================================================
// 3. PARSER
// =============================================================================

class Parser {
private:
    std::vector<Token>& tokens;
    size_t pos = 0;

    Token& current() { return tokens[pos]; }
    void advance() { if (pos < tokens.size() - 1) pos++; }
    bool check(TokenType t) { return current().type == t; }

    std::unique_ptr<ASTNode> parseExpression() {
        if (check(TokenType::NUMBER)) {
            double val = 0;
            try { val = std::stod(current().value); }
            catch (...) { throw std::runtime_error("Invalid number: " + current().value); }
            advance();
            return std::make_unique<LiteralNode>(val);
        }
        if (check(TokenType::STRING)) {
            std::string val = current().value; advance();
            return std::make_unique<LiteralNode>(std::move(val));
        }
        if (check(TokenType::IDENTIFIER)) {
            std::string name = current().value; advance();
            return std::make_unique<IdentifierNode>(std::move(name));
        }
        return nullptr;
    }

public:
    Parser(std::vector<Token>& toks) : tokens(toks) {}

    std::vector<std::unique_ptr<ASTNode>> parseProgram() {
        std::vector<std::unique_ptr<ASTNode>> program;

        while (!check(TokenType::EOF_TOKEN)) {

            if (check(TokenType::KEYWORD_DEF)) {
                advance();
                if (!check(TokenType::IDENTIFIER))
                    throw std::runtime_error("Expected variable name after def");
                std::string name = current().value; advance();
                if (check(TokenType::OP_ASSIGN)) advance();
                if (check(TokenType::SYM_LPAREN)) advance();
                auto val = parseExpression();
                if (check(TokenType::SYM_RPAREN)) advance();
                program.push_back(std::make_unique<VarDefNode>(std::move(name), std::move(val)));
            }

            else if (check(TokenType::KEYWORD_VAR_CHANGE)) {
                advance();
                if (!check(TokenType::IDENTIFIER))
                    throw std::runtime_error("Expected variable name after Variable_Change");
                std::string targetName = current().value; advance();

                char op = '+';
                if (check(TokenType::OP_PLUS)) op = '+';
                else if (check(TokenType::OP_MINUS)) op = '-';
                else if (check(TokenType::OP_MUL)) op = 'x';
                else if (check(TokenType::OP_DIV)) op = '/';
                else throw std::runtime_error("Expected operator after variable name");
                advance();

                if (check(TokenType::SYM_LPAREN)) advance();
                auto val = parseExpression();
                if (check(TokenType::SYM_RPAREN)) advance();
                program.push_back(std::make_unique<VarChangeNode>(std::move(targetName), op, std::move(val)));
            }

            else if (check(TokenType::KEYWORD_PRINT) || check(TokenType::KEYWORD_CONSOLE)) {
                bool toConsole = check(TokenType::KEYWORD_CONSOLE);
                advance();
                if (check(TokenType::SYM_LPAREN)) advance();
                auto val = parseExpression();
                if (check(TokenType::SYM_RPAREN)) advance();
                program.push_back(std::make_unique<PrintNode>(std::move(val), toConsole));
            }

            else {
                advance();
            }
        }
        return program;
    }
};

// =============================================================================
// 4. VIRTUAL MACHINE (큰 숫자 안전 처리)
// =============================================================================

class VM {
private:
    std::unordered_map<std::string, Value> variables;

    Value evaluate(ASTNode* node) {
        if (!node) return 0.0;
        if (auto* lit = dynamic_cast<LiteralNode*>(node)) return lit->value;
        if (auto* id = dynamic_cast<IdentifierNode*>(node)) {
            auto it = variables.find(id->name);
            if (it != variables.end()) return it->second;
            return 0.0;
        }
        return 0.0;
    }

    std::string valueToString(const Value& v) {
        if (std::holds_alternative<std::string>(v))
            return std::get<std::string>(v);
        if (std::holds_alternative<double>(v)) {
            double d = std::get<double>(v);
            // ★ 큰 숫자 안전 처리: long long 범위 내에서만 정수 변환 ★
            if (d == std::floor(d) && std::abs(d) < 9.0e18) {
                return std::to_string(static_cast<long long>(d));
            }
            std::ostringstream oss;
            oss << std::setprecision(10) << d;
            return oss.str();
        }
        return "";
    }

public:
    void execute(std::vector<std::unique_ptr<ASTNode>>& program) {
        for (const auto& node : program) {

            if (auto* def = dynamic_cast<VarDefNode*>(node.get())) {
                variables[def->name] = evaluate(def->value.get());
            }

            else if (auto* change = dynamic_cast<VarChangeNode*>(node.get())) {
                Value cur = variables.count(change->name) ? variables[change->name] : 0.0;
                Value add = evaluate(change->value.get());

                if (change->op == '+') {
                    if (std::holds_alternative<std::string>(cur) ||
                        std::holds_alternative<std::string>(add)) {
                        variables[change->name] = valueToString(cur) + valueToString(add);
                    } else {
                        variables[change->name] = std::get<double>(cur) + std::get<double>(add);
                    }
                } else {
                    double c = std::holds_alternative<double>(cur) ? std::get<double>(cur) : 0;
                    double a = std::holds_alternative<double>(add) ? std::get<double>(add) : 0;
                    double r = 0;
                    if (change->op == '-') r = c - a;
                    else if (change->op == 'x') r = c * a;
                    else if (change->op == '/') {
                        if (a == 0) throw std::runtime_error("Division by zero");
                        r = c / a;
                    }
                    variables[change->name] = r;
                }
            }

            else if (auto* p = dynamic_cast<PrintNode*>(node.get())) {
                std::string result = valueToString(evaluate(p->value.get()));
                if (p->toConsole)
                    std::cout << "[Console] " << result << std::endl;
                else
                    std::cout << "[Output] " << result << std::endl;
            }
        }
    }
};

// =============================================================================
// MAIN (모든 엣지 케이스 테스트)
// =============================================================================

int main() {
    std::string code = R"(
        # === Jwan Engine v1.5 Final Test ===

        # 기본 변수
        def_Name = ("Jun")
        def_Score = (10)

        # 문자열 결합
        Variable_Change_Name + ("_Dev")

        # 숫자 연산
        Variable_Change_Score + (5)
        Variable_Change_Score x (2)

        print(Name)
        print(Score)
        console(Score)

        # 변수명 x 테스트
        def_x = (42)
        print(x)

        # x를 곱하기로 사용
        def_Result = (3)
        Variable_Change_Result x (x)
        print(Result)
    )";

    try {
        Lexer lexer(code);
        auto tokens = lexer.tokenize();

        Parser parser(tokens);
        auto ast = parser.parseProgram();

        VM vm;
        vm.execute(ast);
    } catch (const std::exception& e) {
        std::cerr << "[Error] " << e.what() << std::endl;
    }

    return 0;
}
