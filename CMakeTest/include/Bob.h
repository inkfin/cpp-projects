#ifndef BOB_H
#define BOB_H

#include <string>

struct Bob {
    std::string name;
    int age;
    Bob(std::string name, int age) : name(name), age(age) {
    }

    void print_name();
};

#endif // BOB_H
