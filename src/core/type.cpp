#include "core/type.h"
#include <sstream>

namespace coil {

Type::Type(uint16_t tId, uint32_t tSize, uint32_t tAlign, uint16_t tFlags, 
           const std::string& tName)
    : typeId(tId), flags(tFlags), size(tSize), alignment(tAlign), name(tName) {
}

uint16_t Type::getTypeId() const {
    return typeId;
}

uint16_t Type::getFlags() const {
    return flags;
}

uint32_t Type::getSize() const {
    return size;
}

uint32_t Type::getAlignment() const {
    return alignment;
}

const std::string& Type::getName() const {
    return name;
}

void Type::setName(const std::string& tName) {
    name = tName;
}

bool Type::isPointer() const {
    return (typeId & TYPE_PTR) != 0;
}

bool Type::isVector() const {
    return (typeId & 0xF0) == 0x20; // Vector mask
}

uint16_t Type::getBaseType() const {
    if (isPointer()) {
        return typeId & 0x000F; // Lower 4 bits
    } else if (isVector()) {
        return typeId & 0x000F; // Lower 4 bits
    }
    return 0;
}

std::string Type::toString() const {
    if (!name.empty()) {
        return name;
    }
    
    std::ostringstream oss;
    
    if (isPointer()) {
        oss << "ptr(" << Type::fromBasicType(getBaseType()).toString() << ")";
    } else if (isVector()) {
        switch (typeId & 0x00F0) {
            case TYPE_VEC128: oss << "vec128("; break;
            case TYPE_VEC256: oss << "vec256("; break;
            case TYPE_VEC512: oss << "vec512("; break;
            default: oss << "vec("; break;
        }
        oss << Type::fromBasicType(getBaseType()).toString() << ")";
    } else {
        switch (typeId) {
            case TYPE_VOID:   oss << "void"; break;
            case TYPE_INT8:   oss << "int8"; break;
            case TYPE_INT16:  oss << "int16"; break;
            case TYPE_INT32:  oss << "int32"; break;
            case TYPE_INT64:  oss << "int64"; break;
            case TYPE_INT128: oss << "int128"; break;
            case TYPE_UINT8:  oss << "uint8"; break;
            case TYPE_UINT16: oss << "uint16"; break;
            case TYPE_UINT32: oss << "uint32"; break;
            case TYPE_UINT64: oss << "uint64"; break;
            case TYPE_UINT128: oss << "uint128"; break;
            case TYPE_FP16:   oss << "fp16"; break;
            case TYPE_FP32:   oss << "fp32"; break;
            case TYPE_FP64:   oss << "fp64"; break;
            case TYPE_FP80:   oss << "fp80"; break;
            case TYPE_FP128:  oss << "fp128"; break;
            default:          oss << "unknown"; break;
        }
    }
    
    return oss.str();
}

Type Type::fromBasicType(uint16_t typeId) {
    switch (typeId) {
        case TYPE_VOID:
            return Type(TYPE_VOID, 0, 1, 0, "void");
        case TYPE_INT8:
            return Type(TYPE_INT8, 1, 1, 0, "int8");
        case TYPE_INT16:
            return Type(TYPE_INT16, 2, 2, 0, "int16");
        case TYPE_INT32:
            return Type(TYPE_INT32, 4, 4, 0, "int32");
        case TYPE_INT64:
            return Type(TYPE_INT64, 8, 8, 0, "int64");
        case TYPE_INT128:
            return Type(TYPE_INT128, 16, 16, 0, "int128");
        case TYPE_UINT8:
            return Type(TYPE_UINT8, 1, 1, 0, "uint8");
        case TYPE_UINT16:
            return Type(TYPE_UINT16, 2, 2, 0, "uint16");
        case TYPE_UINT32:
            return Type(TYPE_UINT32, 4, 4, 0, "uint32");
        case TYPE_UINT64:
            return Type(TYPE_UINT64, 8, 8, 0, "uint64");
        case TYPE_UINT128:
            return Type(TYPE_UINT128, 16, 16, 0, "uint128");
        case TYPE_FP16:
            return Type(TYPE_FP16, 2, 2, 0, "fp16");
        case TYPE_FP32:
            return Type(TYPE_FP32, 4, 4, 0, "fp32");
        case TYPE_FP64:
            return Type(TYPE_FP64, 8, 8, 0, "fp64");
        case TYPE_FP80:
            return Type(TYPE_FP80, 10, 16, 0, "fp80");
        case TYPE_FP128:
            return Type(TYPE_FP128, 16, 16, 0, "fp128");
        default:
            return Type(0, 0, 1, 0, "unknown");
    }
}

} // namespace coil