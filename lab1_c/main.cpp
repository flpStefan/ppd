#include <chrono>
#include <iostream>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <thread>
#define LIMIT 10
#define THREAD_MODE "rows"
using namespace std;

ifstream f("D:\\Facultate\\ppd\\lab1\\lab1_c\\input.txt");
int rows, cols;
int convolusion_rows, convolusion_cols;
int no_threads;
int result[10000][10000];
int matrix[10000][10000], convolusion[5][10000];
//vector<vector<int> > matrix, convolusion;
double final_time;


class LineThread {
    int start, stop;

public:
    LineThread(int start, int stop) : start(start), stop(stop) {
    }

    void operator()() const {
        for (int i = start; i < stop; i++) {
            for (int j = 0; j < cols; j++) {
                for (int convolusion_i = i - convolusion_rows / 2; convolusion_i <= i + convolusion_rows / 2;
                     convolusion_i
                     ++) {
                    int final_i;
                    if (convolusion_i < 0) final_i = 0;
                    else if (convolusion_i >= rows) final_i = rows - 1;
                    else final_i = convolusion_i;

                    for (int convolusion_j = j - convolusion_cols / 2; convolusion_j <= j + convolusion_cols / 2;
                         convolusion_j++) {
                        int final_j;
                        if (convolusion_j < 0) final_j = 0;
                        else if (convolusion_j >= cols) final_j = cols - 1;
                        else final_j = convolusion_j;

                        result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2]
                        [
                            convolusion_j - j + convolusion_cols / 2];
                    }
                }
            }
        }
    }
};

class ColumnThread {
    int start, stop;

public:
    ColumnThread(int start, int stop) : start(start), stop(stop) {
    }

    void operator()() const {
        for (int i = 0; i < rows; i++) {
            for (int j = start; j < stop; j++) {
                for (int convolusion_i = i - convolusion_rows / 2; convolusion_i <= i + convolusion_rows / 2;
                     convolusion_i
                     ++) {
                    int final_i;
                    if (convolusion_i < 0) final_i = 0;
                    else if (convolusion_i >= rows) final_i = rows - 1;
                    else final_i = convolusion_i;

                    for (int convolusion_j = j - convolusion_cols / 2; convolusion_j <= j + convolusion_cols / 2;
                         convolusion_j++) {
                        int final_j;
                        if (convolusion_j < 0) final_j = 0;
                        else if (convolusion_j >= cols) final_j = cols - 1;
                        else final_j = convolusion_j;

                        result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2]
                        [
                            convolusion_j - j + convolusion_cols / 2];
                    }
                }
            }
        }
    }
};

void generate_matrix_static(int rows, int cols, int limit, int ma[][10000]) {
    random_device random_device;
    mt19937 gen(random_device());
    uniform_int_distribution distr(0, limit);

    for (int i = 0; i < rows; i++)
        for (int j = 0; j < cols; j++) ma[i][j] = distr(gen);
}

vector<vector<int> > generate_matrix_dynamic(int rows, int cols, int limit) {
    random_device random_device;
    mt19937 gen(random_device());
    uniform_int_distribution distr(0, limit);

    vector<vector<int> > ma;
    for (int i = 0; i < rows; i++) {
        vector<int> row;
        for (int j = 0; j < cols; j++) row.push_back(distr(gen));
        ma.push_back(row);
    }

    return ma;
}

void secvential() {
    auto start = chrono::high_resolution_clock::now();

    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            for (int convolusion_i = i - convolusion_rows / 2; convolusion_i <= i + convolusion_rows / 2; convolusion_i
                 ++) {
                int final_i;
                if (convolusion_i < 0) final_i = 0;
                else if (convolusion_i >= rows) final_i = rows - 1;
                else final_i = convolusion_i;

                for (int convolusion_j = j - convolusion_cols / 2; convolusion_j <= j + convolusion_cols / 2;
                     convolusion_j++) {
                    int final_j;
                    if (convolusion_j < 0) final_j = 0;
                    else if (convolusion_j >= cols) final_j = cols - 1;
                    else final_j = convolusion_j;

                    result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2][
                        convolusion_j - j + convolusion_cols / 2];
                }
            }
        }
    }
    auto stop = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop - start);
    final_time = duration.count() / 1000.0;
}

void rows_thread_run() {
    auto start_time = chrono::high_resolution_clock::now();

    int start = 0;
    int end = rows / no_threads;
    int rest = rows % no_threads;
    int step = rows / no_threads;
    thread threads[no_threads];

    for (int i = 0; i < no_threads; i++) {
        if (rest > 0) {
            end++;
            rest--;
        }

        threads[i] = thread(LineThread(start, end));
        start = end;
        end = start + step;
    }

    for (int i = 0; i < no_threads; i++)
        threads[i].join();
    auto stop_time = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time);
    final_time = duration.count() / 1000.0;
}

void column_thread_run() {
    auto start_time = chrono::high_resolution_clock::now();

    int start = 0;
    int end = cols / no_threads;
    int rest = cols % no_threads;
    int step = cols / no_threads;
    thread threads[no_threads];

    for (int i = 0; i < no_threads; i++) {
        if (rest > 0) {
            end++;
            rest--;
        }

        threads[i] = thread(ColumnThread(start, end));
        start = end;
        end = start + step;
    }

    for (int i = 0; i < no_threads; i++)
        threads[i].join();

    auto stop_time = chrono::high_resolution_clock::now();
    auto duration = std::chrono::duration_cast<std::chrono::microseconds>(stop_time - start_time);
    final_time = duration.count() / 1000.0;
}

int main(int argc, char *argv[]) {
    f >> rows >> cols;
    f >> convolusion_rows >> convolusion_cols;
    no_threads = atoi(argv[1]);


    generate_matrix_static(rows, cols, LIMIT, matrix);
    generate_matrix_static(convolusion_rows, convolusion_cols, LIMIT, convolusion);

    // matrix = generate_matrix_dynamic(rows, cols, LIMIT);
    // convolusion = generate_matrix_dynamic(convolusion_rows, convolusion_cols, LIMIT);


    if (no_threads == 0) {
        secvential();

        for(int i = 0; i < rows; i++) {
            for(int j = 0; j < cols; j++) {
                cout<<result[i][j]<<" ";
            }
            cout<<endl;
        }
    }
    else {
        if (THREAD_MODE == "rows")
            rows_thread_run();
        else
            column_thread_run();
    }

    cout << final_time;
}
