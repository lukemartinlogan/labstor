
LABSTOR_ROOT=$1

cpplint --recursive ${LABSTOR_ROOT}/src --recursive ${LABSTOR_ROOT}/include --recursive ${LABSTOR_ROOT}/test \
--exclude ${LABSTOR_ROOT}/constants/singleton_macros.h \
--exclude ${LABSTOR_ROOT}/include/labstor/singleton_macros.h \
--exclude ${LABSTOR_ROOT}/include/labstor/errors.h