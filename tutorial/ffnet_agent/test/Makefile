TEST_INCLUDES=-I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_INSTALL}/include
TEST_OBJ=gtest-all.o gmock-all.o
TEST_FLAGS=--coverage -fno-elide-constructors

test: TensorOneDTest.x  TensorTwoDTest.x LocalNeuralNetTest.x

TensorOneDTest.x: TensorOneDTest.cpp ../TensorOneD.cpp ../TensorOneD.hpp json11.o $(TEST_OBJ)
	g++ $(TEST_INCLUDES) $(TEST_OBJ) -I../ TensorOneDTest.cpp json11.o ../TensorOneD.cpp -lpthread -o TensorOneDTest.x $(TEST_FLAGS)
	./TensorOneDTest.x
	gcov ../TensorOneD.cpp -o .

TensorTwoDTest.x: TensorTwoDTest.cpp ../TensorTwoD.cpp ../TensorOneD.cpp ../TensorTwoD.hpp ../TensorOneD.hpp json11.o $(TEST_OBJ)
	g++ $(TEST_INCLUDES) -I../ TensorTwoDTest.cpp json11.o ../TensorTwoD.cpp ../TensorOneD.cpp $(TEST_OBJ) -lpthread -o TensorTwoDTest.x $(TEST_FLAGS)
	./TensorTwoDTest.x
	gcov ../TensorTwoD.cpp -o .

LocalNeuralNetTest.x: LocalNeuralNetTest.cpp ../LocalNeuralNet.cpp ../TensorTwoD.cpp ../TensorOneD.cpp ../LocalNeuralNet.hpp ../TensorTwoD.hpp ../TensorOneD.hpp
	g++ $(TEST_INCLUDES) -I../ LocalNeuralNetTest.cpp json11.o ../LocalNeuralNet.cpp ../TensorTwoD.cpp ../TensorOneD.cpp $(TEST_OBJ) -lpthread -o LocalNeuralNetTest.x $(TEST_FLAGS) 
	./LocalNeuralNetTest.x
	gcov ../LocalNeuralNet.cpp -o .

gmock-all.o:
	g++ -g -O2 -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest -pthread -std=c++11 -c ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/src/gmock-all.cc

gtest-all.o:
	g++ -g -O2 -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googlemock -isystem ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/include -I${GEOPM_SOURCE}/googletest-release-1.11.0/googletest -pthread -std=c++11 -c ${GEOPM_SOURCE}/googletest-release-1.11.0/googletest/src/gtest-all.cc

json11.o:
	g++ $(TEST_INCLUDES) -c -o json11.o $(GEOPM_SOURCE)/service/contrib/json11/json11.cpp

clean:
	rm -rf TensorOneDTest.x TensorTwoDTest.x LocalNeuralNetTest.x *.gc*
