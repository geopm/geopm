LIBGEOPMD_PATH := $(realpath $(PWD)/../libgeopmd)
CGO_CPPFLAGS := -D_FORTIFY_SOURCE=2
CGO_CFLAGS := -I$(LIBGEOPMD_PATH)/include -pipe -fno-plt -O2
CGO_CXXFLAGS := -pipe -fno-plt -O2
CGO_LDFLAGS := -L$(LIBGEOPMD_PATH)/.libs -fstack-protector-strong -Wl,-O1,--sort-common,--as-needed,-z,relro,-z,now,-z,noexecstack,-z,defs
LD_LIBRARY_PATH := $(LD_LIBRARY_PATH):$(LIBGEOPMD_PATH)/.libs
CGOFLAGS := -trimpath -gcflags="all=-spectre=all -N -l" -asmflags="all=-spectre=all" -buildmode=pie -ldflags="all=-linkmode=external -s -w"

export CGO_CPPFLAGS CGO_CFLAGS CGO_CXXFLAGS CGO_LDFLAGS LD_LIBRARY_PATH

TARGET := geopmread
EXAMPLES_DIR := examples
TEST_FILES := $(wildcard geopmdgo/*_test.go)

.PHONY: all build check clean

all: build

build:
	CGO_ENABLED=1 go build $(CGOFLAGS) $(EXAMPLES_DIR)/$(TARGET).go

check:
	@for TEST_FILE in $(TEST_FILES); do \
		SOURCE_FILE=$$(echo $$TEST_FILE | sed 's|_test\.go$$|.go|'); \
		if [ $$TEST_FILE = geopmdgo/error_test.go ]; then \
			CGO_ENABLED=1 go test $$TEST_FILE $$SOURCE_FILE; \
		else \
			CGO_ENABLED=1 go test $$TEST_FILE $$SOURCE_FILE geopmdgo/error.go; \
		fi; \
	done
	TEMP_FILE=$$(mktemp); \
	./$(TARGET) --help 2>&1 | tee $$TEMP_FILE; \
	test $$(cat $$TEMP_FILE | wc -l) -gt 0; \
	./$(TARGET) > $$TEMP_FILE; \
	test $$(cat $$TEMP_FILE | wc -l) -gt 0; \
	./$(TARGET) TIME board 0 2>&1 | tee $$TEMP_FILE; \
	test $$(cat $$TEMP_FILE | wc -l) -gt 0; \
	rm -f $$TEMP_FILE

clean:
	rm -f $(TARGET)
