SAN_FLAGS = -fsanitize=address,leak,undefined

CFLAGS += \
	-g \
	$(SAN_FLAGS) \
	-DJP_DEBUG \
	-fprofile-arcs \
	-ftest-coverage

# To debug vectorization, add these CFLAGS.
# GCC:
# 	CFLAGS += -fopt-info-vec-optimized -fopt-info-vec-missed -fopt-info-vec-all
# Clang:
# 	CFLAGS += -Rpass=loop-vectorize -Rpass-missed=loop-vectorize -Rpass-analysis=loop-vectorize

# Uncomment to break on assert failure
# CFLAGS += -DJP_DEBUG_BREAK_ON_ASSERT_FAIL

LDFLAGS += $(SAN_FLAGS) -lgcov --coverage
