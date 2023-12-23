#pragma once

#include <memory>
#include <type_traits>
#include <utility>
#include <variant>

template <typename... TVisitorFunction>
struct variant_visitor : TVisitorFunction...
{
    template <typename... TCallable>
    variant_visitor(TCallable&&... vc)
        : TVisitorFunction{ std::forward<TCallable>(vc) }...
    {
    }

    using TVisitorFunction::operator()...;
};

template <typename... TVisitorFunction>
variant_visitor(TVisitorFunction...)
    -> variant_visitor<std::remove_reference_t<TVisitorFunction>...>;
