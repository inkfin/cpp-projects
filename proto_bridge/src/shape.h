#include "proto_bridge.h"

template <class T>
using PBBase = proto_bridge::Base<T>;

IMPL_PB_FIELD_OPTIONAL(Location3, float, x);
IMPL_PB_FIELD_OPTIONAL(Location3, float, y);
IMPL_PB_FIELD_OPTIONAL(Location3, float, z);

struct Location3 : PBBase<Location3> {
    using Self = Location3;
    // using proto_bridge::Base<Location3>::operator=;

    std::optional<float> x;
    std::optional<float> y;
    std::optional<float> z;

    REGISTER_PB_FIELDS(
        PB_FIELD_OPTIONAL(Self, x, float),
        PB_FIELD_OPTIONAL(Self, y, float),
        PB_FIELD_OPTIONAL(Self, z, float))
};

IMPL_PB_FIELD_OPTIONAL(Apple, int, id);
IMPL_PB_FIELD_OPTIONAL(Apple, float, size);
IMPL_PB_FIELD_OPTIONAL(Apple, Location3, location);

struct Apple : PBBase<Apple> {
    using Self = Apple;

    std::optional<int> id;
    std::optional<float> size;
    std::optional<Location3> location;

    REGISTER_PB_FIELDS(
        PB_FIELD_OPTIONAL(Self, id, int),
        PB_FIELD_OPTIONAL(Self, size, float),
        PB_FIELD_OPTIONAL(Self, location, Location3))
};


IMPL_PB_FIELD_REPEATED(AppleList, Apple, list);

struct AppleList : PBBase<AppleList> {
    using Self = AppleList;

    std::vector<Apple> list;

    REGISTER_PB_FIELDS(
        PB_FIELD_REPEATED(Self, list, Apple))
};

