#pragma once

#include <bit>
#include <concepts>
#include <filesystem>
#include <fstream>
#include <memory>
#include <ostream>
#include <ranges>
#include <string>
#include <tuple>
#include <type_traits>
#include <typeinfo>
#include <utility>
#include <variant>
#include <vector>

#include "entities.hpp"
#include "utils.hpp"

class EntitySerializer
{
    // TODO: make parameter
    const static inline std::filesystem::path m_output_file_rel{ "./entities.bin" };
    const static inline std::filesystem::path m_output_file_abs{
        std::filesystem::absolute(m_output_file_rel),
    };

public:
    template <typename... T>
    using entity_variant = std::variant<T...>;

    // ctor that takes in any number of vectors (each can contain different types of entities and
    // everything will still work as long as each entity type:
    // 1. inherits from `Serializable<>`
    // 2. IFF the entity type in not trivially copyable, two functions also need to be implemented.
    //    a. one to define how to safely convert all member data to binary (char*, std::byte, etc)
    //    b. one to define how to unpack the binary back into the struct. see entities.hpp for
    //       examples
    template <typename... TIterable>
    EntitySerializer(TIterable&... entity_containers)
    {
        auto entity_collections = { std::forward<entity_variant<TIterable...>>(
            entity_containers)... };

        const size_t entity_list_count{ entity_collections.size() };
        for (auto&& ent_vec : entity_collections)
        {
            auto process_entity_vectors = [&](auto&& entity_vec) {
                if (entity_vec.empty())
                    return;

                using entity_t = std::remove_cvref_t<decltype(entity_vec)>;
                std::string type_name{ typeid(entity_t::value_type).name() };
                if (type_name.size() > 30)
                    type_name.resize(30);

                detail::EntityBinFile bindata{};
                std::memcpy(bindata.info.name, type_name.data(), type_name.size());
                bindata.info.name[type_name.size()] = '\0';
                bindata.info.size = static_cast<uint32_t>(sizeof(entity_t));
                bindata.info.count = static_cast<uint32_t>(entity_vec.size());

                for (auto&& entity : entity_vec)
                    entity.store_to_buffer(bindata.buffer);

                m_binfile_data.push_back(bindata);
            };

            std::visit(
                variant_visitor{
                    process_entity_vectors,
                },
                ent_vec);
        }

        // first need to record how many collections of unique entities are about to
        // be serialized so that we know how many records will be in the file header
        std::ofstream fh_out{ m_output_file_abs, std::ios::out | std::ios::binary };
        fh_out.write(reinterpret_cast<const char*>(&entity_list_count), sizeof(entity_list_count));
        for (const auto& ei : m_binfile_data)
            ei.info.store_to_buffer(fh_out);
        for (const auto& ei : m_binfile_data)
            fh_out.write(ei.buffer.data(), ei.buffer.size());
        fh_out.close();
    }

    std::vector<char> m_buffer{};
    std::vector<detail::EntityBinFile> m_binfile_data{};
};
