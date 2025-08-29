#include "proto_bridge.h"
#include <iostream>
#include <vector>

using proto_bridge::Optional;
using proto_bridge::Repeated;
using proto_bridge::Required;

struct Location3 : proto_bridge::proto_convertible_trait<Location3> {
    using Self = Location3;

    PB_FIELD(Optional<float>, x);
    PB_FIELD(Optional<float>, y);
    PB_FIELD(Optional<float>, z);

    REGISTER_PB_FIELDS(
        PB_FIELD_PTR(x),
        PB_FIELD_PTR(y),
        PB_FIELD_PTR(z))

    template <typename Proto>
    void from_proto_impl(const Proto& p)
    {
        if (p.has_x()) x = p.x();
        if (p.has_y()) y = p.y();
        if (p.has_z()) z = p.z();
    }

    template <typename Proto>
    void to_proto_impl(Proto& p) const
    {
        if (x) p.set_x(*x);
        if (y) p.set_y(*y);
        if (z) p.set_z(*z);
    }
};

struct Apple : proto_bridge::proto_convertible_trait<Apple> {
    using Self = Apple;

    PB_FIELD(proto_bridge::Optional<int>, id);
    PB_FIELD(proto_bridge::Optional<float>, size);
    PB_FIELD(proto_bridge::Optional<Location3>, location);

    REGISTER_PB_FIELDS(
        PB_FIELD_PTR(id),
        PB_FIELD_PTR(size),
        PB_FIELD_PTR(location))

    template <typename Proto>
    void from_proto_impl(const Proto& p)
    {
        if (p.has_id()) {
            id = p.id();
        }
        if (p.has_size()) {
            size = p.size();
        }
        if (p.has_location()) {
            location.emplace(Location3::from_proto(p.location()));
        }
    }

    template <typename Proto>
    void to_proto_impl(Proto& p) const
    {
        if (id) {
            p.set_id(*id);
        }
        if (size) {
            p.set_size(*size);
        }
        if (location) {
            location->to_proto(*p.mutable_location());
        }
    }
};

struct AppleList : proto_bridge::proto_convertible_trait<AppleList> {
    using Self = AppleList;

    PB_FIELD(Repeated<Apple>, list);

    REGISTER_PB_FIELDS(
        PB_FIELD_PTR(list))

    template <typename Proto>
    void from_proto_impl(const Proto& p)
    {
        for (int i = 0; i < p.list_size(); ++i) {
            list.emplace_back().from_proto(p.list(i));
        }
    }

    template <typename Proto>
    void to_proto_impl(Proto& p) const
    {
        for (const auto& apple : list) {
            apple.to_proto(*p.add_list());
        }
    }
};

