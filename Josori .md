<img width="1185" height="1187" alt="스크린샷 2026-08-11 115815" src="https://github.com/user-attachments/assets/e16fe397-8226-479b-a579-84affab3ac68" />
``` 
#pragma once

#include <cstdint>
#include <vector>
#include <string>
#include <memory>
#include <functional>
#include <chrono>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <optional>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <limits>
#include <random>
#include <cstring>

namespace josori {

// ============================================================================
// Basic Types
// ============================================================================

struct JosoriException : public std::runtime_error {
    explicit JosoriException(const std::string& msg) : std::runtime_error(msg) {}
};

using Byte = uint8_t;
using ByteBuffer = std::vector<Byte>;
using ParamMap = std::unordered_map<std::string, int64_t>;

enum class LogicalType {
    Int64Array,
    UInt64Array,
    ByteStream
};

enum class ComponentKind : uint8_t {
    Transform = 0,
    Encoder = 1
};

static std::string toString(ComponentKind kind) {
    switch (kind) {
        case ComponentKind::Transform: return "Transform";
        case ComponentKind::Encoder: return "Encoder";
    }
    return "Unknown";
}

static std::string toString(LogicalType t) {
    switch (t) {
        case LogicalType::Int64Array: return "Int64Array";
        case LogicalType::UInt64Array: return "UInt64Array";
        case LogicalType::ByteStream: return "ByteStream";
    }
    return "Unknown";
}

// Very permissive compatibility for prototype.
// Real invalid pipelines are rejected during actual execution / integrity check.
static bool compatible(LogicalType required, LogicalType actual) {
    if (required == LogicalType::ByteStream) return true;
    if (actual == LogicalType::ByteStream) return true;
    return required == actual;
}

static LogicalType baseLogicalType(const std::string& baseType) {
    if (baseType == "int64") return LogicalType::Int64Array;
    if (baseType == "uint64") return LogicalType::UInt64Array;
    return LogicalType::ByteStream;
}

// ============================================================================
// Binary Writer / Reader
// ============================================================================

class BinaryWriter {
public:
    ByteBuffer buf;

    void writeU8(uint8_t v) {
        buf.push_back(v);
    }

    void writeU32(uint32_t v) {
        for (int i = 0; i < 4; ++i) {
            buf.push_back(Byte((v >> (8 * i)) & 0xFF));
        }
    }

    void writeU64(uint64_t v) {
        for (int i = 0; i < 8; ++i) {
            buf.push_back(Byte((v >> (8 * i)) & 0xFF));
        }
    }

    void writeI64(int64_t v) {
        writeU64(static_cast<uint64_t>(v));
    }

    void writeString(const std::string& s) {
        writeU64(static_cast<uint64_t>(s.size()));
        buf.insert(buf.end(), s.begin(), s.end());
    }
};

class BinaryReader {
public:
    const ByteBuffer& buf;
    size_t pos = 0;

    explicit BinaryReader(const ByteBuffer& b) : buf(b) {}

    void ensure(size_t n) const {
        if (pos + n > buf.size()) {
            throw JosoriException("BinaryReader: unexpected end of buffer");
        }
    }

    uint8_t readU8() {
        ensure(1);
        return buf[pos++];
    }

    uint32_t readU32() {
        ensure(4);
        uint32_t v = 0;
        for (int i = 0; i < 4; ++i) {
            v |= uint32_t(buf[pos++]) << (8 * i);
        }
        return v;
    }

    uint64_t readU64() {
        ensure(8);
        uint64_t v = 0;
        for (int i = 0; i < 8; ++i) {
            v |= uint64_t(buf[pos++]) << (8 * i);
        }
        return v;
    }

    int64_t readI64() {
        return static_cast<int64_t>(readU64());
    }

    std::string readString() {
        uint64_t len = readU64();
        if (len > buf.size() - pos) {
            throw JosoriException("BinaryReader: invalid string length");
        }
        std::string s(buf.begin() + pos, buf.begin() + pos + len);
        pos += static_cast<size_t>(len);
        return s;
    }
};

// ============================================================================
// Varint / Little-endian helpers
// ============================================================================

static void appendVarint(ByteBuffer& out, uint64_t v) {
    while (v >= 0x80) {
        out.push_back(Byte((v & 0x7F) | 0x80));
        v >>= 7;
    }
    out.push_back(Byte(v));
}

static uint64_t readVarint(const ByteBuffer& in, size_t& pos) {
    uint64_t v = 0;
    int shift = 0;

    while (true) {
        if (pos >= in.size()) {
            throw JosoriException("readVarint: unexpected end");
        }

        Byte b = in[pos++];
        v |= uint64_t(b & 0x7F) << shift;

        if ((b & 0x80) == 0) break;

        shift += 7;
        if (shift >= 64) {
            throw JosoriException("readVarint: too long");
        }
    }

    return v;
}

static uint64_t loadU64(const ByteBuffer& b, size_t offset) {
    if (offset + 8 > b.size()) {
        throw JosoriException("loadU64: out of range");
    }

    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= uint64_t(b[offset + i]) << (8 * i);
    }
    return v;
}

static void storeU64(ByteBuffer& b, size_t offset, uint64_t v) {
    if (offset + 8 > b.size()) {
        throw JosoriException("storeU64: out of range");
    }

    for (int i = 0; i < 8; ++i) {
        b[offset + i] = Byte((v >> (8 * i)) & 0xFF);
    }
}

static int64_t loadI64(const ByteBuffer& b, size_t offset) {
    return static_cast<int64_t>(loadU64(b, offset));
}

static void storeI64(ByteBuffer& b, size_t offset, int64_t v) {
    storeU64(b, offset, static_cast<uint64_t>(v));
}

// ============================================================================
// Recipe
// ============================================================================

struct PipelineStep {
    ComponentKind kind;
    std::string name;
    ParamMap params;
};

struct FormatRecipe {
    std::string id = "recipe";
    uint32_t version = 2;
    std::string base_type = "int64";
    uint64_t original_count = 0;

    std::vector<PipelineStep> pipeline;

    std::string index_strategy = "NONE";
    uint64_t block_size = 4096;
};

static ByteBuffer serializeRecipe(const FormatRecipe& r) {
    BinaryWriter w;

    w.writeString(r.id);
    w.writeU32(r.version);
    w.writeString(r.base_type);
    w.writeU64(r.original_count);
    w.writeU64(r.block_size);
    w.writeString(r.index_strategy);

    w.writeU32(static_cast<uint32_t>(r.pipeline.size()));

    for (const auto& step : r.pipeline) {
        w.writeU8(static_cast<uint8_t>(step.kind));
        w.writeString(step.name);

        w.writeU32(static_cast<uint32_t>(step.params.size()));
        for (const auto& kv : step.params) {
            w.writeString(kv.first);
            w.writeI64(kv.second);
        }
    }

    return std::move(w.buf);
}

static FormatRecipe deserializeRecipe(const ByteBuffer& data) {
    BinaryReader reader(data);
    FormatRecipe r;

    r.id = reader.readString();
    r.version = reader.readU32();

    if (r.version > 2) {
        throw JosoriException("Unsupported recipe version");
    }

    r.base_type = reader.readString();
    r.original_count = reader.readU64();
    r.block_size = reader.readU64();
    r.index_strategy = reader.readString();

    uint32_t pipelineCount = reader.readU32();

    r.pipeline.reserve(pipelineCount);

    for (uint32_t i = 0; i < pipelineCount; ++i) {
        PipelineStep step;

        uint8_t kindValue = reader.readU8();
        if (kindValue > 1) {
            throw JosoriException("Invalid component kind in recipe");
        }

        step.kind = static_cast<ComponentKind>(kindValue);
        step.name = reader.readString();

        uint32_t paramCount = reader.readU32();

        for (uint32_t j = 0; j < paramCount; ++j) {
            std::string key = reader.readString();
            int64_t value = reader.readI64();
            step.params[key] = value;
        }

        r.pipeline.push_back(std::move(step));
    }

    return r;
}

// Topology key excludes recipe id.
static std::string recipeTopologyKey(const FormatRecipe& r) {
    BinaryWriter w;

    w.writeString(r.base_type);
    w.writeU64(r.original_count);
    w.writeU64(r.block_size);
    w.writeString(r.index_strategy);

    w.writeU32(static_cast<uint32_t>(r.pipeline.size()));

    for (const auto& step : r.pipeline) {
        w.writeU8(static_cast<uint8_t>(step.kind));
        w.writeString(step.name);

        w.writeU32(static_cast<uint32_t>(step.params.size()));
        for (const auto& kv : step.params) {
            w.writeString(kv.first);
            w.writeI64(kv.second);
        }
    }

    return std::string(w.buf.begin(), w.buf.end());
}

// ============================================================================
// Component Interfaces
// ============================================================================

class ITransform {
public:
    virtual ~ITransform() = default;

    virtual std::string name() const = 0;
    virtual LogicalType inputType() const = 0;
    virtual LogicalType outputType() const = 0;

    virtual ByteBuffer forward(const ByteBuffer& input, const ParamMap& params) const = 0;
    virtual ByteBuffer inverse(const ByteBuffer& input, const ParamMap& params) const = 0;
};

class IEncoder {
public:
    virtual ~IEncoder() = default;

    virtual std::string name() const = 0;
    virtual LogicalType inputType() const = 0;
    virtual LogicalType outputType() const = 0;

    virtual ByteBuffer encode(const ByteBuffer& input, const ParamMap& params) const = 0;
    virtual ByteBuffer decode(const ByteBuffer& input, const ParamMap& params) const = 0;
};

template <typename T>
class Registry {
public:
    void registerComponent(std::shared_ptr<T> component) {
        components_[component->name()] = std::move(component);
    }

    bool exists(const std::string& name) const {
        return components_.find(name) != components_.end();
    }

    std::shared_ptr<T> get(const std::string& name) const {
        auto it = components_.find(name);
        if (it == components_.end()) {
            throw JosoriException("Component not found: " + name);
        }
        return it->second;
    }

    std::vector<std::string> names() const {
        std::vector<std::string> out;
        out.reserve(components_.size());
        for (const auto& kv : components_) {
            out.push_back(kv.first);
        }
        std::sort(out.begin(), out.end());
        return out;
    }

private:
    std::unordered_map<std::string, std::shared_ptr<T>> components_;
};

using TransformRegistry = Registry<ITransform>;
using EncoderRegistry = Registry<IEncoder>;

// ============================================================================
// Transforms
// ============================================================================

class DeltaTransform : public ITransform {
public:
    std::string name() const override { return "DELTA"; }
    LogicalType inputType() const override { return LogicalType::Int64Array; }
    LogicalType outputType() const override { return LogicalType::Int64Array; }

    ByteBuffer forward(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("DELTA.forward: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;
        ByteBuffer output(input.size());

        uint64_t prev = 0;

        for (size_t i = 0; i < count; ++i) {
            uint64_t current = loadU64(input, i * 8);

            uint64_t delta;
            if (i == 0) {
                delta = current;
            } else {
                // Unsigned modulo subtraction avoids signed overflow UB.
                delta = current - prev;
            }

            storeU64(output, i * 8, delta);
            prev = current;
        }

        return output;
    }

    ByteBuffer inverse(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("DELTA.inverse: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;
        ByteBuffer output(input.size());

        uint64_t acc = 0;

        for (size_t i = 0; i < count; ++i) {
            uint64_t delta = loadU64(input, i * 8);

            if (i == 0) {
                acc = delta;
            } else {
                acc += delta;
            }

            storeU64(output, i * 8, acc);
        }

        return output;
    }
};

class ZigZagTransform : public ITransform {
public:
    std::string name() const override { return "ZIGZAG"; }
    LogicalType inputType() const override { return LogicalType::Int64Array; }
    LogicalType outputType() const override { return LogicalType::UInt64Array; }

    ByteBuffer forward(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("ZIGZAG.forward: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;
        ByteBuffer output(input.size());

        for (size_t i = 0; i < count; ++i) {
            uint64_t ux = loadU64(input, i * 8);

            uint64_t sign = ux >> 63;
            uint64_t zigzag = (ux << 1) ^ (0ULL - sign);

            storeU64(output, i * 8, zigzag);
        }

        return output;
    }

    ByteBuffer inverse(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("ZIGZAG.inverse: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;
        ByteBuffer output(input.size());

        for (size_t i = 0; i < count; ++i) {
            uint64_t z = loadU64(input, i * 8);
            uint64_t ux = (z >> 1) ^ (0ULL - (z & 1ULL));
            storeU64(output, i * 8, ux);
        }

        return output;
    }
};

// ============================================================================
// Encoders
// ============================================================================

class RawEncoder : public IEncoder {
public:
    std::string name() const override { return "RAW"; }
    LogicalType inputType() const override { return LogicalType::ByteStream; }
    LogicalType outputType() const override { return LogicalType::ByteStream; }

    ByteBuffer encode(const ByteBuffer& input, const ParamMap&) const override {
        return input;
    }

    ByteBuffer decode(const ByteBuffer& input, const ParamMap&) const override {
        return input;
    }
};

class VarIntEncoder : public IEncoder {
public:
    std::string name() const override { return "VARINT"; }
    LogicalType inputType() const override { return LogicalType::UInt64Array; }
    LogicalType outputType() const override { return LogicalType::ByteStream; }

    ByteBuffer encode(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("VARINT.encode: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;
        ByteBuffer output;
        output.reserve(input.size() / 2);

        for (size_t i = 0; i < count; ++i) {
            uint64_t v = loadU64(input, i * 8);
            appendVarint(output, v);
        }

        return output;
    }

    ByteBuffer decode(const ByteBuffer& input, const ParamMap&) const override {
        ByteBuffer output;
        output.reserve(input.size() * 2);

        size_t pos = 0;

        constexpr uint64_t MAX_ELEMENTS = 1ULL << 28;

        while (pos < input.size()) {
            uint64_t v = readVarint(input, pos);

            if (output.size() / 8 >= MAX_ELEMENTS) {
                throw JosoriException("VARINT.decode: too many elements");
            }

            size_t oldSize = output.size();
            output.resize(oldSize + 8);
            storeU64(output, oldSize, v);
        }

        return output;
    }
};

class RLEEncoder : public IEncoder {
public:
    std::string name() const override { return "RLE"; }
    LogicalType inputType() const override { return LogicalType::ByteStream; }
    LogicalType outputType() const override { return LogicalType::ByteStream; }

    ByteBuffer encode(const ByteBuffer& input, const ParamMap&) const override {
        ByteBuffer output;
        output.reserve(input.size());

        size_t i = 0;

        while (i < input.size()) {
            Byte value = input[i];
            size_t j = i + 1;

            while (j < input.size() && input[j] == value) {
                ++j;
            }

            uint64_t runLength = static_cast<uint64_t>(j - i);

            appendVarint(output, runLength);
            output.push_back(value);

            i = j;
        }

        return output;
    }

    ByteBuffer decode(const ByteBuffer& input, const ParamMap&) const override {
        ByteBuffer output;
        output.reserve(input.size() * 2);

        size_t pos = 0;

        constexpr uint64_t MAX_DECODED = 1ULL << 30;

        while (pos < input.size()) {
            uint64_t count = readVarint(input, pos);

            if (pos >= input.size()) {
                throw JosoriException("RLE.decode: missing value byte");
            }

            Byte value = input[pos++];

            size_t oldSize = output.size();

            if (count > MAX_DECODED - oldSize) {
                throw JosoriException("RLE.decode: decoded size limit exceeded");
            }

            output.resize(oldSize + static_cast<size_t>(count), value);
        }

        return output;
    }
};

class BitPackEncoder : public IEncoder {
public:
    std::string name() const override { return "BITPACK"; }
    LogicalType inputType() const override { return LogicalType::UInt64Array; }
    LogicalType outputType() const override { return LogicalType::ByteStream; }

    ByteBuffer encode(const ByteBuffer& input, const ParamMap& params) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("BITPACK.encode: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;

        int requestedBits = 0;
        auto it = params.find("bits");
        if (it != params.end()) {
            if (it->second < 0 || it->second > 64) {
                throw JosoriException("BITPACK.encode: bits must be in [0,64]");
            }
            requestedBits = static_cast<int>(it->second);
        }

        uint64_t maxValue = 0;

        for (size_t i = 0; i < count; ++i) {
            uint64_t v = loadU64(input, i * 8);
            if (v > maxValue) maxValue = v;
        }

        int autoBits = bitWidth(maxValue);
        int bits = (requestedBits == 0) ? autoBits : requestedBits;

        if (bits < 0 || bits > 64) {
            throw JosoriException("BITPACK.encode: invalid bit width");
        }

        if (bits < 64) {
            uint64_t limit = (bits == 0) ? 0 : ((1ULL << bits) - 1ULL);

            for (size_t i = 0; i < count; ++i) {
                uint64_t v = loadU64(input, i * 8);
                if (v > limit) {
                    throw JosoriException("BITPACK.encode: value does not fit in requested bits");
                }
            }
        }

        ByteBuffer output;

        appendVarint(output, static_cast<uint64_t>(bits));
        appendVarint(output, static_cast<uint64_t>(count));

        if (count == 0 || bits == 0) {
            return output;
        }

        if (bits == 64) {
            size_t oldSize = output.size();
            output.resize(oldSize + count * 8);

            for (size_t i = 0; i < count; ++i) {
                uint64_t v = loadU64(input, i * 8);
                storeU64(output, oldSize + i * 8, v);
            }

            return output;
        }

        uint8_t currentByte = 0;
        int bitPos = 0;

        auto flush = [&]() {
            if (bitPos > 0) {
                output.push_back(currentByte);
                currentByte = 0;
                bitPos = 0;
            }
        };

        for (size_t i = 0; i < count; ++i) {
            uint64_t v = loadU64(input, i * 8);

            for (int b = 0; b < bits; ++b) {
                if (v & (1ULL << b)) {
                    currentByte |= uint8_t(1U << bitPos);
                } else {
                    currentByte &= uint8_t(~(1U << bitPos));
                }

                ++bitPos;

                if (bitPos == 8) {
                    output.push_back(currentByte);
                    currentByte = 0;
                    bitPos = 0;
                }
            }
        }

        flush();

        return output;
    }

    ByteBuffer decode(const ByteBuffer& input, const ParamMap&) const override {
        size_t pos = 0;

        uint64_t bitsValue = readVarint(input, pos);
        uint64_t count = readVarint(input, pos);

        if (bitsValue > 64) {
            throw JosoriException("BITPACK.decode: invalid bit width");
        }

        constexpr uint64_t MAX_ELEMENTS = 1ULL << 28;

        if (count > MAX_ELEMENTS) {
            throw JosoriException("BITPACK.decode: too many elements");
        }

        int bits = static_cast<int>(bitsValue);

        ByteBuffer output(count * 8, 0);

        if (count == 0 || bits == 0) {
            return output;
        }

        size_t payloadBytes = input.size() - pos;

        if (bits == 64) {
            if (payloadBytes < count * 8) {
                throw JosoriException("BITPACK.decode: not enough payload");
            }

            for (uint64_t i = 0; i < count; ++i) {
                uint64_t v = loadU64(input, pos + i * 8);
                storeU64(output, i * 8, v);
            }

            return output;
        }

        uint64_t requiredBits = count * uint64_t(bits);
        uint64_t availableBits = uint64_t(payloadBytes) * 8ULL;

        if (requiredBits > availableBits) {
            throw JosoriException("BITPACK.decode: not enough bits");
        }

        size_t bytePos = pos;
        int bitPos = 0;

        auto readBit = [&]() -> bool {
            if (bytePos >= input.size()) {
                throw JosoriException("BITPACK.decode: unexpected end");
            }

            bool bit = (input[bytePos] & (1U << bitPos)) != 0;
            ++bitPos;

            if (bitPos == 8) {
                ++bytePos;
                bitPos = 0;
            }

            return bit;
        };

        for (uint64_t i = 0; i < count; ++i) {
            uint64_t v = 0;

            for (int b = 0; b < bits; ++b) {
                if (readBit()) {
                    v |= (1ULL << b);
                }
            }

            storeU64(output, i * 8, v);
        }

        return output;
    }

private:
    static int bitWidth(uint64_t v) {
        int bits = 0;
        while (v > 0) {
            ++bits;
            v >>= 1;
        }
        return bits;
    }
};

class DictionaryEncoder : public IEncoder {
public:
    std::string name() const override { return "DICTIONARY"; }
    LogicalType inputType() const override { return LogicalType::ByteStream; }
    LogicalType outputType() const override { return LogicalType::ByteStream; }

    ByteBuffer encode(const ByteBuffer& input, const ParamMap&) const override {
        if (input.size() % 8 != 0) {
            throw JosoriException("DICTIONARY.encode: input must be multiple of 8 bytes");
        }

        size_t count = input.size() / 8;

        std::vector<uint64_t> dictionary;
        std::unordered_map<uint64_t, uint32_t> idMap;
        idMap.reserve(std::min<size_t>(count, 1ULL << 20));

        std::vector<uint32_t> ids;
        ids.reserve(count);

        for (size_t i = 0; i < count; ++i) {
            uint64_t value = loadU64(input, i * 8);

            auto it = idMap.find(value);
            if (it == idMap.end()) {
                uint32_t id = static_cast<uint32_t>(dictionary.size());
                dictionary.push_back(value);
                idMap[value] = id;
                ids.push_back(id);
            } else {
                ids.push_back(it->second);
            }
        }

        ByteBuffer output;

        appendVarint(output, static_cast<uint64_t>(dictionary.size()));

        size_t oldSize = output.size();
        output.resize(oldSize + dictionary.size() * 8);

        for (size_t i = 0; i < dictionary.size(); ++i) {
            storeU64(output, oldSize + i * 8, dictionary[i]);
        }

        for (uint32_t id : ids) {
            appendVarint(output, id);
        }

        return output;
    }

    ByteBuffer decode(const ByteBuffer& input, const ParamMap&) const override {
        size_t pos = 0;

        uint64_t dictSize = readVarint(input, pos);

        size_t remaining = input.size() - pos;
        if (dictSize > remaining / 8) {
            throw JosoriException("DICTIONARY.decode: invalid dictionary size");
        }

        std::vector<uint64_t> dictionary;
        dictionary.reserve(static_cast<size_t>(dictSize));

        for (uint64_t i = 0; i < dictSize; ++i) {
            uint64_t v = loadU64(input, pos);
            pos += 8;
            dictionary.push_back(v);
        }

        ByteBuffer output;
        output.reserve(remaining);

        while (pos < input.size()) {
            uint64_t id = readVarint(input, pos);

            if (id >= dictSize) {
                throw JosoriException("DICTIONARY.decode: invalid dictionary id");
            }

            size_t oldSize = output.size();
            output.resize(oldSize + 8);
            storeU64(output, oldSize, dictionary[static_cast<size_t>(id)]);
        }

        return output;
    }
};

// ============================================================================
// Pipeline Executor
// ============================================================================

class PipelineExecutor {
public:
    PipelineExecutor(const TransformRegistry& transforms, const EncoderRegistry& encoders)
        : transforms_(transforms), encoders_(encoders) {}

    LogicalType simulateForward(const FormatRecipe& recipe) const {
        LogicalType current = baseLogicalType(recipe.base_type);

        for (const auto& step : recipe.pipeline) {
            if (step.kind == ComponentKind::Transform) {
                auto component = transforms_.get(step.name);
                if (!compatible(component->inputType(), current)) {
                    throw JosoriException("Incompatible transform: " + component->name());
                }
                current = component->outputType();
            } else {
                auto component = encoders_.get(step.name);
                if (!compatible(component->inputType(), current)) {
                    throw JosoriException("Incompatible encoder: " + component->name());
                }
                current = component->outputType();
            }
        }

        return current;
    }

    ByteBuffer encode(const FormatRecipe& recipe, const ByteBuffer& rawData) const {
        LogicalType currentType = baseLogicalType(recipe.base_type);
        ByteBuffer current = rawData;

        for (const auto& step : recipe.pipeline) {
            if (step.kind == ComponentKind::Transform) {
                auto component = transforms_.get(step.name);

                if (!compatible(component->inputType(), currentType)) {
                    throw JosoriException("Encode pipeline type mismatch at " + component->name());
                }

                current = component->forward(current, step.params);
                currentType = component->outputType();
            } else {
                auto component = encoders_.get(step.name);

                if (!compatible(component->inputType(), currentType)) {
                    throw JosoriException("Encode pipeline type mismatch at " + component->name());
                }

                current = component->encode(current, step.params);
                currentType = component->outputType();
            }
        }

        return current;
    }

    ByteBuffer decode(const FormatRecipe& recipe, const ByteBuffer& encodedData) const {
        LogicalType currentType = simulateForward(recipe);
        ByteBuffer current = encodedData;

        for (auto it = recipe.pipeline.rbegin(); it != recipe.pipeline.rend(); ++it) {
            const PipelineStep& step = *it;

            if (step.kind == ComponentKind::Transform) {
                auto component = transforms_.get(step.name);

                if (!compatible(component->outputType(), currentType)) {
                    throw JosoriException("Decode pipeline type mismatch at " + component->name());
                }

                current = component->inverse(current, step.params);
                currentType = component->inputType();
            } else {
                auto component = encoders_.get(step.name);

                if (!compatible(component->outputType(), currentType)) {
                    throw JosoriException("Decode pipeline type mismatch at " + component->name());
                }

                current = component->decode(current, step.params);
                currentType = component->inputType();
            }
        }

        return current;
    }

private:
    const TransformRegistry& transforms_;
    const EncoderRegistry& encoders_;
};

// ============================================================================
// Analyzer
// ============================================================================

struct DataProfile {
    uint64_t count = 0;
    double unique_ratio = 0.0;
    double entropy = 0.0;
    double avg_abs_delta = 0.0;
    uint64_t max_run = 0;
    bool is_sorted = true;
    int64_t min_value = 0;
    int64_t max_value = 0;
    double null_ratio = 0.0;
};

class Analyzer {
public:
    static DataProfile analyze(const std::vector<int64_t>& data) {
        DataProfile profile;

        profile.count = data.size();

        if (data.empty()) {
            return profile;
        }

        std::unordered_map<int64_t, size_t> freq;
        freq.reserve(std::min<size_t>(data.size(), 100000));

        profile.min_value = data[0];
        profile.max_value = data[0];

        uint64_t currentRun = 1;
        uint64_t maxRun = 1;

        uint64_t deltaSum = 0;
        bool sorted = true;

        for (size_t i = 0; i < data.size(); ++i) {
            int64_t value = data[i];

            freq[value]++;

            if (value < profile.min_value) profile.min_value = value;
            if (value > profile.max_value) profile.max_value = value;

            if (i > 0) {
                int64_t prev = data[i - 1];

                if (prev > value) {
                    sorted = false;
                }

                if (prev == value) {
                    ++currentRun;
                } else {
                    if (currentRun > maxRun) maxRun = currentRun;
                    currentRun = 1;
                }

                uint64_t d = uint64_t(value) - uint64_t(prev);
                if (d >> 63) {
                    d = 0 - d;
                }

                deltaSum += d;
            }
        }

        if (currentRun > maxRun) maxRun = currentRun;

        profile.max_run = maxRun;
        profile.is_sorted = sorted;
        profile.unique_ratio = double(freq.size()) / double(data.size());

        double entropy = 0.0;

        for (const auto& kv : freq) {
            double p = double(kv.second) / double(data.size());
            if (p > 0.0) {
                entropy -= p * std::log2(p);
            }
        }

        profile.entropy = entropy;

        if (data.size() > 1) {
            profile.avg_abs_delta = double(deltaSum) / double(data.size() - 1);
        }

        return profile;
    }
};

// ============================================================================
// Optimization Goal / Evaluation
// ============================================================================

struct OptimizationGoal {
    double size_weight = 0.45;
    double encode_speed_weight = 0.20;
    double decode_speed_weight = 0.25;
    double memory_weight = 0.10;

    double time_budget_ms = 100.0;
};

struct EvaluatorConfig {
    int warmup = 2;
    int iterations = 5;
};

struct EvaluationMetrics {
    bool valid = false;
    std::string error;

    size_t encoded_size = 0;
    double compression_ratio = 1.0;

    double encode_ms = 0.0;
    double decode_ms = 0.0;

    double memory_bytes = 0.0;

    double score = std::numeric_limits<double>::infinity();
};

static double median(std::vector<double> values) {
    if (values.empty()) return 0.0;

    std::sort(values.begin(), values.end());

    return values[values.size() / 2];
}

class Evaluator {
public:
    Evaluator(const PipelineExecutor& executor, OptimizationGoal goal, EvaluatorConfig config)
        : executor_(executor), goal_(goal), config_(config) {}

    EvaluationMetrics evaluate(const FormatRecipe& recipe, const ByteBuffer& rawData) const {
        EvaluationMetrics metrics;

        try {
            if (rawData.empty()) {
                ByteBuffer encoded = executor_.encode(recipe, rawData);
                ByteBuffer decoded = executor_.decode(recipe, encoded);

                if (decoded != rawData) {
                    metrics.valid = false;
                    metrics.error = "Integrity check failed";
                    return metrics;
                }

                metrics.valid = true;
                metrics.encoded_size = encoded.size();
                metrics.compression_ratio = 1.0;
                metrics.encode_ms = 0.0;
                metrics.decode_ms = 0.0;
                metrics.memory_bytes = 0.0;
                metrics.score = 0.0;
                return metrics;
            }

            // Warmup
            for (int i = 0; i < config_.warmup; ++i) {
                ByteBuffer encoded = executor_.encode(recipe, rawData);
                ByteBuffer decoded = executor_.decode(recipe, encoded);

                if (decoded != rawData) {
                    metrics.valid = false;
                    metrics.error = "Integrity check failed in warmup";
                    return metrics;
                }
            }

            std::vector<double> encodeTimes;
            std::vector<double> decodeTimes;

            size_t encodedSize = 0;
            double memoryBytes = 0.0;

            for (int i = 0; i < config_.iterations; ++i) {
                auto t0 = std::chrono::high_resolution_clock::now();
                ByteBuffer encoded = executor_.encode(recipe, rawData);
                auto t1 = std::chrono::high_resolution_clock::now();

                ByteBuffer decoded = executor_.decode(recipe, encoded);
                auto t2 = std::chrono::high_resolution_clock::now();

                if (decoded != rawData) {
                    metrics.valid = false;
                    metrics.error = "Integrity check failed";
                    return metrics;
                }

                double encodeMs = std::chrono::duration<double, std::milli>(t1 - t0).count();
                double decodeMs = std::chrono::duration<double, std::milli>(t2 - t1).count();

                encodeTimes.push_back(encodeMs);
                decodeTimes.push_back(decodeMs);

                encodedSize = encoded.size();

                memoryBytes = double(rawData.size()) + double(encoded.size()) + double(decoded.size());
            }

            metrics.valid = true;
            metrics.encoded_size = encodedSize;
            metrics.compression_ratio = double(rawData.size()) / double(std::max<size_t>(1, encodedSize));
            metrics.encode_ms = median(std::move(encodeTimes));
            metrics.decode_ms = median(std::move(decodeTimes));
            metrics.memory_bytes = memoryBytes;

            double rawSize = double(std::max<size_t>(1, rawData.size()));

            double sizeCost = double(metrics.encoded_size) / rawSize;
            double encodeCost = metrics.encode_ms / goal_.time_budget_ms;
            double decodeCost = metrics.decode_ms / goal_.time_budget_ms;
            double memoryCost = metrics.memory_bytes / (rawSize * 3.0);

            metrics.score =
                goal_.size_weight * sizeCost +
                goal_.encode_speed_weight * encodeCost +
                goal_.decode_speed_weight * decodeCost +
                goal_.memory_weight * memoryCost;
        } catch (const std::exception& e) {
            metrics.valid = false;
            metrics.error = e.what();
            metrics.score = std::numeric_limits<double>::infinity();
        }

        return metrics;
    }

private:
    const PipelineExecutor& executor_;
    OptimizationGoal goal_;
    EvaluatorConfig config_;
};

// ============================================================================
// AI Search Engine
// ============================================================================

struct SearchConfig {
    size_t initial_population = 16;
    size_t beam_width = 6;
    size_t iterations = 4;
    size_t mutations_per_candidate = 6;

    int warmup = 2;
    int benchmark_iterations = 5;

    uint64_t seed = 123456789ULL;
};

struct ScoredRecipe {
    FormatRecipe recipe;
    EvaluationMetrics metrics;
};

struct SearchOutcome {
    FormatRecipe recipe;
    EvaluationMetrics metrics;
};

static PipelineStep makeTransform(const std::string& name, ParamMap params = {}) {
    return PipelineStep{ComponentKind::Transform, name, std::move(params)};
}

static PipelineStep makeEncoder(const std::string& name, ParamMap params = {}) {
    return PipelineStep{ComponentKind::Encoder, name, std::move(params)};
}

class SearchEngine {
public:
    SearchEngine(
        const PipelineExecutor& executor,
        const TransformRegistry& transforms,
        const EncoderRegistry& encoders,
        SearchConfig config
    )
        : executor_(executor),
          transformNames_(transforms.names()),
          encoderNames_(encoders.names()),
          cfg_(config),
          rng_(config.seed),
          idCounter_(0) {}

    SearchOutcome search(
        const ByteBuffer& rawData,
        const DataProfile& profile,
        const OptimizationGoal& goal
    ) {
        EvaluatorConfig evalConfig;
        evalConfig.warmup = cfg_.warmup;
        evalConfig.iterations = cfg_.benchmark_iterations;

        Evaluator evaluator(executor_, goal, evalConfig);

        std::unordered_map<std::string, EvaluationMetrics> cache;

        auto getMetrics = [&](const FormatRecipe& recipe) -> EvaluationMetrics {
            std::string key = recipeTopologyKey(recipe);

            auto it = cache.find(key);
            if (it != cache.end()) {
                return it->second;
            }

            EvaluationMetrics metrics = evaluator.evaluate(recipe, rawData);
            cache[key] = metrics;
            return metrics;
        };

        std::vector<FormatRecipe> initial = generateInitialPopulation(rawData, profile);

        std::vector<ScoredRecipe> population;

        for (auto& recipe : initial) {
            EvaluationMetrics metrics = getMetrics(recipe);
            population.push_back(ScoredRecipe{std::move(recipe), metrics});
        }

        sortPopulation(population);
        population = uniqueByTopology(population);

        if (population.empty()) {
            throw JosoriException("SearchEngine: no valid initial recipe");
        }

        ScoredRecipe best = population.front();

        for (size_t iter = 0; iter < cfg_.iterations; ++iter) {
            std::vector<ScoredRecipe> nextGeneration;

            size_t eliteCount = std::min(cfg_.beam_width, population.size());

            for (size_t i = 0; i < eliteCount; ++i) {
                nextGeneration.push_back(population[i]);
            }

            for (size_t i = 0; i < eliteCount; ++i) {
                const ScoredRecipe& parent = population[i];

                for (size_t m = 0; m < cfg_.mutations_per_candidate; ++m) {
                    FormatRecipe candidate = mutateRecipe(parent.recipe);

                    EvaluationMetrics metrics = getMetrics(candidate);

                    nextGeneration.push_back(ScoredRecipe{std::move(candidate), metrics});
                }
            }

            sortPopulation(nextGeneration);
            nextGeneration = uniqueByTopology(nextGeneration);

            size_t keep = std::max<size_t>(cfg_.beam_width * 2, cfg_.beam_width);
            if (nextGeneration.size() > keep) {
                nextGeneration.resize(keep);
            }

            population = std::move(nextGeneration);

            if (!population.empty()) {
                if (better(population.front(), best)) {
                    best = population.front();
                }
            }
        }

        return SearchOutcome{best.recipe, best.metrics};
    }

private:
    const PipelineExecutor& executor_;
    std::vector<std::string> transformNames_;
    std::vector<std::string> encoderNames_;
    SearchConfig cfg_;
    std::mt19937_64 rng_;
    uint64_t idCounter_;

    std::string newId() {
        return "gen_" + std::to_string(idCounter_++);
    }

    bool validateQuiet(const FormatRecipe& recipe) const {
        try {
            executor_.simulateForward(recipe);
            return true;
        } catch (...) {
            return false;
        }
    }

    FormatRecipe makeRawRecipe(const ByteBuffer& rawData) const {
        FormatRecipe recipe;
        recipe.id = "raw";
        recipe.version = 2;
        recipe.base_type = "int64";
        recipe.original_count = rawData.size() / 8;
        recipe.index_strategy = "NONE";
        recipe.block_size = 4096;
        return recipe;
    }

    std::vector<FormatRecipe> generateInitialPopulation(
        const ByteBuffer& rawData,
        const DataProfile& profile
    ) {
        std::vector<FormatRecipe> out;
        std::unordered_set<std::string> seen;

        uint64_t rawCount = rawData.size() / 8;

        auto addCandidate = [&](FormatRecipe recipe) {
            recipe.base_type = "int64";
            recipe.original_count = rawCount;

            if (!validateQuiet(recipe)) {
                return;
            }

            std::string key = recipeTopologyKey(recipe);

            if (seen.insert(key).second) {
                out.push_back(std::move(recipe));
            }
        };

        FormatRecipe rawRecipe = makeRawRecipe(rawData);
        addCandidate(rawRecipe);

        // Heuristic seeds are allowed as initial exploration hints.
        // Final decision is still made by actual evaluation and mutation search.
        {
            FormatRecipe r = rawRecipe;
            r.id = "seed_delta_zigzag_varint";
            r.pipeline = {
                makeTransform("DELTA"),
                makeTransform("ZIGZAG"),
                makeEncoder("VARINT")
            };
            addCandidate(std::move(r));
        }

        {
            FormatRecipe r = rawRecipe;
            r.id = "seed_zigzag_varint";
            r.pipeline = {
                makeTransform("ZIGZAG"),
                makeEncoder("VARINT")
            };
            addCandidate(std::move(r));
        }

        {
            FormatRecipe r = rawRecipe;
            r.id = "seed_delta_zigzag_bitpack";
            r.pipeline = {
                makeTransform("DELTA"),
                makeTransform("ZIGZAG"),
                makeEncoder("BITPACK", {{"bits", 0}})
            };
            addCandidate(std::move(r));
        }

        {
            FormatRecipe r = rawRecipe;
            r.id = "seed_dictionary_rle";
            r.pipeline = {
                makeEncoder("DICTIONARY"),
                makeEncoder("RLE")
            };
            addCandidate(std::move(r));
        }

        if (profile.unique_ratio < 0.10) {
            FormatRecipe r = rawRecipe;
            r.id = "seed_dictionary_low_cardinality";
            r.pipeline = {
                makeEncoder("DICTIONARY"),
                makeEncoder("BITPACK", {{"bits", 0}})
            };
            addCandidate(std::move(r));
        }

        if (profile.avg_abs_delta < 64.0) {
            FormatRecipe r = rawRecipe;
            r.id = "seed_small_delta";
            r.pipeline = {
                makeTransform("DELTA"),
                makeTransform("ZIGZAG"),
                makeEncoder("BITPACK", {{"bits", 0}}),
                makeEncoder("RLE")
            };
            addCandidate(std::move(r));
        }

        size_t maxAttempts = 1000;
        size_t attempts = 0;

        while (out.size() < cfg_.initial_population && attempts < maxAttempts) {
            ++attempts;

            FormatRecipe r = generateRandomRecipe(rawCount);
            addCandidate(std::move(r));
        }

        return out;
    }

    FormatRecipe generateRandomRecipe(uint64_t originalCount) {
        FormatRecipe recipe;
        recipe.id = newId();
        recipe.version = 2;
        recipe.base_type = "int64";
        recipe.original_count = originalCount;
        recipe.block_size = randomBlockSize();
        recipe.index_strategy = randomIndexStrategy();

        std::uniform_int_distribution<int> lengthDist(0, 4);
        int length = lengthDist(rng_);

        recipe.pipeline.reserve(length);

        for (int i = 0; i < length; ++i) {
            recipe.pipeline.push_back(randomStep());
        }

        return recipe;
    }

    PipelineStep randomStep() {
        if (transformNames_.empty() && encoderNames_.empty()) {
            throw JosoriException("No components registered");
        }

        bool useTransform = false;

        if (!transformNames_.empty() && encoderNames_.empty()) {
            useTransform = true;
        } else if (transformNames_.empty() && !encoderNames_.empty()) {
            useTransform = false;
        } else {
            std::uniform_int_distribution<int> coin(0, 1);
            useTransform = (coin(rng_) == 0);
        }

        if (useTransform) {
            std::uniform_int_distribution<size_t> dist(0, transformNames_.size() - 1);
            return makeTransform(transformNames_[dist(rng_)]);
        } else {
            std::uniform_int_distribution<size_t> dist(0, encoderNames_.size() - 1);
            std::string name = encoderNames_[dist(rng_)];

            if (name == "BITPACK") {
                return makeEncoder(name, {{"bits", 0}});
            }

            return makeEncoder(name);
        }
    }

    uint64_t randomBlockSize() {
        static const std::vector<uint64_t> sizes = {
            256, 512, 1024, 2048, 4096, 8192, 16384, 32768, 65536
        };

        std::uniform_int_distribution<size_t> dist(0, sizes.size() - 1);
        return sizes[dist(rng_)];
    }

    std::string randomIndexStrategy() {
        static const std::vector<std::string> strategies = {
            "NONE", "SORTED", "HASH", "BITMAP"
        };

        std::uniform_int_distribution<size_t> dist(0, strategies.size() - 1);
        return strategies[dist(rng_)];
    }

    FormatRecipe mutateRecipe(const FormatRecipe& source) {
        for (int attempt = 0; attempt < 32; ++attempt) {
            FormatRecipe candidate = source;
            candidate.id = newId();

            std::uniform_int_distribution<int> opDist(0, 6);
            int op = opDist(rng_);

            if (op == 0) {
                // ADD_COMPONENT
                std::uniform_int_distribution<size_t> posDist(0, candidate.pipeline.size());
                size_t pos = posDist(rng_);
                candidate.pipeline.insert(candidate.pipeline.begin() + pos, randomStep());
            } else if (op == 1) {
                // REMOVE_COMPONENT
                if (candidate.pipeline.empty()) continue;

                std::uniform_int_distribution<size_t> idxDist(0, candidate.pipeline.size() - 1);
                size_t idx = idxDist(rng_);

                candidate.pipeline.erase(candidate.pipeline.begin() + idx);
            } else if (op == 2) {
                // REPLACE_COMPONENT
                if (candidate.pipeline.empty()) continue;

                std::uniform_int_distribution<size_t> idxDist(0, candidate.pipeline.size() - 1);
                size_t idx = idxDist(rng_);

                candidate.pipeline[idx] = randomStep();
            } else if (op == 3) {
                // SWAP_COMPONENT
                if (candidate.pipeline.size() < 2) continue;

                std::uniform_int_distribution<size_t> idxDist(0, candidate.pipeline.size() - 2);
                size_t idx = idxDist(rng_);

                std::swap(candidate.pipeline[idx], candidate.pipeline[idx + 1]);
            } else if (op == 4) {
                // CHANGE_PARAMETER
                std::vector<size_t> bitpackIndices;

                for (size_t i = 0; i < candidate.pipeline.size(); ++i) {
                    if (candidate.pipeline[i].name == "BITPACK") {
                        bitpackIndices.push_back(i);
                    }
                }

                if (!bitpackIndices.empty()) {
                    static const std::vector<int64_t> bitCandidates = {
                        0, 1, 2, 4, 8, 12, 16, 24, 32, 48, 64
                    };

                    std::uniform_int_distribution<size_t> stepDist(0, bitpackIndices.size() - 1);
                    std::uniform_int_distribution<size_t> bitDist(0, bitCandidates.size() - 1);

                    size_t stepIndex = bitpackIndices[stepDist(rng_)];
                    candidate.pipeline[stepIndex].params["bits"] = bitCandidates[bitDist(rng_)];
                } else {
                    candidate.block_size = randomBlockSize();
                }
            } else if (op == 5) {
                // CHANGE_BLOCK_SIZE
                candidate.block_size = randomBlockSize();
            } else {
                // CHANGE_INDEX
                candidate.index_strategy = randomIndexStrategy();
            }

            if (validateQuiet(candidate)) {
                return candidate;
            }
        }

        return source;
    }

    static bool better(const ScoredRecipe& a, const ScoredRecipe& b) {
        if (a.metrics.valid != b.metrics.valid) {
            return a.metrics.valid > b.metrics.valid;
        }

        return a.metrics.score < b.metrics.score;
    }

    void sortPopulation(std::vector<ScoredRecipe>& population) const {
        std::sort(population.begin(), population.end(), [&](const ScoredRecipe& a, const ScoredRecipe& b) {
            return better(a, b);
        });
    }

    std::vector<ScoredRecipe> uniqueByTopology(const std::vector<ScoredRecipe>& population) const {
        std::vector<ScoredRecipe> out;
        std::unordered_set<std::string> seen;

        for (const auto& item : population) {
            std::string key = recipeTopologyKey(item.recipe);

            if (seen.insert(key).second) {
                out.push_back(item);
            }
        }

        return out;
    }
};

// ============================================================================
// Josori File Format
// ============================================================================

struct JosoriFileConstants {
    static constexpr const char MAGIC[8] = {'J', 'O', 'S', 'O', 'R', 'I', '0', '2'};
    static constexpr uint32_t FILE_VERSION = 2;
};

static void writeStreamU32(std::ostream& out, uint32_t v) {
    unsigned char buf[4];
    for (int i = 0; i < 4; ++i) {
        buf[i] = unsigned char((v >> (8 * i)) & 0xFF);
    }
    out.write(reinterpret_cast<const char*>(buf), 4);
}

static void writeStreamU64(std::ostream& out, uint64_t v) {
    unsigned char buf[8];
    for (int i = 0; i < 8; ++i) {
        buf[i] = unsigned char((v >> (8 * i)) & 0xFF);
    }
    out.write(reinterpret_cast<const char*>(buf), 8);
}

static uint32_t readStreamU32(std::istream& in) {
    unsigned char buf[4];
    in.read(reinterpret_cast<char*>(buf), 4);

    if (!in) {
        throw JosoriException("Failed to read U32");
    }

    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) {
        v |= uint32_t(buf[i]) << (8 * i);
    }

    return v;
}

static uint64_t readStreamU64(std::istream& in) {
    unsigned char buf[8];
    in.read(reinterpret_cast<char*>(buf), 8);

    if (!in) {
        throw JosoriException("Failed to read U64");
    }

    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) {
        v |= uint64_t(buf[i]) << (8 * i);
    }

    return v;
}

static void saveJosoriFile(
    const std::string& path,
    const FormatRecipe& recipe,
    const ByteBuffer& encodedData
) {
    std::ofstream out(path, std::ios::binary | std::ios::trunc);

    if (!out) {
        throw JosoriException("Cannot open file for writing: " + path);
    }

    ByteBuffer recipeBytes = serializeRecipe(recipe);

    out.write(JosoriFileConstants::MAGIC, 8);
    writeStreamU32(out, JosoriFileConstants::FILE_VERSION);

    writeStreamU64(out, recipeBytes.size());

    if (!recipeBytes.empty()) {
        out.write(reinterpret_cast<const char*>(recipeBytes.data()), recipeBytes.size());
    }

    writeStreamU64(out, recipe.original_count);
    writeStreamU64(out, encodedData.size());

    if (!encodedData.empty()) {
        out.write(reinterpret_cast<const char*>(encodedData.data()), encodedData.size());
    }

    out.flush();

    if (!out) {
        throw JosoriException("Failed to write Josori file: " + path);
    }
}

static std::pair<FormatRecipe, ByteBuffer> loadJosoriFile(const std::string& path) {
    std::ifstream in(path, std::ios::binary);

    if (!in) {
        throw JosoriException("Cannot open file for reading: " + path);
    }

    char magic[8];
    in.read(magic, 8);

    if (!in) {
        throw JosoriException("Failed to read magic");
    }

    if (std::memcmp(magic, JosoriFileConstants::MAGIC, 8) != 0) {
        throw JosoriException("Invalid Josori magic");
    }

    uint32_t version = readStreamU32(in);

    if (version != JosoriFileConstants::FILE_VERSION) {
        throw JosoriException("Unsupported Josori file version");
    }

    uint64_t recipeSize = readStreamU64(in);

    constexpr uint64_t MAX_RECIPE_SIZE = 64ULL * 1024ULL * 1024ULL;
    if (recipeSize > MAX_RECIPE_SIZE) {
        throw JosoriException("Recipe size too large");
    }

    ByteBuffer recipeBytes(static_cast<size_t>(recipeSize));

    if (recipeSize > 0) {
        in.read(reinterpret_cast<char*>(recipeBytes.data()), recipeSize);

        if (!in) {
            throw JosoriException("Failed to read recipe bytes");
        }
    }

    uint64_t originalCount = readStreamU64(in);
    uint64_t encodedSize = readStreamU64(in);

    constexpr uint64_t MAX_ENCODED_SIZE = 1ULL << 30;

    if (encodedSize > MAX_ENCODED_SIZE) {
        throw JosoriException("Encoded data too large");
    }

    ByteBuffer encodedData(static_cast<size_t>(encodedSize));

    if (encodedSize > 0) {
        in.read(reinterpret_cast<char*>(encodedData.data()), encodedSize);

        if (!in) {
            throw JosoriException("Failed to read encoded data");
        }
    }

    FormatRecipe recipe = deserializeRecipe(recipeBytes);

    if (recipe.original_count != originalCount) {
        throw JosoriException("Recipe original_count mismatch with file header");
    }

    return {recipe, encodedData};
}

// ============================================================================
// Runtime Profiler (prototype)
// ============================================================================

class RuntimeProfiler {
public:
    void recordRead() {
        ++reads_;
    }

    void recordWrite() {
        ++writes_;
    }

    double readRatio() const {
        uint64_t total = reads_ + writes_;
        if (total == 0) return 0.5;
        return double(reads_) / double(total);
    }

    void resetObservation() {
        initialReadRatio_ = readRatio();
        initialized_ = true;
    }

    bool shouldReoptimize(double threshold = 0.25) const {
        if (!initialized_) return false;

        return std::fabs(readRatio() - initialReadRatio_) > threshold;
    }

private:
    uint64_t reads_ = 0;
    uint64_t writes_ = 0;

    bool initialized_ = false;
    double initialReadRatio_ = 0.5;
};

// ============================================================================
// JosoriDB
// ============================================================================

class JosoriDB {
public:
    JosoriDB() {
        registerDefaults();
        executor_ = std::make_unique<PipelineExecutor>(transforms_, encoders_);
    }

    void store(
        const std::string& path,
        const std::vector<int64_t>& data,
        OptimizationGoal goal = OptimizationGoal{},
        SearchConfig searchConfig = SearchConfig{}
    ) {
        ByteBuffer rawData = toByteBuffer(data);

        DataProfile profile = Analyzer::analyze(data);

        SearchEngine engine(*executor_, transforms_, encoders_, searchConfig);

        SearchOutcome outcome = engine.search(rawData, profile, goal);

        FormatRecipe best = outcome.recipe;
        best.original_count = data.size();

        ByteBuffer encoded = executor_->encode(best, rawData);
        ByteBuffer decoded = executor_->decode(best, encoded);

        if (decoded != rawData) {
            throw JosoriException("Final integrity check failed before save");
        }

        saveJosoriFile(path, best, encoded);

        profiler_.recordWrite();
        profiler_.resetObservation();

        printReport(profile, best, outcome.metrics, rawData.size(), encoded.size());
    }

    std::vector<int64_t> load(const std::string& path) {
        auto [recipe, encoded] = loadJosoriFile(path);

        if (recipe.base_type != "int64") {
            throw JosoriException("Prototype supports only int64 base type");
        }

        ByteBuffer decoded = executor_->decode(recipe, encoded);

        if (decoded.size() % 8 != 0) {
            throw JosoriException("Decoded data is not multiple of 8 bytes");
        }

        if (decoded.size() / 8 != recipe.original_count) {
            throw JosoriException("Decoded count mismatch");
        }

        std::vector<int64_t> out(decoded.size() / 8);

        for (size_t i = 0; i < out.size(); ++i) {
            out[i] = loadI64(decoded, i * 8);
        }

        profiler_.recordRead();

        return out;
    }

    // Simple lossless migration:
    // load old file -> decode -> re-optimize -> save new file.
    void migrate(
        const std::string& path,
        OptimizationGoal newGoal = OptimizationGoal{},
        SearchConfig searchConfig = SearchConfig{}
    ) {
        std::vector<int64_t> data = load(path);
        store(path, data, newGoal, searchConfig);
    }

    const RuntimeProfiler& profiler() const {
        return profiler_;
    }

private:
    TransformRegistry transforms_;
    EncoderRegistry encoders_;

    std::unique_ptr<PipelineExecutor> executor_;

    RuntimeProfiler profiler_;

    void registerDefaults() {
        transforms_.registerComponent(std::make_shared<DeltaTransform>());
        transforms_.registerComponent(std::make_shared<ZigZagTransform>());

        encoders_.registerComponent(std::make_shared<RawEncoder>());
        encoders_.registerComponent(std::make_shared<VarIntEncoder>());
        encoders_.registerComponent(std::make_shared<RLEEncoder>());
        encoders_.registerComponent(std::make_shared<BitPackEncoder>());
        encoders_.registerComponent(std::make_shared<DictionaryEncoder>());
    }

    static ByteBuffer toByteBuffer(const std::vector<int64_t>& data) {
        ByteBuffer raw(data.size() * 8);

        for (size_t i = 0; i < data.size(); ++i) {
            storeI64(raw, i * 8, data[i]);
        }

        return raw;
    }

    static void printReport(
        const DataProfile& profile,
        const FormatRecipe& recipe,
        const EvaluationMetrics& metrics,
        size_t rawSize,
        size_t encodedSize
    ) {
        std::cout << "[JOSORI AI]\n";
        std::cout << "Input count: " << profile.count << "\n";
        std::cout << "Unique ratio: " << profile.unique_ratio << "\n";
        std::cout << "Entropy: " << profile.entropy << "\n";
        std::cout << "Avg abs delta: " << profile.avg_abs_delta << "\n";
        std::cout << "Max run: " << profile.max_run << "\n";
        std::cout << "Sorted: " << (profile.is_sorted ? "true" : "false") << "\n";

        std::cout << "\nGenerated Format: " << recipe.id << "\n";
        std::cout << "base_type: " << recipe.base_type << "\n";
        std::cout << "block_size: " << recipe.block_size << "\n";
        std::cout << "index: " << recipe.index_strategy << "\n";

        std::cout << "pipeline:\n";

        if (recipe.pipeline.empty()) {
            std::cout << "  RAW_MEMORY\n";
        } else {
            for (const auto& step : recipe.pipeline) {
                std::cout << "  " << toString(step.kind) << " " << step.name;

                if (!step.params.empty()) {
                    std::cout << " (";
                    bool first = true;

                    for (const auto& kv : step.params) {
                        if (!first) std::cout << ", ";
                        std::cout << kv.first << "=" << kv.second;
                        first = false;
                    }

                    std::cout << ")";
                }

                std::cout << "\n";
            }
        }

        std::cout << "\nEvaluation:\n";
        std::cout << "  valid: " << (metrics.valid ? "true" : "false") << "\n";
        std::cout << "  raw_size: " << rawSize << " bytes\n";
        std::cout << "  encoded_size: " << encodedSize << " bytes\n";
        std::cout << "  compression_ratio: " << metrics.compression_ratio << "\n";
        std::cout << "  encode_ms: " << metrics.encode_ms << "\n";
        std::cout << "  decode_ms: " << metrics.decode_ms << "\n";
        std::cout << "  score: " << metrics.score << "\n";
        std::cout << std::endl;
    }
};

} // namespace josori
