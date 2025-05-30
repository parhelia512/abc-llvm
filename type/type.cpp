#include <algorithm>
#include <cassert>
#include <iostream>

#include "arraytype.hpp"
#include "integertype.hpp"
#include "pointertype.hpp"
#include "type.hpp"
#include "typealias.hpp"

namespace abc {

Type::Type(bool isConst, UStr name)
  : isConst{ isConst }
  , name{ name }
{
}

bool
Type::equals(const Type *ty1, const Type *ty2)
{
    if (ty1->hasConstFlag() != ty2->hasConstFlag()) {
        return false;
    }
    if (ty1->isVoid() && ty2->isVoid()) {
        return true;
    } else if (ty1->isInteger() && ty2->isInteger()) {
        return ty1->isSignedInteger() == ty2->isSignedInteger() &&
               ty1->numBits() == ty2->numBits();
    } else if (ty1->isFloatType() && ty2->isFloatType()) {
        return (ty1->isFloat() && ty2->isFloat()) ||
               (ty1->isDouble() && ty2->isDouble());
    } else if (ty1->isPointer() && ty2->isPointer()) {
        if (ty1->isNullptr() || ty2->isNullptr()) {
            return ty1->isNullptr() == ty2->isNullptr();
        } else {
            return equals(ty1->refType(), ty2->refType());
        }
    } else if (ty1->isStruct() && ty2->isStruct()) {
        return ty1->id() == ty2->id();
    } else if (ty1->isArray() && ty2->isArray()) {
        return ty1->dim() == ty2->dim() &&
               equals(ty1->refType(), ty2->refType());
    } else if (ty1->isFunction() && ty2->isFunction()) {
        if (!equals(ty1->retType(), ty2->retType())) {
            return false;
        }
        if (ty1->hasVarg() != ty2->hasVarg()) {
            return false;
        }
        const auto &paramType1 = ty1->paramType();
        const auto &paramType2 = ty2->paramType();
        if (paramType1.size() != paramType2.size()) {
            return false;
        }
        for (std::size_t i = 0; i < paramType1.size(); ++i) {
            if (!equals(paramType1[i], paramType2[i])) {
                return false;
            }
        }
        return true;
    }
    return false;
}

const Type *
Type::common(const Type *ty1, const Type *ty2)
{
    assert(ty1);
    assert(ty2);

    // if types are integer and floating point always have float type in ty1
    if (ty1->isInteger() && ty2->isFloatType()) {
        std::swap(ty1, ty2);
    }

    const Type *common = nullptr;
    if (equals(ty1->getConstRemoved(), ty2->getConstRemoved())) {
        common = ty1;
    } else if (ty1->isArray() && ty2->isArray()) {
        // both types are arrays but refernce type or dimension are different
        if (equals(ty1->refType(), ty2->refType())) {
            common = PointerType::create(ty1->refType());
        }
    } else if (ty1->isFloatType() && ty2->isInteger()) {
        common = ty1;
    } else if (ty1->isInteger() && ty2->isInteger()) {
        auto size = std::max(ty1->numBits(), ty2->numBits());
        if (ty1->isUnsignedInteger() || ty2->isUnsignedInteger()) {
            common = IntegerType::createUnsigned(size);
        } else {
            common = IntegerType::createSigned(size);
        }
    } else if (ty1->isPointer() && ty2->isNullptr()) {
        common = ty1;
    } else if (ty1->isNullptr() && ty2->isPointer()) {
        common = ty2;
    }
    if (common && (ty1->hasConstFlag() || ty2->hasConstFlag())) {
        common->getConst();
    }
    return common;
}

bool
Type::assignable(const Type *type)
{
    if (type->isArray()) {
        return assignable(type->refType());
    } else {
        return !type->hasConstFlag();
    }
}

static const Type *
convert(const Type *from, const Type *to, bool checkConst)
{
    if (checkConst && from->hasConstFlag() && !to->hasConstFlag()) {
        return nullptr;
    }
    from = from->getConstRemoved();
    to = to->getConstRemoved();
    if (Type::equals(from, to)) {
        return to;
    } else if (to->isBool()) {
        if (from->isInteger()) {
            return to;
        } else if (from->isPointer()) {
            return to;
        } else {
            return nullptr;
        }
    } else if (to->isFloatType()) {
        if (from->isInteger() || from->isFloatType()) {
            return to;
        } else {
            return nullptr;
        }
    } else if (to->isInteger()) {
        if (from->isInteger() || from->isFloatType()) {
            return to;
        } else {
            return nullptr;
        }
    } else if (to->isPointer() && from->isArray()) {
        assert(!to->isNullptr());
        if (from->isNullptr()) {
            return to;
        }
        if (convert(from->refType(), to->refType(), true)) {
            return to;
        } else {
            return nullptr;
        }
    } else if (to->isPointer() && from->isPointer()) {
        assert(!to->isNullptr());
        if (from->isNullptr()) {
            return to;
        }
        if (from->refType()->isVoid() || to->refType()->isVoid()) {
            return to;
        }
        if (convert(from->refType(), to->refType(), true)) {
            auto fromRefTy = from->refType()->getConstRemoved();
            auto toRefTy = to->refType()->getConstRemoved();
            if (Type::equals(fromRefTy, toRefTy)) {
                return to;
            } else {
                return nullptr;
            }
        } else {
            return nullptr;
        }
    } else if (from->isStruct() && to->isStruct()) {
        if (Type::equals(from, to)) {
            return to;
        } else {
            return nullptr;
        }
    } else if (to->isArray() && from->isArray()) {
        if (to->dim() != from->dim() && !to->isUnboundArray()) {
            return nullptr;
        }
        if (convert(from->refType(), to->refType(), checkConst)) {
            return to;
        } else {
            return nullptr;
        }
    } else {
        return nullptr;
    }
}

const Type *
Type::convert(const Type *from, const Type *to)
{
    return abc::convert(from, to, false);
}

const Type *
Type::explicitCast(const Type *from, const Type *to)
{
    if (auto type = convert(from->getConstRemoved(), to->getConstRemoved())) {
        // allow const-casts
        return type;
    } else if (from->isPointer() && to->isPointer()) {
        return to;
    } else {
        return nullptr;
    }
}

UStr
Type::ustr() const
{
    return name;
}

std::size_t
Type::id() const
{
    return isAlias() ? getUnalias()->id() : 0;
}

bool
Type::hasConstFlag() const
{
    return isAlias() ? getUnalias()->isConst : isConst;
}

bool
Type::isScalar() const
{
    return !isArray() && !isStruct();
}

std::size_t
Type::aggregateSize() const
{
    if (isScalar()) {
        return 1;
    } else if (isArray()) {
        return dim();
    } else {
        assert(0);
        return 0;
    }
}

const Type *
Type::aggregateType(std::size_t index) const
{
    assert(isUnboundArray() || index < aggregateSize());

    if (isScalar()) {
        return this;
    } else if (isArray()) {
        return refType();
    } else {
        assert(0);
        return nullptr;
    }
}

// for type aliases
bool
Type::isAlias() const
{
    return getUnalias();
}

const Type *
Type::getUnalias() const
{
    return nullptr;
}

const Type *
Type::getAlias(UStr alias) const
{
    return TypeAlias::create(alias, this);
}

const Type *
Type::getAlias(const char *alias) const
{
    return TypeAlias::create(UStr::create(alias), this);
}

// ---

bool
Type::hasSize() const
{
    return isAlias() ? getUnalias()->hasSize() : true;
}

bool
Type::isAuto() const
{
    return isAlias() ? getUnalias()->isAuto() : false;
}

bool
Type::isVoid() const
{
    return isAlias() ? getUnalias()->isVoid() : false;
}

bool
Type::isBool() const
{
    return isAlias() ? getUnalias()->isBool() : false;
}

bool
Type::isNullptr() const
{
    return isAlias() ? getUnalias()->isNullptr() : false;
}

// for integer (sub-)types
bool
Type::isInteger() const
{
    return isAlias() ? getUnalias()->isInteger() : false;
}

bool
Type::isSignedInteger() const
{
    return isAlias() ? getUnalias()->isSignedInteger() : false;
}

bool
Type::isUnsignedInteger() const
{
    return isAlias() ? getUnalias()->isUnsignedInteger() : false;
}

std::size_t
Type::numBits() const
{
    return isAlias() ? getUnalias()->numBits() : 0;
}

// for floating point type (sub-)types
bool
Type::isFloatType() const
{
    return isAlias() ? getUnalias()->isFloatType() : false;
}

bool
Type::isFloat() const
{
    return isAlias() ? getUnalias()->isFloat() : false;
}

bool
Type::isDouble() const
{
    return isAlias() ? getUnalias()->isDouble() : false;
}

// for pointer and array (sub-)types
bool
Type::isPointer() const
{
    return isAlias() ? getUnalias()->isPointer() : false;
}

bool
Type::isArray() const
{
    return isAlias() ? getUnalias()->isArray() : false;
}

bool
Type::isUnboundArray() const
{
    if (isArray() && dim() == 0) {
        return true;
    }
    return false;
}

const Type *
Type::refType() const
{
    return isAlias() ? getUnalias()->refType() : nullptr;
}

std::size_t
Type::dim() const
{
    return isAlias() ? getUnalias()->dim() : 0;
}

const Type *
Type::patchUnboundArray(const Type *type, std::size_t dim)
{
    assert(type);
    if (type->isUnboundArray()) {
        return ArrayType::create(type->refType(), dim);
    } else {
        return type;
    }
}

// for function (sub-)types
bool
Type::isFunction() const
{
    return isAlias() ? getUnalias()->isFunction() : false;
}

const Type *
Type::retType() const
{
    return isAlias() ? getUnalias()->retType() : nullptr;
}

bool
Type::hasVarg() const
{
    return isAlias() ? getUnalias()->hasVarg() : false;
}

const std::vector<const Type *> &
Type::paramType() const
{
    static std::vector<const Type *> noArgs;
    return isAlias() ? getUnalias()->paramType() : noArgs;
}

// for enum (sub-)types
bool
Type::isEnum() const
{
    return isAlias() ? getUnalias()->isEnum() : false;
}

const Type *
Type::complete(std::vector<UStr> &&, std::vector<std::int64_t> &&)
{
    if (isAlias() && getUnalias()->isEnum()) {
        assert(0 && "Alias type can not be completed");
    }
    assert(0 && "Type can not be completed");
    return nullptr;
}

// for struct (sub-)types
bool
Type::isStruct() const
{
    return isAlias() ? getUnalias()->isStruct() : false;
}

const Type *
Type::complete(std::vector<UStr> &&, std::vector<std::size_t> &&,
               std::vector<const Type *> &&)
{
    if (isAlias() && getUnalias()->isStruct()) {
        assert(0 && "Alias type can not be completed");
    }
    assert(0 && "Type can not be completed");
    return nullptr;
}

const std::vector<UStr> &
Type::memberName() const
{
    static std::vector<UStr> noMembers;
    return isAlias() ? getUnalias()->memberName() : noMembers;
}

const std::vector<std::size_t> &
Type::memberIndex() const
{
    static std::vector<std::size_t> noMembers;
    return isAlias() ? getUnalias()->memberIndex() : noMembers;
}

std::optional<std::size_t>
Type::memberIndex(UStr name) const
{
    return isAlias() ? getUnalias()->memberIndex(name) : std::nullopt;
}

const std::vector<const Type *> &
Type::memberType() const
{
    static std::vector<const Type *> noMembers;
    return isAlias() ? getUnalias()->memberType() : noMembers;
}

const Type *
Type::memberType(UStr name) const
{
    return isAlias() ? getUnalias()->memberType(name) : nullptr;
}

std::ostream &
operator<<(std::ostream &out, const Type *type)
{
    const char *constFlag = type->isConst ? "readonly " : "";

    out << constFlag << type->ustr();
    return out;
}

} // namespace abc
