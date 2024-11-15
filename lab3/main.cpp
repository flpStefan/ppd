#include <stdio.h>
#include <stdlib.h>
#include <mpi.h>
#include <random>
#include <iostream>
#include <fstream>
#include <chrono>

using namespace std;

ifstream fin1("D:\\Facultate\\ppd_lab3\\lab3\\input1.txt");
ifstream fin2("D:\\Facultate\\ppd_lab3\\lab3\\input2.txt");
ofstream fout("D:\\Facultate\\ppd_lab3\\lab3\number.txt");

int a[100015], b[100015], c[100015];

void print(int a[], int n) {
    for (int i = 0; i < n; ++i) {
        cout << a[i] << ' ';
    }
    cout << endl;
}

void write_to_file(int c[], int n) {
    for (int i = 0; i < n; i++) {
        fout << c[i] << ' ';
    }
}

void sequential_run(int n1, int n2, vector<int> a, vector<int> b) {
    int n_max = 0;
    if (n2 > n1) {
        n_max = n2;
        for (int i = n1; i < n2; i++) {
            b[i] = 0;
        }
    }
    else {
        n_max = n1;
        for (int i = n2; i < n1; i++) {
            a[i] = 0;
        }
    }

    int carry = 0;

    for (int i = 0; i < n_max; i++) {
        c[i] = (a[i] + b[i] + carry) % 10;
        carry = (a[i] + b[i] + carry) / 10;
    }
}

int main(int argc, char** argv) {
    int rank, numproc;
    int n1, n2, n;
    int carry = 0;

    fin1 >> n1;
    fin2 >> n2;

    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &numproc);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    int start, end;
    MPI_Status status;

    if (rank == 0) {
        // master code
        auto start_time = std::chrono::high_resolution_clock::now();

        n = max(n1, n2);
        //if (numproc == 1) {
        //    // sequential_run
        //    c = sequential_run(n1, n2, a, b);
        //    return 0;
        //}

        int p = numproc - 1;
        int k = n / p;
        int rest = n % p;
        start = 0;
        end = k;

        for (int id_proces_curent = 1; id_proces_curent < numproc; id_proces_curent++) {
            if (rest > 0) {
                end += 1;
                rest--;
            }

            for (int i = start; i < end; i++) {
                if (i >= n1) {
                    a[i] = 0;
                }
                else fin1 >> a[i];
                //else a[i] = rand() % 10;
                if (i >= n2) {
                    b[i] = 0;
                }
                else fin2 >> b[i];
                //else b[i] = rand() % 10;
            }

            MPI_Send(&start, 1, MPI_INT, id_proces_curent, 0, MPI_COMM_WORLD);
            MPI_Send(&end, 1, MPI_INT, id_proces_curent, 0, MPI_COMM_WORLD);
            MPI_Send(a + start, end - start, MPI_INT, id_proces_curent, 0, MPI_COMM_WORLD);
            MPI_Send(b + start, end - start, MPI_INT, id_proces_curent, 0, MPI_COMM_WORLD);

            start = end;
            end = start + k;

        }

        for (int i = 1; i < numproc; i++) {
            if (i == numproc - 1) {
                MPI_Recv(&carry, 1, MPI_INT, numproc - 1, 0, MPI_COMM_WORLD, &status);
            }

            MPI_Recv(&start, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(&end, 1, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
            MPI_Recv(c + start, end - start, MPI_INT, i, 0, MPI_COMM_WORLD, &status);
        }

        if (carry == 1) {
            c[n] = 1;
            n++;
        }

        auto end_time = std::chrono::high_resolution_clock::now();
        chrono::duration<double> duration = end_time - start_time;
        cout << fixed << showpoint;
        cout << duration.count() * 1E3;

        write_to_file(c, n);
    }
    else {
        // worker code
        MPI_Recv(&start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(&end, 1, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(a + start, end - start, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        MPI_Recv(b + start, end - start, MPI_INT, 0, 0, MPI_COMM_WORLD, &status);
        if (rank > 1) {
            MPI_Recv(&carry, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
        }
        for (int i = start; i < end; i++) {
            c[i] = (a[i] + b[i] + carry) % 10;
            carry = (a[i] + b[i] + carry) / 10;
        }

        if (rank < numproc - 1) {
            MPI_Send(&carry, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
        }
        else {
            MPI_Send(&carry, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        }
        MPI_Send(&start, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(&end, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
        MPI_Send(c + start, end - start, MPI_INT, 0, 0, MPI_COMM_WORLD);
    }

    MPI_Finalize();

    return 0;
//---------------------------------------------------------------------------------//

    //auto start = std::chrono::high_resolution_clock::now();

    //MPI_Init(&argc, &argv);
    //MPI_Comm_size(MPI_COMM_WORLD, &numproc);
    //MPI_Comm_rank(MPI_COMM_WORLD, &rank);

    //MPI_Status status;

    //if (n1 > n2) {
    //    n = n1;
    //}
    //else {
    //    n = n2;
    //}

    //if (rank == 0) {
    //    // master code
    //    for (int i = 0; i < n; i++) {
    //        if (i >= n1) {
    //            a[i] = 0;
    //        }
    //        //else fin1 >> a[i];
    //        else a[i] = rand() % 10;
    //        if (i >= n2) {
    //            b[i] = 0;
    //        }
    //        //else fin2 >> b[i];
    //        else b[i] = rand() % 10;
    //    }
    //}
    //while (n % numproc != 0) {
    //    a[n] = 0;
    //    b[n] = 0;
    //    n++;
    //}

    //int chunk_size = n / numproc;
    //int* auxa = new int[chunk_size];
    //int* auxb = new int[chunk_size];
    //int* auxc = new int[chunk_size];

    //MPI_Scatter(a, chunk_size, MPI_INT, auxa, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);
    //MPI_Scatter(b, chunk_size, MPI_INT, auxb, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    //if (rank > 0) {
    //    MPI_Recv(&carry, 1, MPI_INT, rank - 1, 0, MPI_COMM_WORLD, &status);
    //}

    //for (int i = 0; i < chunk_size; i++) {
    //    auxc[i] = (auxa[i] + auxb[i] + carry) % 10;
    //    carry = (auxa[i] + auxb[i] + carry) / 10;
    //}
    //
    //if (rank < numproc - 1) {
    //    MPI_Send(&carry, 1, MPI_INT, rank + 1, 0, MPI_COMM_WORLD);
    //}
    //else {
    //    MPI_Send(&carry, 1, MPI_INT, 0, 0, MPI_COMM_WORLD);
    //}

    //MPI_Gather(auxc, chunk_size, MPI_INT, c, chunk_size, MPI_INT, 0, MPI_COMM_WORLD);

    //if (rank == 0) {
    //    MPI_Recv(&carry, 1, MPI_INT, numproc - 1, 0, MPI_COMM_WORLD, &status);

    //    if (carry) {
    //        c[n++] = 1;
    //    }

    //    auto end = std::chrono::high_resolution_clock::now();
    //    chrono::duration<double> duration = end - start;
    //    cout << fixed << showpoint;
    //    cout << duration.count() * 1E3;

    //    //print(a, n);
    //    //print(b, n);
    //    //print(c, n);
    //    write_to_file(c, n);
    //}

    //MPI_Finalize();

    //return 0;

}