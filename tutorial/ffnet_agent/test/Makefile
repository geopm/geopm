test: TensorOneDTest.x TensorTwoDTest.x LocalNeuralNetTest.x

TensorOneDTest.x: TensorOneDTest.cpp ../TensorOneD.cpp
	g++ -I../../../googletest-release-1.11.0/googletest/include -I../../../googletest-release-1.11.0/googlemock/include -I../ TensorOneDTest.cpp ../TensorOneD.cpp ../../../gtest-all.o ../../../gmock-all.o -lpthread -o TensorOneDTest.x --coverage -fno-elide-constructors
	./TensorOneDTest.x
	gcov ../TensorOneD.cpp -o .

TensorTwoDTest.x: TensorTwoDTest.cpp
	g++ -I../../../googletest-release-1.11.0/googletest/include -I../../../googletest-release-1.11.0/googlemock/include -I../ TensorTwoDTest.cpp ../TensorTwoD.cpp ../TensorOneD.o ../../../gtest-all.o ../../../gmock-all.o -lpthread -o TensorTwoDTest.x --coverage -fno-elide-constructors
	./TensorTwoDTest.x
	gcov ../TensorTwoD.cpp -o .

LocalNeuralNetTest.x: LocalNeuralNetTest.cpp
	g++ -I../../../googletest-release-1.11.0/googletest/include -I../../../googletest-release-1.11.0/googlemock/include -I../ LocalNeuralNetTest.cpp ../LocalNeuralNet.cpp ../TensorTwoD.o ../TensorOneD.o ../../../gtest-all.o ../../../gmock-all.o -lpthread -o LocalNeuralNetTest.x --coverage -fno-elide-constructors
	./LocalNeuralNetTest.x
	gcov ../LocalNeuralNet.cpp -o .

clean:
	rm -rf TensorOneDTest.x TensorTwoDTest.x *.gc*
