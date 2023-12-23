#pragma once

#include <concepts>
#include <cstdint>
#include <iterator>
#include <ranges>
#include <string>
#include <type_traits>

#include "serializable.hpp"

// examplle of some random external struct that will
// end up as a member of an object that gets serialized
struct Coordinates
{
    float x{ 0.0f };
    float y{ 0.0f };
};

// example on an entity type that isn't trivially copyable
// so it implements the only two functions needed for this
// object to be able to read/write directly to/from binary
struct ObjectA : Serializable<ObjectA>
{
    uint8_t id{ 0 };
    uint8_t health{ 0 };
    std::string name{};

    inline int32_t store_to_buffer(auto& buffer) const
    {
        using buffer_t = std::remove_cvref_t<decltype(buffer)>;

        if constexpr (std::same_as<buffer_t, std::ofstream>)
        {
            // write out fixed size attributes first
            buffer.write(reinterpret_cast<const char*>(&id), sizeof(id));
            buffer.write(reinterpret_cast<const char*>(&health), sizeof(health));
            // instead of just writing out the string, write the length first
            const size_t name_len{ this->name.size() };
            buffer.write(reinterpret_cast<const char*>(&name_len), sizeof(name_len));
            // then the string. that way the length tells us how many bytes to
            // read back in when this file is going to be loaded from disk
            buffer.write(name.c_str(), name_len);
            return static_cast<int32_t>(sizeof(id) + sizeof(health) + sizeof(name_len) + name_len);
        }

        if constexpr (std::same_as<buffer_t, std::vector<char>>)
        {
            const size_t name_len{ this->name.size() };
            const char* bin_id{ reinterpret_cast<const char*>(&id) };
            const char* bin_health{ reinterpret_cast<const char*>(&health) };
            const char* bin_name_len{ reinterpret_cast<const char*>(&name_len) };
            const char* bin_name{ name.c_str() };

            buffer.insert(buffer.end(), bin_id, bin_id + sizeof(id));
            buffer.insert(buffer.end(), bin_health, bin_health + sizeof(health));
            buffer.insert(buffer.end(), bin_name_len, bin_name_len + sizeof(name_len));
            buffer.insert(buffer.end(), bin_name, bin_name + name_len);

            return static_cast<int32_t>(sizeof(id) + sizeof(health) + sizeof(name_len) + name_len);
        }
    }

    inline int32_t load_from_buffer(auto& buffer)
    {
        // std::ifstream only... TODO: add vector support
        buffer.read(reinterpret_cast<char*>(&id), sizeof(id));
        buffer.read(reinterpret_cast<char*>(&health), sizeof(health));

        size_t name_len{ 0 };
        buffer.read(reinterpret_cast<char*>(&name_len), sizeof(name_len));

        name.reserve(name_len + 1);
        buffer.read(reinterpret_cast<char*>(name.data()), name_len);

        *this = std::move({ id, health, name });
        return sizeof(id) + sizeof(health) + sizeof(name_len) + name_len;
    }

    constexpr inline bool operator==(const ObjectA& other) const
    {
        return id == other.id          //
            && health == other.health  //
            && name == other.name;     //
    }
};

// example of a trivially copyable entity type.
// this one just works automatically, you only
// need to inherit from Serializable and pass the
// same entity type in as the template argument
struct ObjectB : Serializable<ObjectB>
{
    uint32_t id{ 0 };
    Coordinates pos = {
        0.0f,
        0.0f,
    };
};

// ===============================================
// the two structs below aren't entity examples,
// they are what will make up the header section
// of the binary file that stores all of the data.
// ===============================================

namespace detail {
    // stores the data about an individual type
    // of entity being serialized or deserialized.
    // This separate nested struct is needed so it
    // also be serialized itself as part of the bin
    // file header combined with the others being managed
    struct EntityProperties : Serializable<EntityProperties>
    {
        // the number of this type of
        // entity being stored or loaded
        uint32_t count{ 0 };
        // the 4-byte aligned sizeof() for
        // the entity being captured
        uint32_t size{ 0 };
        // the type name as a string
        char name[32] = { 0 };
    };

    struct EntityBinFile
    {
        EntityProperties info{};
        // stores the binary data being read/written
        // to a file on disk or a membuffer
        std::vector<char> buffer{};
    };
}
