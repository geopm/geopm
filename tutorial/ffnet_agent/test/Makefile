TEST_INCLUDES=-I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_INSTALL}/include
TEST_OBJ=gtest-all.o gmock-all.o json11.o
TEST_FLAGS=--coverage -fno-elide-constructors

test: TensorOneDTest.x  TensorTwoDTest.x LocalNeuralNetTest.x

TensorOneDTest.x: TensorOneDTest.cpp ../TensorOneD.cpp ../TensorOneD.hpp $(TEST_OBJ)
	g++ $(TEST_INCLUDES) $(TEST_OBJ) -I../ TensorOneDTest.cpp ../TensorOneD.cpp -lpthread -o TensorOneDTest.x $(TEST_FLAGS)
	./TensorOneDTest.x
	gcov ../TensorOneD.cpp -o .

gmock-all.o:
	g++ -g -O2 -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest -pthread -std=c++11 -c ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/src/gmock-all.cc

gtest-all.o:
	g++ -g -O2 -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest -pthread -std=c++11 -c ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/src/gtest-all.cc

json11.o:
	g++ $(TEST_INCLUDES) -c -o json11.o $(GEOPM_SOURCE)/service/contrib/json11/json11.cpp

clean:
	rm -rf TensorOneDTest.x *.gc*
