#ifndef EXPR_HPP
#define EXPR_HPP

#include <charconv>
#include <cstdint>
#include <memory>
#include <string>

#include "gen.hpp"
#include "type.hpp"

class Expr;
struct ExprDeleter
{
    void
    operator()(const Expr *expr) const;
};

using ExprPtr = std::unique_ptr<const Expr, ExprDeleter>;
using ExprVector = std::vector<ExprPtr>;
using ExprVectorPtr = std::unique_ptr<ExprVector>;

struct Literal
{
    const char *val;
    const Type *type;
    std::uint8_t radix;

    Literal(const char *val, const Type *type, std::uint8_t radix)
	: val{val}, type{type}, radix{radix} {}
};

struct Identifier
{
    const char *val;
    const Type *type;

    Identifier(const char *val, const Type *type) : val{val}, type{type} {}
};

struct Proxy
{
    const Expr *expr;

    Proxy(const Expr *expr) : expr{expr} {}
};

struct Unary
{
    enum Kind
    {
	ADDRESS,
	DEREF,
	CAST,
	LOGICAL_NOT,
    };

    Kind kind;
    ExprPtr child;
    const Type *type;

    Unary(Kind kind, ExprPtr &&child, const Type *type)
	: kind{kind}, child{std::move(child)}, type{type}
    {}

    const Type * getType(void);
};

struct Binary
{
    enum Kind
    {
	CALL,
	ADD,
	ASSIGN,
	EQUAL,
	NOT_EQUAL,
	GREATER,
	GREATER_EQUAL,
	LESS,
	LESS_EQUAL,
	LOGICAL_AND,
	LOGICAL_OR,
	SUB,
	MUL,
	DIV,
	MOD,
	POSTFIX_INC,
	POSTFIX_DEC,
    };

    Kind kind;
    ExprPtr left, right;
    const Type *type;

    Binary(Kind kind, ExprPtr &&left, ExprPtr &&right)
	: kind{kind}, left{std::move(left)}, right{std::move(right)}
	, type{nullptr}
    {
	setType();
	//assert(type && "illegal expression"); // TODO: better error handling
	castOperands();
    }

    void setType(void);
    void castOperands(void);
    
};

struct Conditional
{
    ExprPtr cond, left, right;
    const Type *type;

    Conditional(ExprPtr &&cond, ExprPtr &&left, ExprPtr &&right)
	: cond{std::move(cond)}, left{std::move(left)}, right{std::move(right)}
    {
	setTypeAndCastOperands();
    }

    void setTypeAndCastOperands(void);
};

class Expr
{
    public:
	std::variant<Literal, Identifier, Proxy,  Unary, Binary, Conditional,
		     ExprVector> variant;

    private:
	Expr(Literal &&val) : variant{std::move(val)} {}
	Expr(Identifier &&ident) : variant{std::move(ident)} {}
	Expr(Proxy &&proxy) : variant{std::move(proxy)} {}
	Expr(Unary &&unary) : variant{std::move(unary)} {}
	Expr(Binary &&binary) : variant{std::move(binary)} {}
	Expr(Conditional &&con) : variant{std::move(con)} {}
	Expr(ExprVector &&vec) : variant{std::move(vec)} {}

    public:
	static ExprPtr createLiteral(const char *val, std::uint8_t radix,
				     const Type *type = nullptr);
	static ExprPtr createIdentifier(const char *ident, const Type *type);
	static ExprPtr createProxy(const Expr *expr);
	static ExprPtr createUnaryMinus(ExprPtr &&expr);
	static ExprPtr createLogicalNot(ExprPtr &&expr);
	static ExprPtr createAddr(ExprPtr &&expr);
	static ExprPtr createDeref(ExprPtr &&expr);
	static ExprPtr createCast(ExprPtr &&child, const Type *toType);
	static ExprPtr createBinary(Binary::Kind kind,
				    ExprPtr &&left, ExprPtr &&right);
	static ExprPtr createCall(ExprPtr &&fn, ExprVector &&param);
	static ExprPtr createConditional(ExprPtr &&cond,
					 ExprPtr &&left, ExprPtr &&right);
	static ExprPtr createExprVector(ExprVector &&expr);

	const Type *getType(void) const;
	bool isLValue(void) const;
	bool isConst(void) const;

	void print(int indent = 0) const;

	template <typename T>
	T constValue(void) const;

	// code generation
	gen::ConstVal loadConst(void) const;
	gen::Reg loadValue(void) const;
	gen::Reg loadAddr(void) const;
	void condJmp(gen::Label trueLabel, gen::Label falseLabel) const;
};

template <typename T>
T
Expr::constValue(void) const
{
    assert(isConst());

    if (std::holds_alternative<Proxy>(variant)) {
	return std::get<Proxy>(variant).expr->constValue<T>();
    } else if (std::holds_alternative<Literal>(variant)) {
	const auto &lit = std::get<Literal>(variant);
	T result;
	auto [ptr, ec] = std::from_chars(
			    lit.val, lit.val + strlen(lit.val),
			    result, lit.radix);
	assert(ec == std::errc{});
	return result;
    } else if (std::holds_alternative<Unary>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<Binary>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<Conditional>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else if (std::holds_alternative<ExprVector>(variant)) {
	assert(0 && "sorry, currently not implemented");
	return 0;
    } else {
	std::cerr << "not handled variant. Index = " << variant.index()
	    << std::endl;
	assert(0);
	return 0;
    }
}

#endif // EXPR_HPP
