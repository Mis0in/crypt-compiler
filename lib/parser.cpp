#include "parser.h"
#include <fstream>
#include <map>
#include <cassert>
#include <array>


namespace {
    using namespace AST;
    using namespace Parser;
    std::vector<int> line_breaks;

    //wrapper for keeping cursor position and error messages
    struct SimpleStream {
        SimpleStream(std::istream &in) : in(in) {}

        int get() {
            int c = in.get();
            if (c == '\n') {
                line_breaks.push_back(in.tellg());
            }
            return c;
        }

    private:
        std::istream &in;
    };

    std::string temp_identifier;
    int cur_char = ' ';
    int cur_token = 0;
    std::istream NULL_STREAM(nullptr);
    SimpleStream in(NULL_STREAM);
    void *m_stream_ptr = &in;

//-----------------LEXER----------------------
    int helper_return_char(int TOKEN) {
        cur_char = in.get();
        return cur_token = TOKEN;
    }

    int get_tok(int token_info = ANY_TOKEN_EXPECTED) {
        while (cur_char == ' ') cur_char = in.get();
        if (token_info & VALUE_EXPECTED) {
            if (isdigit(cur_char)) {
                temp_identifier.clear();
                do {
                    temp_identifier.push_back(static_cast<char>(cur_char));
                    cur_char = in.get();
                } while (isdigit(cur_char));
                return cur_token = TOKEN_INT_LIT;
            }
            if (isalpha(cur_char) || cur_char == '_') {
                temp_identifier.clear();
                do {
                    temp_identifier.push_back(static_cast<char>(cur_char));
                    cur_char = in.get();
                } while (isalnum(cur_char));
                return cur_token = TOKEN_IDENTIFIER;
            }
            if (cur_char == '(') return helper_return_char(TOKEN_LPAREN);
            if (cur_char == ')') return helper_return_char(TOKEN_RPAREN);
        }
        if (token_info & OPERATOR_EXPECTED) {
            if (cur_char == -1) return cur_token = TOKEN_EOF;
            switch (cur_char) {
                case '+':
                    return helper_return_char(TOKEN_ADD);
                case '-':
                    return helper_return_char(TOKEN_SUB);
                case '*':
                    return helper_return_char(TOKEN_MUL);
                case '/':
                    return helper_return_char(TOKEN_DIV);
            }
        }
        throw std::runtime_error("Unknown token");
        return 0;
    }

//-----------------LEXER----------------------
    unique_ptr<Node> parse_precedence(int prec);

    std::unique_ptr<Node> pfn_identifier() { return std::make_unique<VarExpr>(temp_identifier); }

    std::unique_ptr<Node> pfn_number() { return std::make_unique<IntLitExpr>(atoi(temp_identifier.c_str())); }

    std::unique_ptr<Node> pfn_grouping() {
        auto expr = parse_expression();
        if (cur_token != TOKEN_RPAREN) throw std::runtime_error("Expected RPAREN");
        return expr;
    }

    std::unique_ptr<Node> ifn_binary(std::unique_ptr<Node> lhs, int op);

// clang-format off
// @formatter:off
    RuleInfo rules[] = {
    /* TOKEN_EOF */         {nullptr,        nullptr,    PREC_NONE},
    /* TOKEN_ADD */         {nullptr,        ifn_binary, PREC_ADD},
    /* TOKEN_SUB */         {nullptr,        ifn_binary, PREC_ADD},
    /* TOKEN_MUL */         {nullptr,        ifn_binary, PREC_FACTOR},
    /* TOKEN_DIV */         {nullptr,        ifn_binary, PREC_FACTOR},
    /* TOKEN_IDENTIFIER */  {pfn_identifier, nullptr,    PREC_PRIMARY},
    /* TOKEN_INT_LIT */     {pfn_number,     nullptr,    PREC_PRIMARY},
    /* TOKEN_LPAREN */      {pfn_grouping,   nullptr,    PREC_CALL},
    };
// clang-format on
// @formatter:on

    std::unique_ptr<Node> ifn_binary(std::unique_ptr<Node> lhs, int op) {
        auto rhs = parse_precedence(rules[op].precedence + 1);//because we want all operators to be left-assoc(if right-assoc needed just do not add 1)
        switch (op) {
            case TOKEN_ADD:
                return std::make_unique<BinaryExpr<BinaryOpType::ADD>>(std::move(lhs), std::move(rhs));
            case TOKEN_SUB:
                return std::make_unique<BinaryExpr<BinaryOpType::SUB>>(std::move(lhs), std::move(rhs));
            case TOKEN_MUL:
                return std::make_unique<BinaryExpr<BinaryOpType::MUL>>(std::move(lhs), std::move(rhs));
            case TOKEN_DIV:
                return std::make_unique<BinaryExpr<BinaryOpType::DIV>>(std::move(lhs), std::move(rhs));
            default:
                throw std::runtime_error("Unknown error");
        }
    }

    unique_ptr<Node> parse_precedence(int prec) {
        if (rules[cur_token].prefix == nullptr)
            throw std::runtime_error("should not be null here");
        auto lhs = rules[cur_token].prefix();
        int op = get_tok(OPERATOR_EXPECTED);
        while (prec <= rules[op].precedence) {
            get_tok();
            lhs = rules[op].infix(std::move(lhs), op);
            op = cur_token;
        }
        return lhs;
    }
}
namespace Parser {
    unique_ptr<Node> parse_expression() {
        return parse_precedence(PREC_ASSIGN);
    }

    std::unique_ptr<Node> parse(std::istream &input_stream) {
        in.~SimpleStream();
        new(m_stream_ptr) SimpleStream(input_stream);
        cur_char = ' ';
        get_tok();
        auto temp = parse_expression();
        return temp;
    }
}

//TODO: panic on error and output many errors