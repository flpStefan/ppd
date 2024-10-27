#include <chrono>
#include <condition_variable>
#include <iostream>
#include <iomanip>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <thread>
#include <algorithm>
#define LIMIT 10
using namespace std;

ifstream f("D:\\Facultate\\ppd\\lab2\\lab2_c\\input.txt");
const int convolusion_rows = 3, convolusion_cols = 3;

int rows, cols;
int no_threads;
vector<vector<int> > matrix, convolusion;
double final_time;


class MyBarrier {
    mutex m;
    condition_variable cv;
    int counter;
    int waiting;

public:
    MyBarrier(): counter(0), waiting(0) {
    }

    void wait() {
        unique_lock<mutex> lock(m);

        counter++;
        waiting++;
        cv.wait(lock, [&] { return counter >= no_threads; });
        cv.notify_one();
        waiting--;
        if (waiting == 0)
            counter = 0;

        lock.unlock();
    }
};

class MyThread {
    int start, end;
    MyBarrier &barrier;

public:
    MyThread(int start, int end, MyBarrier &barrier): start(start), end(end), barrier(barrier) {
    }

    void operator()() const {
        int aux[3][cols];

        int row_above = start > 0 ? (start - 1) : start;
        int row_below = end < rows - 1 ? end : rows - 1;
        for (int i = 0; i < cols; i++) {
            aux[0][i] = matrix[row_above][i];
            aux[2][i] = matrix[row_below][i];
        }
        barrier.wait();

        bool isNextRow = false;
        for (int i = start; i < end - 1; i++) {
            if (!isNextRow) {
                for (int j = 0; j < cols; j++) {
                    aux[1][j] = matrix[i][j];
                    if (j + 1 < cols) aux[1][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for (int k = 0; k < 3; k++) {
                        int final_index = j + k - 1;
                        if (final_index < 0) final_index = 0;
                        if (final_index >= cols) final_index = cols - 1;

                        sum += convolusion[0][k] * aux[0][final_index];
                        sum += convolusion[1][k] * aux[1][final_index];
                        sum += convolusion[2][k] * matrix[i + 1][final_index];
                    }
                    matrix[i][j] = sum;
                }
            } else {
                for (int j = 0; j < cols; j++) {
                    aux[0][j] = matrix[i][j];
                    if (j + 1 < cols) aux[0][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for (int k = 0; k < 3; k++) {
                        int final_index = j + k - 1;
                        if (final_index < 0) final_index = 0;
                        if (final_index >= cols) final_index = cols - 1;

                        sum += convolusion[0][k] * aux[1][final_index];
                        sum += convolusion[1][k] * aux[0][final_index];
                        sum += convolusion[2][k] * matrix[i + 1][final_index];
                    }
                    matrix[i][j] = sum;
                }
            }

            isNextRow = !isNextRow;
        }

        int first_index = isNextRow ? 1 : 0;
        int second_index = isNextRow ? 0 : 1;
        for (int j = 0; j < cols; j++) {
            aux[second_index][j] = matrix[end - 1][j];
        }

        for (int j = 0; j < cols; j++) {
            int sum = 0;
            for (int k = 0; k < 3; k++) {
                int final_index = j + k - 1;
                if (final_index < 0) final_index = 0;
                if (final_index >= cols) final_index = cols - 1;

                sum += convolusion[0][k] * aux[first_index][final_index];
                sum += convolusion[1][k] * aux[second_index][final_index];
                sum += convolusion[2][k] * aux[2][final_index];
            }

            matrix[end - 1][j] = sum;
        }
    }
};


vector<vector<int> > generate_matrix(int rows, int cols, int limit) {
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

void sequential_run() {
    int aux[3][cols];
    for (int i = 0; i < cols; i++) {
        aux[0][i] = matrix[0][i];
        aux[2][i] = matrix[rows - 1][i];
    }

    bool isNextRow = false;
    for (int i = 0; i < rows - 1; i++) {
        if (!isNextRow) {
            for (int j = 0; j < cols; j++) {
                aux[1][j] = matrix[i][j];
                if (j + 1 < cols) aux[1][j + 1] = matrix[i][j + 1];

                int sum = 0;
                for (int k = 0; k < 3; k++) {
                    int final_index = j + k - 1;
                    if (final_index < 0) final_index = 0;
                    if (final_index >= cols) final_index = cols - 1;

                    sum += convolusion[0][k] * aux[0][final_index];
                    sum += convolusion[1][k] * aux[1][final_index];
                    sum += convolusion[2][k] * matrix[i + 1][final_index];
                }
                matrix[i][j] = sum;
            }
        } else {
            for (int j = 0; j < cols; j++) {
                aux[0][j] = matrix[i][j];
                if (j + 1 < cols) aux[0][j + 1] = matrix[i][j + 1];

                int sum = 0;
                for (int k = 0; k < 3; k++) {
                    int final_index = j + k - 1;
                    if (final_index < 0) final_index = 0;
                    if (final_index >= cols) final_index = cols - 1;

                    sum += convolusion[0][k] * aux[1][final_index];
                    sum += convolusion[1][k] * aux[0][final_index];
                    sum += convolusion[2][k] * matrix[i + 1][final_index];
                }
                matrix[i][j] = sum;
            }
        }

        isNextRow = !isNextRow;
    }

    for (int j = 0; j < cols; j++) {
        int sum = 0;
        for (int k = 0; k < 3; k++) {
            int final_index = j + k - 1;
            if (final_index < 0) final_index = 0;
            if (final_index >= cols) final_index = cols - 1;

            sum += convolusion[0][k] * aux[isNextRow ? 1 : 0][final_index];
            sum += convolusion[1][k] * aux[2][final_index];
            sum += convolusion[2][k] * aux[2][final_index];
        }

        matrix[rows - 1][j] = sum;
    }
}

void thread_run() {
    MyBarrier barrier;
    thread threads[no_threads];

    int start = 0;
    int end = rows / no_threads;
    int rest = rows % no_threads;
    int step = rows / no_threads;

    for (int i = 0; i < no_threads; i++) {
        if (rest > 0) {
            end++;
            rest--;
        }
        threads[i] = thread(MyThread(start, end, barrier));

        start = end;
        end = start + step;
    }

    for (int i = 0; i < no_threads; i++)
        threads[i].join();
}

int main(int argc, char *argv[]) {
    f >> rows >> cols;
    no_threads = atoi(argv[1]);

    matrix = generate_matrix(rows, cols, LIMIT);
    convolusion = generate_matrix(convolusion_rows, convolusion_cols, LIMIT);


    auto start_time = chrono::high_resolution_clock::now();
    if (no_threads == 0) {
        sequential_run();
    } else {
        thread_run();
    }
    auto stop_time = chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = stop_time - start_time;
    final_time = duration.count();


    cout << std::fixed << std::showpoint << std::setprecision(10) << final_time * 1E3;
}
