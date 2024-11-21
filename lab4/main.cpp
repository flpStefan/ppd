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
using namespace std;


/// Generate INPUT data
vector<string> files;

void generate_input_data() {
    random_device rd;
    mt19937 gen(rd());
    uniform_int_distribution<> dist_participants(80, 100);
    uniform_int_distribution<> dist_points(0, 10);

    for (int i = 1; i <= 5; i++) {
        int no_participants = dist_participants(gen);
        int base_id = i * 100;
        for (int j = 1; j <= 10; j++) {
            string file_name = "inputs/RezultateC" + to_string(i) + "_P" + to_string(j);
            ofstream out(file_name);
            files.push_back(file_name);

            for (int k = 1; k <= no_participants; k++) {
                int points = dist_points(gen);
                int current_id = base_id + k;

                if (points != 0)
                    out << current_id << " " << points << '\n';
            }
            out.close();
        }
    }
}

void load_file_names() {
    for (int i = 1; i <= 5; i++) {
        for (int j = 1; j <= 10; j++) {
            string file_name = "inputs/RezultateC" + to_string(i) + "_P" + to_string(j);
            files.push_back(file_name);
        }
    }
}


/// Linked list
struct Node {
    pair<int, int> data;
    Node *next;
    Node *prev;

    Node(pair<int, int> value) {
        data = value;
        next = nullptr;
        prev = nullptr;
    }
};

class LinkedList {
    Node *head;

public:
    LinkedList() {
        head = nullptr;
    }

    ~LinkedList() {
        Node *current = head;
        while (current != nullptr) {
            Node *next = current->next;
            delete current;
            current = next;
        }
    }

    void add(pair<int, int> value) {
        Node *node = new Node(value);
        if (head == nullptr || head->data.second < node->data.second) {
            if (head != nullptr) head->prev = node;
            node->next = head;
            head = node;
            return;
        }

        Node *current = head;
        while (current->next != nullptr && current->next->data.second > node->data.second) {
            current = current->next;
        }

        node->next = current->next;
        node->prev = current;
        if (current->next != nullptr) {
            current->next->prev = node;
        }
        current->next = node;
    }

    void update_node(pair<int, int> value) {
        Node *current = head;
        while (current != nullptr) {
            if (current->data.first == value.first) break;
            current = current->next;
        }

        if(current == nullptr) return;

        current->data.second += value.second;
        while(current->prev != nullptr && current->data.second > current->prev->data.second) {
            swap(current->data, current->prev->data);
            current = current->prev;
        }
    }

    bool find(int node_id) {
        Node *current = head;
        while (current != nullptr) {
            if (current->data.first == node_id) return true;

            current = current->next;
        }

        return false;
    }

    void delete_node(int node_id) {
        if (head == nullptr) return;

        if (head->data.first == node_id) {
            Node *temp = head;
            head = head->next;
            if (head != nullptr) {
                head->prev = nullptr;
            }
            delete temp;
            return;
        }

        Node *current = head;
        while (current->next != nullptr && current->next->data.first != node_id) {
            current = current->next;
        }

        if (current->next == nullptr) return;

        Node *temp = current->next;
        current->next = current->next->next;
        if (current->next != nullptr) {
            current->next->prev = current;
        }
        delete temp;
    }

    void display() {
        ofstream out("output.txt");
        Node *current = head;
        while (current != nullptr) {
            out << current->data.first << " - " << current->data.second << "\n";
            current = current->next;
        }
        out.close();
    }

    Node* get_head() {
        return head;
    }
};


/// Thread Safe Queue
class ThreadSafeQueue {
    queue<pair<int, int>> q;
    sem_t sem;
    mutex mtx;

public:
    ThreadSafeQueue() {
        sem_init(&sem, 0, 0);
    }

    ~ThreadSafeQueue() {
        sem_destroy(&sem);
    }

    void push(pair<int, int> value) {
        mtx.lock();
        q.push(value);
        mtx.unlock();

        sem_post(&sem);
    }

     pair<int,int> pop() {
        sem_wait(&sem);
        mtx.lock();
        pair<int, int> result = q.front();
        q.pop();
        mtx.unlock();

        return result;
    }

    bool is_empty() {
        unique_lock<mutex> lock(mtx);
        return q.empty();
    }
};


/// Leaderboard
class Leaderboard {
    LinkedList list;
    map<int, bool> blacklist;
    mutex mtx;

public:
    Leaderboard() = default;
    ~Leaderboard() = default;

    void update(pair<int, int> value) {
        unique_lock<mutex> lock(mtx);
        if (blacklist[value.first]) return;

        if(value.second == -1) {
            blacklist[value.first] = true;
            list.delete_node(value.first);

            return;
        }

        if(list.find(value.first)) list.update_node(value);
        else list.add(value);
    }

    bool is_equal_to_list(LinkedList l) {
        Node* current_list = list.get_head();
        Node* other_list = l.get_head();

        while(current_list != nullptr && other_list != nullptr) {
            if(current_list->data.second != other_list-> data.second) return false;
            current_list = current_list->next;
            other_list = other_list->next;
        }

        return current_list == other_list;
    }

    void save_to_file() {
        list.display();
    }
};

/// Thread functions
ThreadSafeQueue tqueue;
Leaderboard leaderboard;
atomic<int> readers_done = 0;

void reader(vector<string> files) {
    for(string file : files) {
        ifstream in(file);

        int id, score;

        while(in >> id >> score) {
            tqueue.push(make_pair(id, score));
        }
        in.close();
    }

    ++readers_done;
}

void worker(int no_readers) {
    while(true) {
        if(tqueue.is_empty() && readers_done == no_readers) break;

        if(!tqueue.is_empty())
            leaderboard.update(tqueue.pop());
    }
}

void parallel_run(int no_readers, int no_workers) {
    thread readers[no_readers];
    thread workers[no_workers];

    // Reader
    int start = 0;
    int step = files.size() / no_readers;
    int rest = files.size() % no_readers;
    for(int i = 0; i < no_readers; i++) {
        int extra_step = rest-- > 0 ? 1 : 0;
        vector current_files(files.begin() + start, files.begin() + start + step + extra_step);
        start += step;
        readers[i] = thread(reader, current_files);
    }

    // Worker
    for(int i = 0; i < no_workers; i++) {
        workers[i] = thread(worker, no_readers);
    }

    //Join threads
    for(int i = 0; i < no_readers; i++)
        readers[i].join();

    for(int i = 0; i < no_workers; i++)
        workers[i].join();

    leaderboard.save_to_file();
}

/// Sequential function
LinkedList seq_list;
void sequential_run() {
    for(string file : files) {
        ifstream in(file);
        int id, score;

        while(in>> id>> score) {
            if(seq_list.find(id)) seq_list.update_node(make_pair(id, score));
            else seq_list.add(make_pair(id, score));
        }

        in.close();
    }

    seq_list.display();
}


int main(int argc, char** argv) {
    // generate_input_data();
    load_file_names();

    auto start_time = chrono::high_resolution_clock::now();
    int no_readers = atoi(argv[1]);
    int no_workers = atoi(argv[2]);
    if(no_readers > 0) {
        parallel_run(no_readers, no_workers);
    }
    else {
        sequential_run();
    }
    auto stop_time = chrono::high_resolution_clock::now();
    std::chrono::duration<double> duration = stop_time - start_time;
    double final_time = duration.count();

    // cout<< leaderboard.is_equal_to_list(seq_list);
    cout << std::fixed << std::showpoint << std::setprecision(10) << final_time * 1E3;
    return 0;
}