//
// Created by lukemartinlogan on 5/14/22.
//

#include <labstor/types/labstack.h>

int main(int argc, char **argv) {
    labstor::LabStack stack;
    for(int i = 1; i < argc; ++i) {
        stack.ModifyLabStack(argv[1]);
    }
}