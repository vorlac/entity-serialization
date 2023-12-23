#include <fmt/format.h>

#include "entity_serializer.hpp"

int main(int argc, char** argv)
{
    auto entities_a = std::vector<ObjectA>{
        { .id = 1, .health = 100, .name = "long variable length name" },
        { .id = 55, .health = 19, .name = "shorter varlen name" }
    };

    auto entities_b = std::vector<ObjectB>{
        { .id = 11, .pos = { .x = 100.0f, .y = 0.1f } },
        { .id = 22, .pos = { .x = 1.0f, .y = 12345.6f } },
        { .id = 33, .pos = { .x = 666.6f, .y = 666.6f } },
    };

    EntitySerializer serializer{
        entities_a,
        entities_b,
    };

    return 0;
}
