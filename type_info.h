/**
 * File: type_info.h
 * Description: A hand rolled type information.
 * 1. Enables dynamic casting using dyn_cast<Destination>(Source) similar to dynamic_cast.
 * 2. Generates unique id for every type using type_id<Type>().
 * 3. Obtain the type id of the dynamic type of an object using type_id(Expression).
 * Example:
 * ~~~
 * class a
 * {
 * public:
 *     using zpp_bases = zpp::make_zpp_bases<>;
 *     virtual zpp::dynamic_type zpp_dynamic_type() const { return *this; }
 *     virtual ~a() = default;
 *
 * private:
 *     int value = 0x11111111;
 * };
 *
 * class b
 * {
 * public:
 *     using zpp_bases = zpp::make_zpp_bases<>;
 *     virtual zpp::dynamic_type zpp_dynamic_type() const { return *this; }
 *     virtual ~b() = default;
 *
 * private:
 *     int value = 0x22222222;
 * };
 *
 * class c : public a, public b
 * {
 * public:
 *     using zpp_bases = zpp:make_zpp_bases<a, b>;
 *     zpp::dynamic_type zpp_dynamic_type() const override { return *this; }
 *     virtual ~c() = default;
 *
 * private:
 *     int value = 0x33333333;
 * };
 *
 * void foo()
 * {
 *     auto object = std::make_unique<c>();
 *     c * pc = object.get();
 *     a * pa = pc;
 *     b * pb = pc;
 *
 *     auto dyn_a_from_b = zpp::dyn_cast<a *>(pb);
 *     auto dyn_b_from_a = zpp::dyn_cast<b *>(pa);
 *     auto dyn_c_from_a = zpp::dyn_cast<c *>(pa);
 *     auto dyn_c_from_b = zpp::dyn_cast<c *>(pb);
 *
 *     auto dyn_void_from_a = zpp::dyn_cast<void *>(pa);
 *     auto dyn_void_from_b = zpp::dyn_cast<void *>(pb);
 *     auto dyn_void_from_c = zpp::dyn_cast<void *>(pc);
 *
 *     std::cout << "a: " << pa << ", dyn_a_from_b: " << dyn_a_from_b << '\n';
 *     std::cout << "b: " << pb << ", dyn_b_from_a: " << dyn_b_from_a << '\n';
 *     std::cout << "c: " << pc << ", dyn_c_from_a: " << dyn_c_from_a << '\n';
 *     std::cout << "c: " << pc << ", dyn_c_from_b: " << dyn_c_from_b << '\n';
 *
 *     std::cout << "a: " << pa << ", dyn_void_from_a: " << dyn_void_from_a << '\n';
 *     std::cout << "b: " << pb << ", dyn_void_from_b: " << dyn_void_from_b << '\n';
 *     std::cout << "c: " << pc << ", dyn_void_from_c: " << dyn_void_from_c << '\n';
 *
 *     std::cout << "type_id<a>(): " << zpp::type_id<a>() << '\n';
 *     std::cout << "type_id<b>(): " << zpp::type_id<b>() << '\n';
 *     std::cout << "type_id<c>(): " << zpp::type_id<c>() << '\n';
 *
 *     std::cout << "type_id(*pa): " << zpp::type_id(*pa) << '\n';
 *     std::cout << "type_id(*pb): " << zpp::type_id(*pb) << '\n';
 *     std::cout << "type_id(*pc): " << zpp::type_id(*pc) << '\n';
 * }
 * ~~~
 */
#pragma once

#include <type_traits>
#include <cstdint>
#include <memory>

namespace zpp {

/**
 * Use this to enable dynamic cast from sources to a given type.
 */
template <typename... Sources>
struct make_zpp_bases {};

/**
 * Returns unique id for a given type.
 */
template <typename Type, typename...,
    typename = std::enable_if_t<std::is_class_v<Type>>,
    typename = typename Type::zpp_bases
>
auto type_id() noexcept -> std::uintptr_t;

namespace type_information_detail {

/**
 * Trait to check whether static cast is well formed.
 */
template <typename Source, typename Destination, typename = void>
struct can_static_cast : std::false_type {};

/**
 * Trait to check whether static cast is well formed.
 */
template <typename Source, typename Destination>
struct can_static_cast<Source, Destination,
    std::void_t<decltype(static_cast<Destination>(std::declval<Source &>()))>
> : std::true_type {};

/**
 * Trait to check whether static cast is well formed.
 */
template <typename Source, typename Destination>
constexpr auto can_static_cast_v = can_static_cast<Source, Destination>::value;

/**
 * A type info entry.
 */
union type_info_entry
{
    /**
     * Construct a type info entry from integral type.
     */
    template <typename Type, typename = std::enable_if_t<std::is_integral_v<Type>>>
    type_info_entry(Type number) :
        number(number)
    {
    }

    /**
     * Construct a type info entry from function pointer.
     */
    type_info_entry(std::uintptr_t(*function)(std::uintptr_t)) :
        function(function)
    {
    }

    /**
     * A user defined number.
     */
    std::uintptr_t number;

    /**
     * A user defined function.
     */
    std::uintptr_t(*function)(std::uintptr_t);
};

/**
 * Casts from source to destination using full erasure.
 */
template <typename Source, typename Destination>
std::uintptr_t erased_static_cast(std::uintptr_t source) noexcept
{
    return reinterpret_cast<std::uintptr_t>(
        static_cast<Destination *>(reinterpret_cast<Source *>(source)));
}

/**
 * Casts from source to destination using full erasure.
 */
template <typename Source, typename Destination>
auto make_erased_static_cast() noexcept
{
    return &erased_static_cast<Source, Destination>;
}

template <typename Type, typename... Sources,
    typename = std::enable_if_t<std::is_class_v<Type>>,
    typename = typename Type::zpp_bases
>
auto type_id(make_zpp_bases<Sources...>) noexcept
{
    // Construct the type information.
    static const type_info_entry info[] = {
        sizeof...(Sources), // Number of source classes.
        zpp::type_id<Sources>()..., // Source classes type information.
        make_erased_static_cast<Type, Sources>()..., // Casts from derived to base.
    };

    // Return the unique id that is the address of the type information.
    return reinterpret_cast<std::uintptr_t>(info);
}

template <typename Type, typename...,
    typename = std::enable_if_t<std::is_class_v<Type>>,
    typename = typename Type::zpp_bases
>
auto type_id(make_zpp_bases<>) noexcept
{
    // Construct the type information.
    static const type_info_entry info[] = {
        0, // Number of source classes.
    };

    // Return the unique id that is the address of the type information.
    return reinterpret_cast<std::uintptr_t>(info);
}

/**
 * Returns true if the dynamic type 'most_derived' is convertible to 'base', else false.
 */
inline bool dynamic_is_convertible(std::uintptr_t base, std::uintptr_t most_derived)
{
    // If the most derived and the base are the same.
    if (most_derived == base) {
        return true;
    }

    // Fetch the type info entries.
    auto type_info_entries = reinterpret_cast<const type_info_entry *>(most_derived);

    // The number of base types.
    std::uintptr_t number_of_base_types = type_info_entries->number;

    // The bases type information.
    auto bases = type_info_entries + 1;

    // Iterate all the bases.
    for (std::size_t index = 0; index < number_of_base_types; ++index) {
        // Check if type represented by bases[index].number is convertible to base.
        if (dynamic_is_convertible(base, bases[index].number)) {
            return true;
        }
    }

    // No conversion was found.
    return false;
}

/**
 * Performs the conversion from the dynamic type whose id is 'most_derived' to the desired base.
 */
inline std::uintptr_t dynamic_convert(std::uintptr_t base,
    std::uintptr_t most_derived_pointer,
    std::uintptr_t most_derived)
{
    // If the most derived and the base are the same.
    if (most_derived == base) {
        return most_derived_pointer;
    }

    // Fetch the type info entries.
    auto type_info_entries = reinterpret_cast<const type_info_entry *>(most_derived);

    // The number of base types.
    std::uintptr_t number_of_base_types = type_info_entries->number;

    // The bases type information.
    auto bases = type_info_entries + 1;

    // The erased static cast function matching base.
    auto erased_static_cast = bases + number_of_base_types;

    for (std::size_t index = 0; index < number_of_base_types; ++index) {
        // Converting the most derived to the type whose id is bases[index].number and
        // perform the conversion from this type.
        auto result = dynamic_convert(base,
            erased_static_cast[index].function(most_derived_pointer),
            bases[index].number);

        // If the conversion succeeded, return the result.
        if (result) {
            return result;
        }
    }

    // No conversion was found.
    return 0;
}

} // type_information_detail

/**
 * Returns unique id for a given type.
 */
template <typename Type, typename...,
    typename /*= std::enable_if_t<std::is_class_v<Type>> */,
    typename /*= typename Type::zpp_bases*/
>
auto type_id() noexcept -> std::uintptr_t
{
    return type_information_detail::type_id<std::remove_cv_t<std::remove_reference_t<Type>>>(typename Type::zpp_bases{});
}

/**
 * Represents the type information.
 */
struct dynamic_type
{
    /**
     * Constructs dynamic type for a given type.
     */
    template <typename Type>
    dynamic_type(Type && value) :
        type_id(zpp::type_id<std::remove_cv_t<std::remove_reference_t<Type>>>()),
        this_pointer(reinterpret_cast<std::uintptr_t>(std::addressof(value)))
    {
    }

    std::uintptr_t type_id;
    std::uintptr_t this_pointer;
};

/**
 * Returns unique id of the dynamic type.
 */
template <typename Type, typename...,
    typename = std::enable_if_t<std::is_class_v<Type>>,
    typename = typename Type::zpp_bases
>
auto type_id(const Type & object) noexcept
{
    return object.zpp_dynamic_type().type_id;
}

/**
 * Casts from source to destination, where destination is the base type,
 * essentially using static cast.
 */
template <typename Destination, typename Source, typename...,
    typename = std::enable_if_t<
        std::is_pointer_v<Destination> &&
        std::is_pointer_v<Source> &&
        std::is_class_v<std::remove_pointer_t<Destination>> &&
        std::is_class_v<std::remove_pointer_t<Source>> &&
        std::is_convertible_v<Source, Destination>
    >
>
constexpr Destination dyn_cast(Source source) noexcept
{
    return static_cast<Destination>(source);
}

/**
 * Casts from source to destination, where destination is the derived type.
 * On failure, null is returned.
 * This overload is used for down casts.
 */
template <typename Destination, typename Source, typename...,
    typename = std::enable_if_t<
        std::is_pointer_v<Destination> &&
        std::is_pointer_v<Source> &&
        std::is_class_v<std::remove_pointer_t<Destination>> &&
        std::is_class_v<std::remove_pointer_t<Source>> &&
        !std::is_convertible_v<Source, Destination> &&
        std::is_convertible_v<Destination, Source>
    >,
    typename = decltype(static_cast<Source>(std::declval<Destination>()))
>
Destination dyn_cast(Source source) noexcept
{
    // Destination type information.
    std::uintptr_t dest_info = type_id<std::remove_pointer_t<Destination>>();

    // Most derived type information.
    std::uintptr_t most_derived = source->zpp_dynamic_type().type_id;

    // If convertible, return static cast to destination.
    if (type_information_detail::dynamic_is_convertible(dest_info, most_derived)) {
        return static_cast<Destination>(source);
    }

    // Conversion is not possible.
    return nullptr;
}

/**
 * Casts from source to destination, where destination is the derived type.
 * On failure, null is returned.
 * This overload is used for cross casts.
 */
template <typename Destination, typename Source, typename...,
    typename = std::enable_if_t<
        std::is_pointer_v<Destination> &&
        std::is_pointer_v<Source> &&
        std::is_class_v<std::remove_pointer_t<Destination>> &&
        std::is_class_v<std::remove_pointer_t<Source>> &&
        !std::is_convertible_v<Source, Destination> &&
        !type_information_detail::can_static_cast_v<Destination, Source> &&
        (
            // Either:

            // Destination is const and volatile.
            (std::is_const_v<std::remove_pointer_t<Destination>> &&
             std::is_volatile_v<std::remove_pointer_t<Destination>>) ||

            // Source is not const and not volatile.
            (!std::is_const_v<std::remove_pointer_t<Source>> &&
             !std::is_volatile_v<std::remove_reference_t<Source>>) ||

            // Destination is not const, volatile, and source is not const.
            (!std::is_const_v<std::remove_pointer_t<Destination>> &&
             std::is_volatile_v<std::remove_pointer_t<Destination>> &&
             !std::is_const_v<std::remove_pointer_t<Source>>) ||

            // Destination is const, not volatile, and source is not volatile.
            (std::is_const_v<std::remove_pointer_t<Destination>> &&
             !std::is_volatile_v<std::remove_pointer_t<Destination>> &&
             !std::is_volatile_v<std::remove_pointer_t<Source>>) ||

            // Destination is not const and not volatile, and source is not const and not volatile.
            (!std::is_const_v<std::remove_pointer_t<Destination>> &&
             !std::is_volatile_v<std::remove_pointer_t<Destination>> &&
             !std::is_const_v<std::remove_pointer_t<Source>> &&
             !std::is_volatile_v<std::remove_pointer_t<Source>>)
        )
    >,
    typename = void, typename = void
>
Destination dyn_cast(Source source) noexcept
{
    // Destination type information.
    std::uintptr_t dest_info = type_id<std::remove_pointer_t<Destination>>();

    // Most derived type information.
    auto most_derived = source->zpp_dynamic_type();

    // Attempt conversion from 'most_derived' to the destination.
    return reinterpret_cast<Destination>(
        type_information_detail::dynamic_convert(dest_info,
            most_derived.this_pointer,
            most_derived.type_id));
}

/**
 * Casts from source to destination, where destination is a cv qualified void pointer.
 */
template <typename Destination, typename Source, typename...,
    typename = std::enable_if_t<
        std::is_pointer_v<Destination> &&
        std::is_pointer_v<Source> &&
        std::is_void_v<std::remove_cv_t<std::remove_pointer_t<Destination>>> &&
        std::is_convertible_v<Source, Destination>
    >,
    typename = void, typename  = void, typename  = void
>
Destination dyn_cast(Source source) noexcept
{
    // Return the pointer to the most derived.
    return reinterpret_cast<Destination>(source->zpp_dynamic_type().this_pointer);
}

} // zpp
