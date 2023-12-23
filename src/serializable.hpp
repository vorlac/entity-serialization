#pragma once

#include <concepts>
#include <cstdint>
#include <type_traits>

#include "entity_serializer.hpp"

template <typename TEntity>
class Serializable
{
    // The serializer is the only object that should
    // be able to call anything defined in the Serialize base class
    friend class EntitySerializer;

    // The serialize and deserialize functions are just passthroughs that will enforce defniitions
    // for:
    //   1. auto TEntity::store_to_buffer(auto&& buffer) const
    //   2. auto TEntity::load_from_buffer(auto&& buffer)
    // exist for any object that will interact with EntitySerializer. 4 byte alignment if forced for
    // for all read and write operations to try ensure consistent behavior when packing/unpacking
    // from a file on disk or in-memory buffer
    inline auto serialize(auto& buffer) const
    {
        auto& self = *static_cast<const TEntity*>(this);

        // clang-format off
        #pragma pack(4)
        auto bytes_written = self.store_to_buffer(buffer);
        #pragma pack()
        // clang-format on

        return bytes_written;
    }

    // The serialize and deserialize functions are just passthroughs that will enforce defniitions
    // for:
    //   1. auto TEntity::store_to_buffer(auto&& buffer) const
    //   2. auto TEntity::load_from_buffer(auto&& buffer)
    // exist for any object that will interact with EntitySerializer. 4 byte alignment if forced for
    // for all read and write operations to try ensure consistent behavior when packing/unpacking
    // from a file on disk or in-memory buffer
    inline auto deserialize(const auto& buffer)
    {
        auto& self = *static_cast<const TEntity*>(this);
        // clang-format off
        #pragma pack(4) // std::forward<decltype(buffer)>(buffer)
        auto bytes_read{ self.load_from_buffer(buffer) };
        #pragma pack()
        // clang-format on
        return bytes_read;
    }

    // Generic implementation of TEntity::store_to_buffer() function that will automatically be
    // provided to all TEntity objects that are trivially copyable. Any complex objects will need to
    // handle serialization and deserialization that's specific to the member data that needs to be
    // stored/loaded for that object
    inline int32_t store_to_buffer(auto& buffer) const
        requires std::is_trivially_copyable_v<TEntity>
    {
        const auto& self = *static_cast<const TEntity*>(this);
        const uint32_t byte_size{ sizeof(self) };
        const char* data{ self };

        using buffer_t = std::remove_cvref_t<decltype(buffer)>;
        if constexpr (std::same_as<buffer_t, std::vector<char>>)
            buffer.insert(buffer.end(), data, data + byte_size);
        if constexpr (std::same_as<buffer_t, std::ofstream>)
            buffer.write(data, byte_size);

        return byte_size;
    }

    // Generic implementation of TEntity::load_from_buffer() function that will automatically be
    // provided to all TEntity objects that are trivially copyable. Any complex objects will need to
    // handle serialization and deserialization that's specific to the member data that needs to be
    // stored/loaded for that object
    inline int32_t load_from_buffer(const auto& buffer) const
        requires std::is_trivially_copyable_v<TEntity>
    {
        auto& self = *static_cast<const TEntity*>(this);
        const int32_t byte_size{ sizeof(self) };

        using buffer_t = std::remove_reference_t<decltype(buffer)>;
        if constexpr (std::same_as<buffer_t, std::vector<char>>)
            // move `size` bytes from the back of the vector into the entity
            self = std::ranges::move(buffer.back() - byte_size, buffer.back());
        if constexpr (std::same_as<buffer_t, std::ifstream>)
            buffer.read(self, byte_size);

        return byte_size;
    }

    // implicit casting from `TEntity` to `char*` will be provided automatically for all trivially
    // copyable TEntity objects that inherit from `Serialize<>`. Any non-pod types will require a
    // more involved approach to converting the TRntity object into binary data, which should be
    // handled in `TEntity::load_from_buffer()` and `TEntity::store_to_buffer()`
    inline operator char*()
        requires std::is_trivially_copyable_v<TEntity>
    {
        TEntity* self{ static_cast<TEntity*>(this) };
        return reinterpret_cast<char*>(self);
    }

    // implicit casting from `const TEntity&` to `const char*` will be provided automatically for
    // all trivially copyable TEntity objects that inherit from `Serialize<>`. Any non-pod types
    // will require a more involved approach to converting the TRntity object into binary data,
    // which should be handled in `TEntity::load_from_buffer()` and `TEntity::store_to_buffer()`
    inline operator const char*() const
        requires std::is_trivially_copyable_v<TEntity>
    {
        const TEntity* self{ static_cast<const TEntity*>(this) };
        return reinterpret_cast<const char*>(self);
    }
};
