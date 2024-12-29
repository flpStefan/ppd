package org.example.server;


import org.example.domain.Participant;

import java.io.*;
import java.net.Socket;
import java.util.ArrayList;
import java.util.Collections;
import java.util.List;
import java.util.concurrent.*;
import java.util.concurrent.atomic.AtomicBoolean;
import java.util.concurrent.atomic.AtomicInteger;
import java.util.concurrent.locks.ReentrantLock;

public class Server {
    private final int no_readers;
    private final int no_workers;
    private final int deltat;
    private final int no_clients;
    private final List<Socket> clientList;
    //--------------------------------------\\
    private final ReentrantLock[] locks;
    private final ReentrantLock scoreLock = new ReentrantLock();
    private final BlockingQueue<Participant> queue = new LinkedBlockingQueue<>();
    private final AtomicBoolean[] requests;
    private final AtomicBoolean[] sent;
    private final AtomicInteger doneReading = new AtomicInteger(0);
    private long lastUpdate = 0;
    private ArrayList<Participant> leaderboard = new ArrayList<>();
    private int[] country_scores;

    public Server(int no_readers, int no_workers, int deltat, int no_clients, List<Socket> clientList) {
        this.no_readers = no_readers;
        this.no_workers = no_workers;
        this.deltat = deltat;
        this.no_clients = no_clients;
        this.clientList = clientList;
        this.country_scores = new int[no_clients];

        requests = new AtomicBoolean[no_clients];
        for (int i = 0; i < no_clients; i++)
            requests[i] = new AtomicBoolean(false);

        sent = new AtomicBoolean[no_clients];
        for (int i = 0; i < no_clients; i++)
            sent[i] = new AtomicBoolean(false);

        locks = new ReentrantLock[no_clients];

        for (int i = 0; i < no_clients; i++)
            locks[i] = new ReentrantLock();
    }

    public void start() {
        Thread[] reader_threads = new Thread[no_readers];
        Thread[] worker_threads = new Thread[no_workers];

        for (int i = 0; i < no_readers; i++) {
            reader_threads[i] = new Thread(this::readerFunction);
            reader_threads[i].start();
        }

        for (int i = 0; i < no_workers; i++) {
            worker_threads[i] = new Thread(this::workerFunction);
            worker_threads[i].start();
        }

        listenForRequests();
        try {
            for (int i = 0; i < no_readers; i++)
                reader_threads[i].join();

            for (int i = 0; i < no_workers; i++)
                worker_threads[i].join();
        } catch (InterruptedException e) {
            System.out.println(e.getMessage());
        }
        System.out.println("Finished leaderboard");

        System.out.println("Creating final files");
        createLeaderboardFile();
        createResultsFile();

        System.out.println("Sending files to clients");
        for(int i = 0; i < no_clients; i++){
            Socket socket = clientList.get(i);
            sendFile("country_results.txt", socket);
            sendFile("results.txt", socket);
        }
    }

    private void readerFunction() {
        try {
            boolean done = false;
            while (!done) {
                done = true;
                for (int index = 0; index < no_clients; index++) {
                    if (locks[index].isLocked()) {
                        continue;
                    }
                    synchronized (requests) {
                        if (requests[index].get()) {
                            continue;
                        }
                    }

                    locks[index].lock();
                    try {
                        done = false;

                        InputStream in = clientList.get(index).getInputStream();
                        BufferedReader reader = new BufferedReader(new InputStreamReader(in));

                        String line;
                        while ((line = reader.readLine()) != null) {
                            if ("leaderboard".equals(line)) {
                                System.out.println("Received leaderboard request from " + index);
                                synchronized (requests) {
                                    requests[index].set(true);
                                }
                                break;
                            }

                            String[] parts = line.split(" ");
                            int country_id = Integer.parseInt(parts[0]);
                            int id = Integer.parseInt(parts[1]);
                            int score = Integer.parseInt(parts[2]);

                            try {
                                queue.put(new Participant(country_id, id, score));
                            } catch (InterruptedException e) {
                                System.out.println(e.getMessage());
                            }
                        }
                    } finally {
                        locks[index].unlock();
                    }
                }
            }

            doneReading.getAndIncrement();
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }

    private void workerFunction() {
        try {
            while (doneReading.get() < no_clients || !queue.isEmpty()) {
                Participant p;
                try {
                    p = queue.poll(500, TimeUnit.MILLISECONDS);
                } catch (InterruptedException e) {
                    System.out.println("Worker interrupted while taking from queue");
                    continue;
                }

                if (p == null) continue;
                scoreLock.lock();
                try {
                    boolean found = false;
                    for (Participant participant : leaderboard) {
                        if (participant.getCountry_id() == p.getCountry_id() && participant.getId() == p.getId()) {
                            found = true;
                            if (p.getScore() == -1 || participant.getScore() == -1) {
                                participant.setScore(-1);
                            } else {
                                participant.setScore(p.getScore() + participant.getScore());
                            }
                            break;
                        }
                    }
                    if (!found) {
                        leaderboard.add(p);
                    }
                } finally {
                    scoreLock.unlock();
                }
            }
        } catch (Exception e) {
            System.out.println("Error in workerFunction: " + e.getMessage());
        }
    }

    void listenForRequests() {
        boolean done = false;

        while (!done) {
            done = true;

            for (int country = 0; country < no_clients; country++) {
                if (!requests[country].get()) {
                    done = false;
                } else {
                    if (!sent[country].get()) {
                        if (System.currentTimeMillis() - lastUpdate > deltat) {
                            CompletableFuture<Void> calculate = CompletableFuture.runAsync(this::calculateLeaderboard);

                            final int countryId = country;
                            calculate.thenRun(() -> {
                                sendLeaderboard(clientList.get(countryId));
                                sent[countryId].set(true);
                            });

                            try {
                                calculate.get();
                            } catch (InterruptedException | ExecutionException e) {
                                System.out.println(e.getMessage());
                            }

                        } else {
                            sendLeaderboard(clientList.get(country));
                            sent[country].set(true);
                        }
                    }
                }
            }

        }
    }

    void calculateLeaderboard() {
        scoreLock.lock();
        try {
            for (int i = 0; i < no_clients; i++) {
                country_scores[i] = 0;
            }

            for (Participant p : leaderboard) {
                if (p.getScore() == -1) {
                    continue;
                }
                country_scores[p.getCountry_id() - 1] += p.getScore();
            }

            lastUpdate = System.currentTimeMillis();
        } finally {
            scoreLock.unlock();
        }
    }

    void sendLeaderboard(Socket client) {
        try {
            OutputStream out = client.getOutputStream();
            PrintWriter writer = new PrintWriter(out, true);

            writer.println(no_clients);
            for (int score : country_scores) {
                writer.println(score);
            }
            writer.flush();
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }

    private void createLeaderboardFile() {
        calculateLeaderboard();

        String filename = "country_results.txt";
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename))) {
            for (int country = 0; country < no_clients; country++) {
                writer.write("Country " + country + " had the score: " + country_scores[country]);
                writer.newLine();
            }
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }

    private void createResultsFile(){
        leaderboard.sort(Collections.reverseOrder());

        String filename = "results.txt";
        try (BufferedWriter writer = new BufferedWriter(new FileWriter(filename))) {
            for (Participant p : leaderboard) {
                writer.write("Participant: " + p.getCountry_id() + " - " + p.getId() + " with score: " + p.getScore());
                writer.newLine();
            }
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }

    private static void sendFile(String fileName, Socket clientSocket) {
        try {
            DataOutputStream outputStream = new DataOutputStream(clientSocket.getOutputStream());
            outputStream.writeUTF(fileName);
            long fileLength = new File(fileName).length();
            outputStream.writeLong(fileLength);

            FileInputStream fileInputStream = new FileInputStream(fileName);
            BufferedInputStream bufferedInputStream = new BufferedInputStream(fileInputStream);

            byte[] buffer = new byte[1024];
            int bytesRead;
            while ((bytesRead = bufferedInputStream.read(buffer)) != -1) {
                outputStream.write(buffer, 0, bytesRead);
            }
            outputStream.flush();
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }
}

