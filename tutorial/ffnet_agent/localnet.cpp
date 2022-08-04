#include <fstream>
#include <math.h>

#include "localnet.hpp"

// TODO
// add constructors
// add destructors
// add #define to .hpp
// remove torch depends
// -- honestly this should be a compile-time flag
//
Vector::Vector() {
    this->allocated = false;
}

Vector::Vector(int n) {
    this->allocated = false;
    this->set_dim(n);
}

Vector::Vector(const Vector &that) {
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.n);
        for (int i=0; i<n; i++) {
            this->vec[i] = that[i];
        }
    }
}

Vector::~Vector() {
    if (this->allocated) {
        delete[] this->vec;
        this->allocated = false;
    }
}

void
Vector::set_dim(int n) {
    if (this->allocated) {
        delete[] this->vec;
    }
    this->vec = new float[n];
    this->n = n;
    this->allocated = true;
}

Vector
Vector::operator+(const Vector& that) {
    Vector rval(this->n);
    // assert(this->n == that.n);
    for (int i=0; i<n; i++) {
        rval[i] = this->vec[i] + that.vec[i];
    }

    return rval;
}

Vector
Vector::operator-(const Vector& that) {
    Vector rval(this->n);
    for (int i=0; i<this->n; i++) {
        rval[i] = this->vec[i] - that.vec[i];
    }

    return rval;
}


float
Vector::operator*(const Vector& that) {
    float rval = 0;
    // assert(this->n == that.n);
    for (int i=0; i<this->n; i++) {
        rval += this->vec[i] * that.vec[i];
    }

    return rval;
}

Vector&
Vector::operator=(const Vector &that) {
    if (this->allocated) {
        delete[] this->vec;
    }
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.n);
        for (int i=0; i<n; i++) {
            this->vec[i] = that[i];
        }
    }
}

float&
Vector::operator[] (int i) {
    return this->vec[i];
}

float
Vector::operator[] (int i) const {
    return this->vec[i];
}

Vector
Vector::sigmoid() {
    Vector rval;
    rval.set_dim(n);
    for(int i=0; i<n; i++) {
        rval[i] = 1/(1 + expf(-(this->vec[i])));
    }
    return rval;
}

std::ostream & operator << (std::ostream &out, const Vector &v) {
    out << "1 " << v.n << " ";

    for (int i=0; i<v.n; i++) {
        out << v[i] << " ";
    }
    return out;
}

std::istream & operator >> (std::istream &in, Vector &v) {
    int one, n;
    in >> one >> n;
    v.set_dim(n);
    for (int i=0; i<n; i++) {
        in >> v[i];
    }
    return in;
}

Matrix::Matrix() {
    this->allocated = false;
}

Matrix::Matrix(int r, int c) {
    this->allocated = false;
    this->set_dim(r, c);
}

Matrix::Matrix(const Matrix &that) {
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.r, that.c);
        for (int i=0; i<this->r; i++) {
            for (int j=0; j<this->c; j++) {
                this->mat[i][j] = that.mat[i][j];
            }
        }
    }
}

Matrix::~Matrix() {
    if (this->allocated) {
        delete[] this->mat;
        this->allocated = false;
    }
}

void
Matrix::set_dim(int r, int c) {
    if (this->allocated) {
        delete[] this->mat;
    }
    this->r = r;
    this->c = c;
    this->mat = new Vector[r];
    for (int i=0; i<r; i++) {
        this->mat[i].set_dim(c);
    }
    this->allocated = true;
}

Vector
Matrix::operator*(const Vector& that) {
    Vector rval(this->r);
    for (int i=0; i<this->r; i++) {
        rval[i] = this->mat[i] * that;
    }
    return rval;
}

Vector&
Matrix::operator[](int i) {
    return this->mat[i];
}

Vector
Matrix::operator[](int i) const {
    return this->mat[i];
}

Matrix&
Matrix::operator=(const Matrix &that) {
    if (this->allocated) {
        delete[] this->mat;
    }
    this->allocated = false;
    if (that.allocated) {
        this->set_dim(that.r, that.c);
        for (int i=0; i<r; i++) {
            for (int j=0; j<c; j++) {
                this->mat[i][j] = that.mat[i][j];
            }
        }
    }
}


std::ostream & operator << (std::ostream &out, const Matrix &m) {
    out << "2 " << m.r << " " << m.c << " ";
    for (int i=0; i<m.r; i++) {
        for (int j=0; j<m.c; j++) {
            out << m[i][j] << " ";
        }
    }
    return out;
}

std::istream & operator >> (std::istream &in, Matrix &m) {
    int two, r, c;
    in >> two >> r >> c;
    m.set_dim(r, c);
    for (int i=0; i<r; i++) {
        for (int j=0; j<c; j++) {
            in >> m[i][j];
        }
    }
    return in;
}

LocalNeuralNet::LocalNeuralNet() {
}

LocalNeuralNet::LocalNeuralNet(const LocalNeuralNet &that) {
    this->allocated = that.allocated;
    if (that.allocated) {
        this->nlayers = that.nlayers;
        this->weights = new Matrix[this->nlayers];
        this->biases = new Vector[this->nlayers];
        for (int i=0; i<this->nlayers; i++) {
            this->weights = that.weights;
            this->biases = that.biases;
        }
    }
}

Vector
LocalNeuralNet::model(Vector inp) {
    Vector tmp;

    tmp = inp;
    for (int i=0; i<this->nlayers; i++) {
        tmp = this->weights[i] * tmp;
        tmp = tmp + this->biases[i];


        if (i != nlayers - 1) {
            tmp = tmp.sigmoid();
        }
    }

    return tmp;
}

std::ostream & operator << (std::ostream &out, const LocalNeuralNet &n) {
    out << 2*n.nlayers << std::endl;
    for (int i=0; i<n.nlayers; i++) {
        out << n.weights[i] << std::endl;
        out << n.biases[i] << std::endl;
    }
    return out;
}

std::istream & operator >> (std::istream &in, LocalNeuralNet &n) {
    int soup;
    in >> n.nlayers;
    n.nlayers /= 2;
    n.weights = new Matrix[n.nlayers];
    n.biases = new Vector[n.nlayers];
    for (int i=0; i<n.nlayers; i++) {
        in >> n.weights[i];
        in >> n.biases[i];
    }
    n.allocated = true;
    return in;
}

LocalNeuralNet::~LocalNeuralNet() {
    if (allocated && false) {
        delete[] this->weights;
        delete[] this->biases;
    }
}

LocalNeuralNet read_nnet(std::string nn_path) {
    std::ifstream inp(nn_path);
    LocalNeuralNet lnn;
    inp >> lnn;
    return lnn;
}
