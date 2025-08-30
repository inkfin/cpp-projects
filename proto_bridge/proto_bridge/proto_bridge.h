#pragma once

#include <type_traits>
#include <vector>
#include <sstream>

namespace proto_bridge {

// ========== Common Traits ==========

template <class T> struct is_optional : std::false_type {};
template <class U> struct is_optional<std::optional<U>> : std::true_type { using value_type = U; };

template <class T> struct is_vector : std::false_type {};
template <class U, class A> struct is_vector<std::vector<U,A>> : std::true_type { using value_type = U; };

template <class, class = void> struct is_msg : std::false_type {}; // nested message
template <class T> struct is_msg<T, std::void_t<decltype(T::field_members())>> : std::true_type {};


// ========== Field IO ==========

template<class...> struct always_false : std::false_type {};

// ----------------------------------------------------------------------------
// io<T>
// ----------------------------------------------------------------------------
// Purpose: Access a *singular* field `x` on a protobuf message `Proto`.
//
// You will *specialize* this template for your field, and in that specialization
// implement the methods that match the protobuf-generated accessors.
//
// There are two usage patterns:
//
// 1) Scalar / string field (e.g., int32, double, std::string)
//    - Proto API: x() -> T, set_x(const T&)
//    - Implement: get_value(), set_value()
//
// 2) Message field (submessage)
//    - Proto API: x() -> const Msg&, mutable_x() -> Msg*, clear_x()
//    - Implement: get_message(), mutable_message(), (optionally) clear()
//
// Notes:
// - You do NOT need to implement both shapes; implement only the ones that
//   correspond to your field’s actual type.
// - clear() is optional here (singular required fields may not expose clear_x()).
// ----------------------------------------------------------------------------
template <class T, class Tag>
struct io {
    // --- Scalar / string singular field ---
    template <class Proto>
    static T get_value(const Proto&);

    template <class Proto>
    static void set_value(Proto&, const T&);

    // --- Message singular field ---
    // Return the protobuf *submessage*; callers typically convert it.
    template <class Proto>
    static const auto& get_message(const Proto&);

    // Return a mutable protobuf *submessage* pointer for writing.
    template <class Proto>
    static auto* mutable_message(Proto&);

    // Optional: clear singular field when API provides clear_x()
    template <class Proto>
    static void clear(Proto&);
};

// ----------------------------------------------------------------------------
// optional_io<T>
// ----------------------------------------------------------------------------
// Purpose: Presence control for *optional* singular fields.
//
// Map to generated accessors:
//   - has_x() -> bool
//   - clear_x()
//
// Combine with io<T>::get_value / get_message for the actual read,
// and io<T>::set_value / mutable_message for the write.
// ----------------------------------------------------------------------------
template <class T, class Tag>
struct optional_io {
    template <class Proto>
    static bool has(const Proto&);

    template <class Proto>
    static void clear(Proto&);
};

// ----------------------------------------------------------------------------
// repeated_io<T>
// ----------------------------------------------------------------------------
// Purpose: Access a *repeated* field `x` with element type T.
//
// Protobuf generates two different APIs depending on whether T is scalar/string
// or a submessage. Provide the subset that matches your field.
//
// Scalar/string repeated (e.g., repeated int32/string):
//   - x_size() -> int
//   - x(int) -> T                 // read element by value
//   - add_x(const T&)             // append value
//   - set_x(int, const T&)        // overwrite at index (optional but useful)
//   - clear_x()
//
// Message repeated (e.g., repeated Msg):
//   - x_size() -> int
//   - x(int) -> const Msg&        // read const ref
//   - mutable_x(int) -> Msg*      // get mutable element
//   - add_x() -> Msg*             // append new element, returns pointer
//   - clear_x()
//
// We expose both shapes below. Specialize the ones that apply.
// ----------------------------------------------------------------------------
template <class T, class Tag>
struct repeated_io {
    template <class Proto>
    static int size(const Proto&);

    // ---- Scalar/string shape ----
    template <class Proto>
    static T at_value(const Proto&, int /*index*/);

    template <class Proto>
    static void set_value(Proto&, int /*index*/, const T& /*v*/);

    template <class Proto>
    static void add_value(Proto&, const T& /*v*/);

    // ---- Message shape ----
    template <class Proto>
    static const auto& at_message(const Proto&, int /*index*/);

    template <class Proto>
    static auto* mutable_at_message(Proto&, int /*index*/);

    template <class Proto>
    static auto* add_message(Proto&);

    // ---- Common ----
    template <class Proto>
    static void clear(Proto&);
};


// ========== Field Descriptor ==========

template <auto MemberPtr, class Policy>
struct Field {
    static constexpr auto member = MemberPtr;
    using policy = Policy;
    const char* name; /* for debug print */
};

#define PB_FIELD_REQUIRED(SelfT, member, ElemT) \
    ::proto_bridge::Field<&SelfT::member, ::proto_bridge::Required<ElemT, member##_tag>>{#member}

#define PB_FIELD_OPTIONAL(SelfT, member, ElemT) \
    ::proto_bridge::Field<&SelfT::member, ::proto_bridge::Optional<ElemT, member##_tag>>{#member}

#define PB_FIELD_REPEATED(SelfT, member, ElemT) \
    ::proto_bridge::Field<&SelfT::member, ::proto_bridge::Repeated<ElemT, member##_tag>>{#member}

// ========== Policies ==========

template <class T, class Tag>
struct Required {
    template <class Proto>
    static T read(const Proto& p)
    {
        if constexpr (is_msg<T>::value) {
            return T::from_proto(io<T, Tag>::template get_message<Proto>(p));
        } else {
            return io<T, Tag>::template get_value<Proto>(p);
        }
    }

    template <class Proto>
    static void write(Proto& p, const T& v) {
        if constexpr (is_msg<T>::value) {
            v.to_proto(*io<T, Tag>::template mutable_message<Proto>(p));
        } else {
            io<T, Tag>::template set_value<Proto>(p, v);
        }
    }
};


template <class T, class Tag>
struct Optional {
    using Opt = std::optional<T>;

    template <class Proto>
    static Opt read(const Proto& p) {
        if (!optional_io<T, Tag>::template has<Proto>(p)) {
            return std::nullopt;
        }
        if constexpr (is_msg<T>::value) {
            return T::from_proto(io<T, Tag>::template get_message<Proto>(p));
        } else {
            return io<T, Tag>::template get_value<Proto>(p);
        }
    }

    template <class Proto>
    static void write(Proto& p, const Opt& ov) {
        if (!ov) {
            // explicitly clear the field if optional is empty
            optional_io<T, Tag>::template clear<Proto>(p);
            return;
        }
        if constexpr (is_msg<T>::value) {
            ov->to_proto(*io<T, Tag>::template mutable_message<Proto>(p));
        } else {
            io<T, Tag>::template set_value<Proto>(p, *ov);
        }
    }
};


template <class T, class Tag>
struct Repeated {
    using Vec = std::vector<T>;

    template <class Proto>
    static Vec read(const Proto& p) {
        Vec out;
        out.reserve(repeated_io<T, Tag>::template size<Proto>(p));
        if constexpr (is_msg<T>::value) {
            for (int i = 0, n = repeated_io<T, Tag>::template size<Proto>(p); i < n; ++i) {
                out.push_back(T::from_proto(repeated_io<T, Tag>::template at_message<Proto>(p, i)));
            }
        } else {
            for (int i = 0, n = repeated_io<T, Tag>::template size<Proto>(p); i < n; ++i) {
                out.push_back(repeated_io<T, Tag>::template at_value<Proto>(p, i));
            }
        }
        return out;
    }

    template <class Proto>
    static void write(Proto& p, const Vec& v) {
        repeated_io<T, Tag>::template clear<Proto>(p);
        if constexpr (is_msg<T>::value) {
            for (const auto& e : v) {
                e.to_proto(*repeated_io<T, Tag>::template add_message<Proto>(p));
            }
        } else {
            for (const auto& e : v) {
                repeated_io<T, Tag>::template add_value<Proto>(p, e);
            }
        }
    }
};


// ========== Debug Print Adapters ==========

template <int L, int Step>
inline std::ostream& pad(std::ostream& os) {
    for (int i = 0; i < L * Step; ++i) os.put(' ');
    return os;
}

// fallback printer
template <int L, int Step, typename T,
          typename = std::enable_if_t<!is_optional<T>::value &&
                                      !is_vector<T>::value &&
                                      !is_msg<T>::value>>
std::ostream& print_field(std::ostream& os, const T& v) {
    os << v;
    return os;
}

// optional<T>
template <int L, int Step, typename T>
std::ostream& print_field(std::ostream& os, const std::optional<T>& ov) {
    if (ov) {
        print_field<L, Step>(os, *ov);
    } else {
        os << "nullopt";
    }
    return os;
}

// vector<T>
template <int L, int Step, typename T>
std::ostream& print_field(std::ostream& os, const std::vector<T>& vec) {
    if (vec.empty()) { os << "[]"; return os; }
    os << "[\n";
    for (size_t i = 0; i < vec.size(); ++i) {
        pad<L+1, Step>(os);
        print_field<L+1, Step>(os, vec[i]);
        if (i + 1 < vec.size()) os << ",";
        os << "\n";
    }
    pad<L, Step>(os) << "]";
    return os;
}

// message
template <int L, int Step, typename T>
std::ostream& print_field(std::ostream& os, const T& v) {
    os << "{\n";
    std::apply([&](auto&&... field) {
            auto each = [&](auto&& f) {
                pad<L+1, Step>(os) << (f.name ? f.name : "<unknown>") << ": ";
                print_field<L+1, Step>(os, v.*(std::decay_t<decltype(f)>::member));
                os << "\n";
            };
            (each(field), ...);
        }, T::field_members());
    pad<L, Step>(os) << "}";
    return os;
}


// ========== CRTP Base Class ==========

template <typename Self>
struct Base {
    using decl_type = Self;

    Self&       self()       { return static_cast<Self&>(*this); }
    const Self& self() const { return static_cast<const Self&>(*this); }

    // ========== Core Methods ==========

    // Load all fields from proto message
    template <typename Proto>
    static Self from_proto(const Proto& p)
    {
        Self out {};
        std::apply([&](auto... field){
            (read_one(out, p, field), ...);
        }, Self::field_members());
        return out;
    }

    // Convert to any proto message
    template <typename Proto>
    void to_proto(Proto& p) const {
        std::apply([&](auto... field) {
            (write_one(self(), p, field), ...);
        }, Self::field_members());
    }

    // ======== Constructors and Assignment ========

    // default constructor
    Base() = default;

    // copy constructor from proto
    template <typename Proto>
    explicit Base(const Proto& p) {
        std::apply([&](auto... field){
            (read_one(self(), p, field), ...);
        }, Self::field_members());
    }

    template <typename Proto>
    explicit Base(const Proto&& p) = delete;

    // copy constructor from proto
    template <typename Proto>
    Self& operator=(const Proto& p) {
        self().from_proto(p);
        return self();
    }

    template <typename Proto>
    Self& operator=(const Proto&& p) = delete;

    // conversion operator to proto
    template <typename Proto>
    explicit operator Proto() const {
        Proto out{};
        self().to_proto(out);
        return out;
    }

    // ========== Utilities ==========

    std::string debug_string() const
    {
        std::ostringstream oss;
        std::apply([this, &oss](auto&&... field) {
            auto one = [&](auto&& f) {
                oss << (f.name ? f.name : "<unnamed>") << ": ";
                print_field<0, 2>(oss, self().*(std::decay_t<decltype(f)>::member));
                oss << "\n";
            };
            (one(field), ...);
        }, Self::field_members());
        return oss.str();
    }

    // // equality operator, only if all fields are comparable
    // bool operator==(const Self& other) const
    // {
    //     return std::apply([&](auto&&... field) {
    //         return (((self().*(field.member)) == (other.*(field.member))) && ...);
    //     }, Self::field_members());
    // }

private:
    // Read one field from proto to self, used for recursion read
    template <class Proto, class FD>
    inline static void read_one(Self& self, const Proto& proto, FD)
    {
        self.*FD::member = FD::policy::template read<Proto>(proto);
    }

    // Write one field from self to proto, used for recursion write
    template <typename Proto, typename FD>
    inline static void write_one(const Self& self, Proto& proto, FD)
    {
        const auto& v = self.*FD::member;
        FD::policy::template write<Proto>(proto, v);
    }
};


// Overload operator<< for debug print
template <typename T>
std::ostream& operator<<(std::ostream& os, const Base<T>& obj) {
    os << obj.debug_string();
    return os;
}


// ========== Helper Macros ==========

#define REGISTER_PB_FIELDS(...)              \
    static constexpr auto field_members()    \
    {                                        \
        return std::make_tuple(__VA_ARGS__); \
    }

// Declarations

#define DECL_PB_FIELD_REQUIRED(T, name) \
    T name;

#define DECL_PB_FIELD_OPTIONAL(T, name) \
    std::optional<T> name;

#define DECL_PB_FIELD_REPEATED(T, name) \
    std::vector<T> name;

// Implementations

#define IMPL_PB_FIELD_REQUIRED(SelfT, T, name)                                \
    struct name##_tag { }; /* distinguish different type impl */              \
    template <>                                                               \
    struct ::proto_bridge::io<T, name##_tag> {                                \
        template <class Proto>                                                \
        static T get_value(const Proto& p) { return p.name(); }               \
        template <class Proto>                                                \
        static void set_value(Proto& p, const T& v) { p.set_##name(v); }      \
        template <class Proto>                                                \
        static const auto& get_message(const Proto& p) { return p.name(); }   \
        template <class Proto>                                                \
        static auto* mutable_message(Proto& p) { return p.mutable_##name(); } \
    };

#define IMPL_PB_FIELD_OPTIONAL(SelfT, T, name)                     \
    IMPL_PB_FIELD_REQUIRED(SelfT, T, name)                         \
    template <>                                                    \
    struct ::proto_bridge::optional_io<T, name##_tag> {            \
        template <class Proto>                                     \
        static bool has(const Proto& p) { return p.has_##name(); } \
        template <class Proto>                                     \
        static void clear(Proto& p) { p.clear_##name(); }          \
    };

#define IMPL_PB_FIELD_REPEATED(SelfT, T, name)                                                   \
    IMPL_PB_FIELD_REQUIRED(SelfT, T, name)                                                       \
    template <>                                                                                  \
    struct ::proto_bridge::repeated_io<T, name##_tag> {                                          \
        template <class Proto>                                                                   \
        static int size(const Proto& p) { return p.name##_size(); }                              \
        template <class Proto>                                                                   \
        static T at_value(const Proto& p, int index) { return p.name(index); }                   \
        template <class Proto>                                                                   \
        static void set_value(Proto& p, int index, const T& v) { p.set_##name(index, v); }       \
        template <class Proto>                                                                   \
        static void add_value(Proto& p, const T& v) { p.add_name(v); }                           \
        template <class Proto>                                                                   \
        static const auto& at_message(const Proto& p, int index) { return p.name(index); }       \
        template <class Proto>                                                                   \
        static auto* mutable_at_message(Proto& p, int index) { return p.mutable_##name(index); } \
        template <class Proto>                                                                   \
        static auto* add_message(Proto& p) { return p.add_##name(); }                            \
        template <class Proto>                                                                   \
        static void clear(Proto& p) { p.clear_##name(); }                                        \
    };
}
