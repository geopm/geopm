#  Copyright (c) 2015 - 2024, Intel Corporation
#  SPDX-License-Identifier: BSD-3-Clause
#

check_PROGRAMS += test/geopm_test \
                  test/isadmin \
                  # end

test_isadmin_SOURCES = test/isadmin.cpp
test_isadmin_LDADD = libgeopmd.la

TESTS_ENVIRONMENT = PYTHON='$(PYTHON)'

TEST_LOG_DRIVER = env AM_TAP_AWK='$(AWK)' $(SHELL) \
                  $(top_srcdir)/build-aux/tap-driver.sh

TESTS += test/geopm_test.test \
         # end

EXTRA_DIST += test/legacy_allowlist.out \
              test/geopm_test.test \
              # end

test_geopm_test_SOURCES = test/GPUTopoNullTest.cpp \
                          test/AggTest.cpp \
                          test/BatchClientTest.cpp \
                          test/BatchServerTest.cpp \
                          test/BatchStatusTest.cpp \
                          test/CircularBufferTest.cpp \
                          test/CNLIOGroupTest.cpp \
                          test/CombinedSignalTest.cpp \
                          test/ConstConfigIOGroupTest.cpp \
                          test/CpufreqSysfsDriverTest.cpp \
                          test/CpuinfoIOGroupTest.cpp \
                          test/DCGMIOGroupTest.cpp \
                          test/DerivativeSignalTest.cpp \
                          test/DifferenceSignalTest.cpp \
                          test/RatioSignalTest.cpp \
                          test/DomainControlTest.cpp \
                          test/ExceptionTest.cpp \
                          test/geopm_test.cpp \
                          test/geopm_test.hpp \
                          test/geopm_test_helper.cpp \
                          test/GEOPMHintTest.cpp \
                          test/HelperTest.cpp \
                          test/IOGroupTest.cpp \
                          test/IOUringTest.cpp \
                          test/LevelZeroGPUTopoTest.cpp \
                          test/LevelZeroDevicePoolTest.cpp \
                          test/LevelZeroIOGroupTest.cpp \
                          test/MSRIOGroupTest.cpp \
                          test/MSRIOTest.cpp \
                          test/MSRFieldControlTest.cpp \
                          test/MSRFieldSignalTest.cpp \
                          test/MockCpuid.hpp \
                          test/MockGPUTopo.hpp \
                          test/MockBatchClient.hpp \
                          test/MockBatchStatus.hpp \
                          test/MockControl.hpp \
                          test/MockDCGMDevicePool.hpp \
                          test/MockIOGroup.hpp \
                          test/MockIOUring.hpp \
                          test/MockMSRIO.hpp \
                          test/MockNVMLDevicePool.hpp \
                          test/MockLevelZeroDevicePool.hpp \
                          test/MockLevelZero.hpp \
                          test/MockPlatformIO.hpp \
                          test/MockPlatformTopo.cpp \
                          test/MockPlatformTopo.hpp \
                          test/MockSaveControl.hpp \
                          test/MockSDBus.hpp \
                          test/MockSDBusMessage.hpp \
                          test/MockServiceProxy.hpp \
                          test/MockSharedMemory.hpp \
                          test/MockSignal.hpp \
                          test/MockSSTIO.hpp \
                          test/MockSSTIoctl.hpp \
                          test/MockSysfsDriver.hpp \
                          test/MockPOSIXSignal.hpp \
                          test/NVMLGPUTopoTest.cpp \
                          test/NVMLIOGroupTest.cpp \
                          test/POSIXSignalTest.cpp \
                          test/PlatformIOTest.cpp \
                          test/PlatformTopoTest.cpp \
                          test/RawMSRSignalTest.cpp \
                          test/SharedMemoryTest.cpp \
                          test/SaveControlTest.cpp \
                          test/SecurePathTest.cpp \
                          test/ServiceIOGroupTest.cpp \
                          test/ServiceProxyTest.cpp \
                          test/SSTControlTest.cpp \
                          test/SSTIOGroupTest.cpp \
                          test/SSTSignalTest.cpp \
                          test/SSTIOTest.cpp \
                          test/SysfsIOGroupTest.cpp \
                          test/TimeIOGroupTest.cpp \
                          test/UniqueFdTest.cpp \
                          # end

test_geopm_test_LDADD = libgeopmd.la \
                        libgmock.a \
                        libgtest.a \
                        # end

test_geopm_test_CPPFLAGS = $(AM_CPPFLAGS) -Iplugin
test_geopm_test_CFLAGS = $(AM_CFLAGS)
test_geopm_test_CXXFLAGS = $(AM_CXXFLAGS)

init-coverage: all
	lcov --no-external --capture --initial --directory src --output-file coverage-service-initial.info

if ENABLE_COVERAGE
check: init-coverage
endif

coverage: init-coverage check
	lcov --no-external --capture --directory src --output-file coverage-service.info
	lcov -a coverage-service-initial.info -a coverage-service.info --output-file coverage-service-combined.info
	genhtml coverage-service-combined.info --output-directory coverage-service --legend -t $(VERSION) -f

include test/googletest.mk
