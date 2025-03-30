//
// Created by motya on 30.03.2025.
//
#include "ast.h"

namespace ast {

std::string Node::to_str1() {
    throw std::runtime_error("shouldn't be called");
}

std::string IntLitExpr::to_str1() {
    return std::to_string(number);
}

std::string VarExpr::to_str1() {
    return name;
}
}
