#include "localnet.hpp"
#include <fstream>

int
main(int argc, char** argv) {
    std::ifstream test_in;
    LocalNeuralNet lnn = read_nnet(argv[1]);

    test_in.open(argv[2]);

    for (int i=0; i<10; i++) {
        std::cout << "example " << i << std::endl;
        Vector tin, tout;
        test_in >> tin >> tout;
        Vector modelout(lnn.model(tin));
        std::cout << tin << std::endl;
        std::cout << tout << std::endl;
        std::cout << modelout << std::endl;
        float dist = (modelout - tout)*(modelout - tout);
        if (dist > 1e-18) {
            std::cout << "output didn't match reference: " << i << " dist = " << dist << std::endl;
        } else {
            std::cout << "error within tolerance: " << dist << " dist = " << dist << std::endl;
        }
        std::cout << std::endl;
    }

    test_in.close();

    return 0;
}
