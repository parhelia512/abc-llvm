#ifndef ASSERTHACK_HPP
#define ASSERTHACK_HPP

#include "expr.hpp"

namespace asserthack {

extern UStr assertIdent;
void makeDecl();
ExprPtr createCall(ExprPtr &&expr, Token::Loc loc);

} // namespace asserthack {


#endif // ASSERTHACK_HPP
