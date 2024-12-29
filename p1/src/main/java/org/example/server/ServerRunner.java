package org.example.server;

import java.io.IOException;
import java.net.ServerSocket;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;

public class ServerRunner {
    public static int port = 8080;
    public static final int no_clients = 2;

    public static void main(String[] args) {
        int no_readers;
        int no_workers;
        int deltat;

        try {
            no_readers = Integer.parseInt(args[0]);
            no_workers = Integer.parseInt(args[1]);
            deltat = Integer.parseInt(args[2]);
        } catch (Exception e) {
            System.out.println("Wrong input");
            return;
        }

        List<Socket> clientList = new ArrayList<>();
        System.out.println("Starting server");
        try (ServerSocket serverSocket = new ServerSocket(port)) {
            for (int i = 1; i <= no_clients; i++) {
                Socket client = serverSocket.accept();
                clientList.add(client);
            }

            System.out.println("Connected to all clients..");
            Server server = new Server(no_readers, no_workers, deltat, no_clients, clientList);
            server.start();

            System.out.println("Shutting down server..");
            for(int i = 0; i < no_clients; i++){
                clientList.get(i).close();
            }
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }
}
