#include <iostream>
#include "test.pb.h"
#include "shape.h"

int main(int, char**) {
    // build a sample message
    Test::Location3 loc_proto;
    loc_proto.set_x(1.0);
    loc_proto.set_y(2.0);
    loc_proto.set_z(3.0);

    std::cout << "protobuf location descriptor: " << loc_proto.GetDescriptor() << "\n";

    {
        auto loc_cpp = Location3::from_proto(loc_proto);

        std::cout << "Test Location3 debug_string():\n" << loc_cpp << "\n";

        Test::Location3 new_loc_proto;
        loc_cpp.to_proto(new_loc_proto);

        // std::cout << "Convert to proto from Location3:\n" << new_loc_proto.DebugString() << "\n";
    }

    Test::Apple apple_proto;
    apple_proto.set_id(123);
    apple_proto.set_size(3.14159);
    auto* mutloc = apple_proto.mutable_location();
    mutloc->CopyFrom(loc_proto);

    std::cout << "Load an apple:\n" << apple_proto.DebugString() << "\n";

    {
        auto apple_cpp = Apple::from_proto(apple_proto);

        std::cout << "Test Apple debug_string():\n";
        std::cout << apple_cpp.debug_string() << "\n";

        Test::Apple new_apple_proto;
        apple_cpp.to_proto(new_apple_proto);

        // std::cout << "Convert to proto from Apple:\n" << new_apple_proto.DebugString() << "\n";
    }

    Test::AppleList apple_list_proto;
    apple_list_proto.add_list()->CopyFrom(apple_proto); // 1
    apple_list_proto.clear_list(); // 0
    apple_list_proto.add_list()->CopyFrom(apple_proto); // 1
    apple_list_proto.add_list()->CopyFrom(apple_proto);
    apple_list_proto.add_list()->CopyFrom(apple_proto);
    apple_list_proto.add_list()->CopyFrom(apple_proto);
    apple_list_proto.add_list()->CopyFrom(apple_proto); // 5

    std::cout << "Load an apple list\n";
    std::cout << "  size of apple list: " << apple_list_proto.list().size() << "\n";

    {
        auto apple_list_cpp = AppleList::from_proto(apple_list_proto);

        std::cout << "Test AppleList debug_string():\n";
        std::cout << apple_list_cpp.debug_string() << "\n";

        Test::AppleList new_apple_list_proto;
        apple_list_cpp.to_proto(new_apple_list_proto);

        std::cout << "Convert to proto from AppleList:\n" << new_apple_list_proto.DebugString() << "\n";
    }

    return 0;
}
