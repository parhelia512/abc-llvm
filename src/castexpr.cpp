#include <iomanip>
#include <iostream>

#include "castexpr.hpp"

CastExpr::CastExpr(ExprPtr &&expr, const Type *toType, Token::Loc loc)
    : Expr{loc, toType}, expr{std::move(expr)}
{
}

ExprPtr
CastExpr::create(ExprPtr &&expr, const Type *toType, Token::Loc loc)
{
    auto p = new CastExpr{std::move(expr), toType, loc};
    return std::unique_ptr<CastExpr>{p};
}

ExprPtr
CastExpr::create(ExprPtr &&expr, const Type *toType)
{
    auto loc = expr->loc;
    return create(std::move(expr), toType, loc);
}


bool
CastExpr::hasAddr() const
{
    return false;
}

bool
CastExpr::isLValue() const
{
    return false;
}

bool
CastExpr::isConst() const
{
    return expr->isConst();
}

// for code generation
gen::ConstVal
CastExpr::loadConstValue() const
{
    if (type->isBool()) {
	auto zero = gen::loadZero(expr->type);
	return gen::cond(gen::NE, expr->loadConstValue(), zero);
    }
    return gen::cast(expr->loadConstValue(), expr->type, type);
}

gen::Reg
CastExpr::loadValue() const
{
    if (type->isBool()) {
	auto zero = gen::loadZero(expr->type);
	return gen::cond(gen::NE, expr->loadValue(), zero);
    }
    return gen::cast(expr->loadValue(), expr->type, type);
}


gen::Reg
CastExpr::loadAddr() const
{
    assert(0 && "IntegerLiteral has no address");
    return nullptr;
}

void
CastExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    auto zero = gen::loadZero(type);
    auto cond = gen::cond(gen::NE, loadValue(), zero);
    gen::jmp(cond, trueLabel, falseLabel);
}

// for debugging and educational purposes
void
CastExpr::print(int indent) const
{
    std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    std::cerr << "cast" << " [ " << type << " ] " << std::endl;
    expr->print(indent + 4);
}

