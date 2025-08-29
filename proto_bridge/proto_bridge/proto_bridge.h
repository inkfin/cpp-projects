#pragma once

#include <vector>
#include <sstream>

namespace proto_bridge {

template <typename T> struct Repeated { using type = std::vector<T>; };
template <typename T> struct Optional { using type = std::optional<T>; };
template <typename T> struct Required { using type = T; };

template <typename T>
struct unwrap_field_type { using type = T; };

template <template <typename> class Wrapper, typename T>
struct unwrap_field_type<Wrapper<T>> { using type = typename Wrapper<T>::type; };

template <typename T>
using field_t = typename unwrap_field_type<T>::type;

#define PB_FIELD(spec, name)                                                                        \
    static_assert(!std::is_same_v<spec, typename ::proto_bridge::unwrap_field_type<spec>::type>, \
        "Field spec must be wrapped in Optional / Required / Repeated");                         \
    ::proto_bridge::field_t<spec> name;

#define REGISTER_PB_FIELDS(...) \
    static constexpr auto field_members() { \
        return std::make_tuple(__VA_ARGS__); \
    }

#define PB_FIELD_PTR(name) std::make_pair(#name, &Self::name)

template <typename T>
std::ostream& operator<<(std::ostream& os, const std::optional<T>& opt)
{
    if (opt) {
        os << *opt;
    } else {
        os << "nullopt";
    }
    return os;
}


struct proto_convertible_base_tag {};

template <typename Self>
struct proto_convertible_trait : proto_convertible_base_tag {
    using decl_type = Self;

    template <typename Proto>
    static Self from_proto(const Proto& p)
    {
        Self out;
        out.from_proto_impl(p);
        return out;
    }

    template <typename Proto>
    void to_proto(Proto& p) const
    {
        static_cast<const Self*>(this)->to_proto_impl(p);
    }

    std::string debug_string() const
    {
        std::ostringstream oss;
        std::apply([this, &oss](auto&&... pair) {
            ((oss << pair.first << ": " << static_cast<const Self*>(this)->*(pair.second) << "\n"), ...);
        },
            Self::field_members());
        return oss.str();
    }

    bool operator==(const Self& other) const
    {
        return std::apply([&](auto&&... pair) {
            return (((static_cast<const Self*>(this)->*(pair.second)) == (other.*(pair.second))) && ...);
        },
            Self::field_members());
    }

};

/// Overload operator<< for debug print

template <typename T>
std::ostream& operator<<(std::ostream& os, const proto_bridge::proto_convertible_trait<T>& obj) {
    os << obj.debug_string();
    return os;
}

/// Trait Detector

template <typename T>
constexpr bool has_proto_convertible_trait_v = std::is_base_of_v<proto_convertible_base_tag, T>;

}
