
LABSTOR_ROOT=$1

cpplint --recursive \
--exclude="${LABSTOR_ROOT}/include/labstor/constants/singleton_macros.h" \
--exclude="${LABSTOR_ROOT}/src/singleton.cc" \
--exclude="${LABSTOR_ROOT}/include/labstor/util/formatter.h" \
--exclude="${LABSTOR_ROOT}/include/labstor/errors.h" \
"${LABSTOR_ROOT}/src" "${LABSTOR_ROOT}/include" "${LABSTOR_ROOT}/test"