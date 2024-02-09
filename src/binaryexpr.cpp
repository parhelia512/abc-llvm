#include <cassert>
#include <iostream>
#include <iomanip>

#include "binaryexpr.hpp"
#include "error.hpp"
#include "identifier.hpp"
#include "promotion.hpp"

BinaryExpr::BinaryExpr(Kind kind, ExprPtr &&left, ExprPtr &&right,
		       const Type *type, Token::Loc loc)
    : Expr{loc, type}, kind{kind}, left{std::move(left)}
    , right{std::move(right)}
{
}

ExprPtr
BinaryExpr::create(Kind kind, ExprPtr &&left, ExprPtr &&right, Token::Loc loc)
{
    auto promotion = promotion::binary(kind, std::move(left),
				       std::move(right), &loc);
    auto p = new BinaryExpr{kind,
			    std::move(std::get<0>(promotion)),
			    std::move(std::get<1>(promotion)),
			    std::get<2>(promotion),
			    loc};
    return std::unique_ptr<BinaryExpr>{p};
}

bool
BinaryExpr::hasAddr() const
{
    return isLValue();
}

bool
BinaryExpr::isLValue() const
{
    return kind == MEMBER && right->isLValue();
}

//-- for checking constness 

bool
BinaryExpr::isIntegerConstExpr() const
{
    return type->isInteger()
	&& left->type->isInteger() && left->isConst()
	&& right->type->isInteger() && right->isConst();
}

bool
BinaryExpr::isArithmeticConstExpr() const
{
    return isIntegerConstExpr();
}

bool
BinaryExpr::isAddressConstant() const
{
    return type->isPointer() && left->isConst() && right->isConst();
}

bool
BinaryExpr::isConst() const
{
    switch (kind) {
	case CALL:
	case ASSIGN:
	    return false;
	default:
	    return isArithmeticConstExpr() || isAddressConstant();
    }
}

gen::ConstVal
BinaryExpr::loadConstValue() const
{
    assert(0);
    return nullptr;
}

// for code generation

static gen::AluOp getGenAluOp(BinaryExpr::Kind kind, const Type *type);
static gen::CondOp getGenCondOp(BinaryExpr::Kind kind, const Type *type);

gen::Reg
BinaryExpr::loadValue() const
{
    switch (kind) {
	case MEMBER:
	    return gen::fetch(loadAddr(), type);
	case ASSIGN:
	    return gen::store(right->loadValue(),
			      left->loadAddr(),
			      type);
	case ADD:
	case SUB:
	case MUL:
	case DIV:
	case MOD:
	    if (kind == ADD && type->isPointer()) {
		// pointer + integer
		assert(left->type->isPointer());
		assert(right->type->isInteger());
		return gen::ptrInc(left->type->getRefType(),
				   left->loadValue(),
				   right->loadValue());
	    } else if (kind == SUB && left->type->isPointer()) {
		// pointer - pointer
		assert(right->type->isPointer());
		assert(type->isInteger());
		return gen::ptrDiff(left->type->getRefType(),
				    left->loadValue(),
				    right->loadValue());
	    } else {
		assert(*left->type == *right->type);
		return gen::aluInstr(getGenAluOp(kind, type),
				     left->loadValue(),
				     right->loadValue());
	    }
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    assert(*left->type == *right->type);
	    return gen::cond(getGenCondOp(kind, left->type),
			     left->loadValue(),
			     right->loadValue());
	case LOGICAL_AND:
	case LOGICAL_OR:
	    {
		assert(*left->type == *right->type);

		auto trueLabel = gen::getLabel("true");
		auto falseLabel = gen::getLabel("false");
		auto phiLabel = gen::getLabel("true");

		condJmp(trueLabel, falseLabel);

		gen::labelDef(trueLabel);
		auto one = gen::loadIntConst(1, type);
		gen::jmp(phiLabel);

		gen::labelDef(falseLabel);
		auto zero = gen::loadIntConst(0, type);
		gen::jmp(phiLabel);

		gen::labelDef(phiLabel);
		return gen::phi(one, trueLabel, zero, falseLabel, type);
	    }
	    
	case CALL:
	default:
	    error::out() << "kind = " << int(kind) << std::endl;
	    assert(0);
	    return nullptr;
    }
}

gen::Reg
BinaryExpr::loadAddr() const
{
    assert(hasAddr());
    assert(kind == MEMBER);
    auto ident = dynamic_cast<const Identifier *>(right.get())->ident;
    return gen::ptrMember(left->type,
			  left->loadAddr(),
			  left->type->getMemberIndex(ident));
}

void
BinaryExpr::condJmp(gen::Label trueLabel, gen::Label falseLabel) const
{
    switch (kind) {
	case LESS:
	case LESS_EQUAL:
	case GREATER:
	case GREATER_EQUAL:
	case NOT_EQUAL:
	case EQUAL:
	    {
		assert(*left->type == *right->type);
		auto cond = gen::cond(getGenCondOp(kind, left->type),
				      left->loadValue(),
				      right->loadValue());
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
	case LOGICAL_AND:
            {
		auto chkRightLabel = gen::getLabel("chkRight");

		left->condJmp(chkRightLabel, falseLabel);
		gen::labelDef(chkRightLabel);
		right->condJmp(trueLabel, falseLabel);
		return;
            }
	case LOGICAL_OR:
            {
		auto chkRightLabel = gen::getLabel("chkRight");
		
		left->condJmp(trueLabel, chkRightLabel);
		gen::labelDef(chkRightLabel);
		right->condJmp(trueLabel, falseLabel);
		return;
            }
	default:
	    {
		auto zero = gen::loadZero(type);
		auto cond = gen::cond(gen::NE, loadValue(), zero);
		gen::jmp(cond, trueLabel, falseLabel);
		return;
	    }
    }
}

// for debugging and educational purposes
static const char *kindStr(BinaryExpr::Kind kind);

void
BinaryExpr::print(int indent) const
{
    if (indent) {
	std::cerr << std::setfill(' ') << std::setw(indent) << ' ';
    }
    std::cerr << kindStr(kind) << " [ " << type << " ] " << std::endl;
    left->print(indent + 4);
    right->print(indent + 4);
}

/*
 * Auxiliary functions
 */

static gen::AluOp
getGenAluOp(BinaryExpr::Kind kind, const Type *type)
{
    bool isSignedInt = type->isInteger()
		    && type->getIntegerKind() == Type::SIGNED;
    switch (kind) {
	case BinaryExpr::Kind::ADD: return gen::ADD;
	case BinaryExpr::Kind::SUB: return gen::SUB;
	case BinaryExpr::Kind::MUL: return gen::SMUL;
	case BinaryExpr::Kind::DIV: return isSignedInt
					? gen::SDIV
					: gen::UDIV;
	case BinaryExpr::Kind::MOD: return isSignedInt
					? gen::SMOD
					: gen::UMOD;
	default:
	    assert(0);
	    return gen::ADD;
    }
}

static gen::CondOp
getGenCondOp(BinaryExpr::Kind kind, const Type *type)
{
    bool isSignedInt = type->isInteger()
		    && type->getIntegerKind() == Type::SIGNED;
    switch (kind) {
	case BinaryExpr::Kind::EQUAL:
	    return gen::EQ;
	case BinaryExpr::Kind::NOT_EQUAL:
	    return gen::NE;
	case BinaryExpr::Kind::LESS:
	    return isSignedInt ? gen::SLT : gen::ULT;
	case BinaryExpr::Kind::LESS_EQUAL:
	    return isSignedInt ? gen::SLE : gen::ULE;
	case BinaryExpr::Kind::GREATER:
	    return isSignedInt ? gen::SGT : gen::UGT;
	case BinaryExpr::Kind::GREATER_EQUAL:
	    return isSignedInt ? gen::SGE : gen::UGE;
	case BinaryExpr::Kind::LOGICAL_AND:
	    return gen::AND;
	case BinaryExpr::Kind::LOGICAL_OR:
	    return gen::OR;
	default:
	    std::cerr << "Not handled: " << int(kind) << std::endl;
	    assert(0);
	    return gen::EQ;
    }
}

static const char *
kindStr(BinaryExpr::Kind kind)
{
    switch (kind) {
	case BinaryExpr::CALL: return "call";
	case BinaryExpr::ADD: return "+";
	case BinaryExpr::ASSIGN: return "=";
	case BinaryExpr::EQUAL: return "==";
	case BinaryExpr::NOT_EQUAL: return "!=";
	case BinaryExpr::GREATER: return ">";
	case BinaryExpr::GREATER_EQUAL: return ">=";
	case BinaryExpr::LESS: return "<";
	case BinaryExpr::LESS_EQUAL: return "<=";
	case BinaryExpr::LOGICAL_AND: return "&&";
	case BinaryExpr::LOGICAL_OR: return "||";
	case BinaryExpr::SUB: return "-";
	case BinaryExpr::MUL: return "*";
	case BinaryExpr::DIV: return "/";
	case BinaryExpr::MOD: return "%";
	case BinaryExpr::MEMBER: return ".member";
	default: return "?? (binary)";
    }
}
