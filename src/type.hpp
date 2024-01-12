#ifndef TYPE_HPP
#define TYPE_HPP

#include <cstddef>
#include <cassert>
#include <variant>
#include <vector>
#include <ostream>

class Type
{
    public:
	enum Id {
	    VOID,
	    INTEGER,
	    POINTER,
	    ARRAY,
	    FUNCTION,
	} id;

	bool isVoid() const
	{
	    return id == VOID;
	}

	// for integer (sub-)types 

	enum IntegerKind {
	    SIGNED,
	    UNSIGNED,
	};

	bool isInteger() const
	{
	    return id == INTEGER;
	}

	IntegerKind getIntegerKind() const
	{
	    return std::get<IntegerData>(data).kind;
	}

	std::size_t getIntegerNumBits() const
	{
	    assert(std::holds_alternative<IntegerData>(data));
	    return std::get<IntegerData>(data).numBits;
	}

	// for pointer and array (sub-)types

	bool isPointer() const
	{
	    return id == POINTER;
	}

	bool isArray() const
	{
	    return id == ARRAY;
	}

	const Type *getRefType() const
	{
	    if (std::holds_alternative<ArrayData>(data)) {
		return std::get<ArrayData>(data).refType;
	    }
	    assert(std::holds_alternative<PointerData>(data));
	    return std::get<PointerData>(data).refType;
	}

	std::size_t getDim() const
	{
	    assert(std::holds_alternative<ArrayData>(data));
	    return std::get<ArrayData>(data).dim;
	}

	// for function (sub-)types 
	bool isFunction() const
	{
	    return id == FUNCTION;
	}

	const Type *getRetType() const
	{
	    assert(std::holds_alternative<FunctionData>(data));
	    return std::get<FunctionData>(data).retType;
	}

	const std::vector<const Type *> &getArgType() const
	{
	    assert(std::holds_alternative<FunctionData>(data));
	    return std::get<FunctionData>(data).argType;
	}

    protected:
	struct IntegerData {
	    std::size_t numBits;
	    IntegerKind kind;
	};

	struct PointerData {
	    const Type *refType;
	};

	struct ArrayData {
	    const Type *refType;
	    std::size_t dim;
	};

	struct FunctionData {
	    const Type *retType;
	    std::vector<const Type *> argType;
	};

	std::variant<IntegerData, PointerData, ArrayData, FunctionData> data;

	template <typename Data>
	Type(Id id, Data &&data) : id{id}, data{data} {}

    public:
	static const Type *getVoid(void);
	static const Type *getUnsignedInteger(std::size_t numBits);
	static const Type *getSignedInteger(std::size_t numBits);
	static const Type *getPointer(const Type *refType);
	static const Type *getArray(const Type *refType, std::size_t dim);
	static const Type *getFunction(const Type *retType,
				       std::vector<const Type *> argType);

	// TODO: provide bool type
	// TODO: provide void type? (currently nullptr represents void)
};

std::ostream &operator<<(std::ostream &out, const Type *type);

#endif // TYPE_HPP
