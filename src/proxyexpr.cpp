#include <iomanip>
#include <iostream>

#include "proxyexpr.hpp"

ProxyExpr::ProxyExpr(const Expr *expr, Token::Loc loc)
    : Expr{loc, expr->type}, expr{expr}
{
}

ExprPtr
ProxyExpr::create(const Expr *expr, Token::Loc loc)
{
    auto p = new ProxyExpr{expr, loc};
    return std::unique_ptr<ProxyExpr>{p};
}

bool
ProxyExpr::hasAddr() const
{
    return expr->hasAddr();
}

bool
ProxyExpr::isLValue() const
{
    return expr->isLValue();
}

bool
ProxyExpr::isConst() const
{
    return expr->isConst();
}

// for code generation
gen::ConstVal
ProxyExpr::loadConstValue() const
{
    return expr->loadConstValue();
}

gen::Reg
ProxyExpr::loadValue() const
{
    return expr->loadConstValue();
}


gen::Reg
ProxyExpr::loadAddr() const
{
    return expr->loadAddr();
}

void
ProxyExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    expr->condJmp(trueLabel, falseLabel);
}

// for debugging and educational purposes
void
ProxyExpr::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    std::cerr << "proxy" << " [ " << type << " ] " << std::endl;
    expr->print(indent + 4);
}

