#include "runtime.hpp"

int main(int argc, char** argv) {
    tw::Runtime runtime(argc, argv);
    runtime.run();

    return 0;
}
