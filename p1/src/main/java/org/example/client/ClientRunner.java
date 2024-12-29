package org.example.client;

import java.io.IOException;
import java.net.Socket;
import java.util.Scanner;

public class ClientRunner {
    public static int port = 8080;

    public static void main(String[] args) {
//        Scanner scanner = new Scanner(System.in);
//        int country_id = scanner.nextInt();
        int country_id = -1;
        int deltax = 1;
        try {
            country_id = Integer.parseInt(args[0]);
            deltax = Integer.parseInt(args[1]);
        } catch (Exception e) {
            System.out.println("Invalid country ID");
            return;
        }

        System.out.println("Creating socket to server");
        try (Socket socket = new Socket("localhost", port)) {
            Client client = new Client(country_id, deltax, socket);

            client.sendData();
            System.out.println("Sent data");

            client.requestLeaderboard();
            System.out.println("Sent leaderboard request");
            client.receiveLeaderboard();

            client.receiveFile();
            client.receiveFile();
        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }
}