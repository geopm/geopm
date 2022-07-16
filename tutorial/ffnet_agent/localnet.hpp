#include <iostream>

class Vector
{
    public:
        Vector();
        Vector(int n);
        Vector(const Vector&);
        ~Vector();
        void set_dim(int n);
        Vector operator+(const Vector&);
        Vector operator-(const Vector&);
        float operator*(const Vector&);
        Vector& operator=(const Vector&);
        float &operator[](int i);
        float operator[](int i) const;
        Vector sigmoid();

        friend std::ostream & operator << (std::ostream &out, const Vector &v);
        friend std::istream & operator >> (std::istream &in, Vector &v);

    private:
        float *vec;
        int n;
        bool allocated;
};

class Matrix
{
    public:
        Matrix();
        Matrix(int, int);
        Matrix(const Matrix&);
        ~Matrix();
        void set_dim(int r, int c);
        Vector operator*(const Vector&);
        Vector &operator[](int i);
        Vector operator[](int i) const;
        Matrix& operator=(const Matrix&);
        friend std::ostream & operator << (std::ostream &out, const Matrix &m);
        friend std::istream & operator >> (std::istream &in, Matrix &m);

    private:
        Vector *mat;
        int r, c;
        bool allocated;
};

class LocalNeuralNet
{
    public:
        LocalNeuralNet();
        LocalNeuralNet(const LocalNeuralNet&);
        Vector model(Vector inp);
        friend std::ostream & operator << (std::ostream &out, const LocalNeuralNet &n);
        friend std::istream & operator >> (std::istream &in, LocalNeuralNet &n);
        ~LocalNeuralNet();

    private:
        Matrix *weights;
        Vector *biases;
        int nlayers;
        bool allocated;
};

LocalNeuralNet read_nnet(std::string nn_path);
