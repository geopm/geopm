test_sleep_region: test_sleep_region.c
	gcc ${GEOPM_CFLAGS} ${GEOPM_CPPFLAGS} ${GEOPM_LDFLAGS} -lgeopmload -lgeopm -lgeopmd -g -O0 $^ -o $@

clean:
	rm -f test_sleep_region

.PHONY: clean
