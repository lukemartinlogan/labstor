#!/bin/bash

LABSTOR_ROOT=$1

cpplint --recursive \
--exclude="${LABSTOR_ROOT}/include/labstor/constants/singleton_macros.h" \
--exclude="${LABSTOR_ROOT}/include/labstor/constants/data_structure_singleton_macros.h" \
--exclude="${LABSTOR_ROOT}/src/singleton.cc" \
--exclude="${LABSTOR_ROOT}/src/data_structure_singleton.cc" \
--exclude="${LABSTOR_ROOT}/include/labstor/util/formatter.h" \
--exclude="${LABSTOR_ROOT}/include/labstor/util/errors.h" \
"${LABSTOR_ROOT}/src" "${LABSTOR_ROOT}/include" "${LABSTOR_ROOT}/test"