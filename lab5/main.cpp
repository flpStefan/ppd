#include <algorithm>
#include <iostream>
#include <fstream>
#include <map>
#include <mutex>
#include <queue>
#include <random>
#include <semaphore.h>
#include <string>
#include <thread>
#include <iomanip>
#include <atomic>
#include <condition_variable>
using namespace std;

constexpr int countries = 5;
constexpr int problems = 10;
/// Generate INPUT data
void generate_input_data() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist_participants(80, 100);
    uniform_int_distribution<> dist_points(0, 10);

    for (int i = 1; i <= countries; i++) {
        int no_participants = dist_participants(gen);
        int base_id = i * 100;
        for (int j = 1; j <= problems; j++) {
            string file_name = "inputs/RezultateC" + to_string(i) + "_P" + to_string(j);
            ofstream out(file_name);

            for (int k = 1; k <= no_participants; k++) {
                int points = dist_points(gen);
                int current_id = base_id + k;

                if (points != 0)
                    out << i << " " << current_id << " " << points << '\n';
            }
            out.close();
        }
    }
}


/// Thread Safe Linked list
struct Node {
    int country;
    int id;
    int score;
    Node *next;
    mutex node_mutex;

    Node(int country, int id, int score) {
        this->country = country;
        this->id = id;
        this->score = score;
        next = nullptr;
    }
};

class ThreadSafeLinkedList {
    Node *head;
    Node *tail;

public:
    ThreadSafeLinkedList() {
        head = new Node(-1, -1, 0);
        tail = new Node(-1, -1, 0);
        head->next = tail;
    }

    ~ThreadSafeLinkedList() {
        Node *current = head;
        while (current != nullptr) {
            Node *next = current->next;
            delete current;
            current = next;
        }
    }

    void add_or_update(int country_id, int id, int score) {
        Node* prev = head;
        Node* current = head->next;

        while(current != tail) {
            current->node_mutex.lock();
            if(current-> id == id) {
                current->score += score;
                current->node_mutex.unlock();
                return;
            }
            current->node_mutex.unlock();

            prev = current;
            current = current->next;
        }

        Node* new_node = new Node(country_id, id, score);
        new_node->node_mutex.lock();
        prev->node_mutex.lock();

        new_node->next = prev->next;
        prev->next = new_node;

        new_node->node_mutex.unlock();
        prev->node_mutex.unlock();
    }

    void delete_node(int node_id) {
        Node *current = head;

        while (current->next != tail) {
            unique_lock<mutex> lock(current->node_mutex);
            if (current->next->id == node_id) {
                Node *temp = current->next;
                current->next = temp->next;
                if (temp == tail) {
                    tail = current;
                }
                delete temp;
                return;
            }
            current = current->next;
        }
    }

    void sort_list() {
        bool not_sorted = true;
        while(not_sorted == true) {
            Node* current = head->next;
            not_sorted = false;

            while(current -> next != nullptr) {
                if (current->score < current->next->score) {
                    swap(current->score, current->next->score);
                    swap(current->id, current->next->id);
                    swap(current->country, current->next->country);
                    not_sorted = true;
                }
                else if(current->score == current->next->score) {
                    if(current->id > current->next->id) {
                        swap(current->score, current->next->score);
                        swap(current->id, current->next->id);
                        swap(current->country, current->next->country);
                        not_sorted = true;
                    }
                }
                current = current->next;
            }
        }
    }

    void display() {
        ofstream out("output.txt");
        Node *current = head->next;
        while (current != tail) {
            out << current->country << " - " << current->id << " - " << current->score << "\n";
            current = current->next;
        }
        out.close();
    }

    Node *get_head() {
        return head;
    }
};


/// Thread Safe Queue
struct Participant {
    int id, country, score;

    Participant(int id, int country, int score) {
        this->id = id;
        this->country = country;
        this->score = score;
    }
};

class ThreadSafeQueue {
    queue<Participant> q;
    mutex mtx;
    condition_variable not_full, not_empty;
    int max_size;

public:
    ThreadSafeQueue(int size) {
        max_size = size;
    }

    ~ThreadSafeQueue() = default;

    void push(int country, int id, int score) {
        unique_lock<mutex> lock(mtx);
        not_full.wait(lock, [this]() { return q.size() < max_size; });
        q.push(Participant(id, country, score));
        lock.unlock();

        not_empty.notify_one();
    }

    Participant pop() {
        unique_lock<mutex> lock(mtx);
        not_empty.wait(lock, [this]() { return !q.empty(); });
        Participant p = q.front();
        q.pop();
        lock.unlock();

        not_full.notify_one();
        return p;
    }

    bool is_empty() {
        unique_lock<mutex> lock(mtx);
        return q.empty();
    }
};


/// Leaderboard
class Leaderboard {
    ThreadSafeLinkedList list;
    map<int, bool> blacklist;
    mutex mtx;

public:
    Leaderboard() = default;

    ~Leaderboard() = default;

    void update(int country, int id, int score) {
        mtx.lock();
        if (blacklist[id]) {
            mtx.unlock();
            return;
        }
        mtx.unlock();

        if (score == -1) {
            mtx.lock();
            blacklist[id] = true;
            mtx.unlock();
            list.delete_node(id);
            return;
        }

        list.add_or_update(country,id, score);
    }

    bool is_equal_to_list(ThreadSafeLinkedList l) {
        Node *current_list = list.get_head();
        Node *other_list = l.get_head();

        while (current_list != nullptr && other_list != nullptr) {
            if (current_list->score != other_list->score) return false;
            current_list = current_list->next;
            other_list = other_list->next;
        }

        return current_list == other_list;
    }

    void save_to_file() {
        list.sort_list();
        list.display();
    }
};

/// Thread functions
ThreadSafeQueue tqueue(50);
Leaderboard leaderboard;
atomic<int> readers_done = 0;

void reader(int start, int countries, int problems, int step) {
    for(int i = start + 1; i <= countries; i+= step) {
        for(int j = 1; j<= problems; j++) {
            string filename = "inputs/RezultateC" + to_string(i) + "_P" + to_string(j);
            ifstream in(filename);

            if(in.is_open()) {
                int country, id, score;
                while(in >> country >> id >> score) {
                    tqueue.push(country,id,score);
                }
            }

            in.close();
        }
    }

    readers_done.fetch_add(1, memory_order_relaxed);
}

void worker(int no_readers) {
    while (true) {
        if (tqueue.is_empty() && readers_done == no_readers) break;

        if (!tqueue.is_empty()) {
            Participant p = tqueue.pop();
            leaderboard.update(p.country, p.id, p.score);
        }
    }
}

void parallel_run(int no_readers, int no_workers) {
    thread readers[no_readers];
    thread workers[no_workers];

    // Reader
    for (int i = 0; i < no_readers; i++) {
        readers[i] = thread(reader, i, countries, problems, no_readers);
    }

    // Worker
    for (int i = 0; i < no_workers; i++) {
        workers[i] = thread(worker, no_readers);
    }

    //Join threads
    for (int i = 0; i < no_readers; i++)
        readers[i].join();

    for (int i = 0; i < no_workers; i++)
        workers[i].join();

    leaderboard.save_to_file();
}

/// Sequential function
ThreadSafeLinkedList seq_list;

void sequential_run() {
    for (int i = 1; i <= countries; i++) {
        for(int j = 1; j<= problems; j++) {
            string file = "inputs/RezultateC" + to_string(i) + "_P" + to_string(j);
            ifstream in(file);
            int country_id, id, score;

            while (in >> country_id >> id >> score) {
                seq_list.add_or_update(country_id, id, score);
            }

            in.close();
        }

    }
    seq_list.sort_list();
    seq_list.display();
}


int main(int argc, char **argv) {
    // generate_input_data();

    auto start_time = chrono::high_resolution_clock::now();
    int no_readers = atoi(argv[1]);
    int no_workers = atoi(argv[2]);
    if (no_readers > 0) {
        parallel_run(no_readers, no_workers);
    } else {
        sequential_run();
    }
    auto stop_time = chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = stop_time - start_time;
    double final_time = duration.count();

    // cout<< leaderboard.is_equal_to_list(seq_list);
    cout << std::fixed << std::showpoint << std::setprecision(10) << final_time * 1E3;

    return 0;
}