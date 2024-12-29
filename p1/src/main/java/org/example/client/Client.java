package org.example.client;

import org.example.domain.Participant;

import java.io.*;
import java.net.Socket;
import java.util.ArrayList;
import java.util.List;
import java.util.Random;

public class Client {
    private final int no_problems = 10;
    private final int country_id;
    private final int deltax;
    private final List<Participant> results;
    private final Socket socket;

    public Client(int country_id, int deltax, Socket socket) {
        this.country_id = country_id;
        this.deltax = deltax;
        this.socket = socket;
        results = new ArrayList<>();
        //generateFiles();
        readFromFiles();
    }

    private void generateFiles() {
        Random random = new Random();
        int no_participants = random.nextInt(70, 100);

        for (int i = 1; i <= no_problems; i++) {
            String filename = "inputs/ProblemC" + country_id + "_P" + i;
            try (FileWriter writer = new FileWriter(filename)) {
                for (int j = 1; j <= no_participants; j++) {
                    int score = random.nextInt(100);
                    writer.write(j + " " + score + "\n");
                }
            } catch (IOException err) {
                System.out.println("Error in creating file " + filename);
            }
        }
    }

    public int countryPoints() {
        int sum = 0;
        for (Participant p : results) sum += p.getScore();
        return sum;
    }


    private void readFromFiles() {
        for (int i = 1; i <= no_problems; i++) {
            String filename = "inputs/ProblemC" + country_id + "_P" + i;
            try (BufferedReader reader = new BufferedReader(new FileReader(filename))) {
                String line;

                while ((line = reader.readLine()) != null) {
                    try {
                        String[] parts = line.split(" ");
                        int id = Integer.parseInt(parts[0].trim());
                        int score = Integer.parseInt(parts[1].trim());

                        Participant p = new Participant(country_id, id, score);
                        results.add(p);
                    } catch (NumberFormatException err) {
                        System.err.println("Skipping invalid line: " + line);
                    }
                }
            } catch (IOException err) {
                System.err.println("Error reading file: " + filename);
            }
        }
    }

    public void sendData() throws IOException {
        OutputStream out = socket.getOutputStream();
        PrintWriter writer = new PrintWriter(out, true);

        System.out.println("Sending data...");
        for (int i = 0; i < results.size(); i++) {
            if ((i - 1) % 20 == 0) {
                try {
                    Thread.sleep(deltax * 1000L);
                } catch (InterruptedException e) {
                    System.out.println("Error in thread.sleep()");
                }
            }

            Participant p = results.get(i);
            writer.println(p.getCountry_id() + " " + p.getId() + " " + p.getScore());
            writer.flush();
        }
    }

    public void requestLeaderboard() throws IOException {
        OutputStream out = socket.getOutputStream();
        PrintWriter writer = new PrintWriter(out, true);
        writer.println("leaderboard");
        writer.flush();
    }

    public void receiveLeaderboard() {
        System.out.println("Current scores are: ");
        try {
            InputStream inputStream = socket.getInputStream();
            BufferedReader reader = new BufferedReader(new InputStreamReader(inputStream));

            String line;
            line = reader.readLine();

            int count = Integer.parseInt(line);
            for (int i = 0; i < count; i++) {
                line = reader.readLine();
                int score = Integer.parseInt(line);
                System.out.println(score);
            }

        } catch (IOException e) {
            System.out.println(e.getMessage());
        }
    }


    public void receiveFile() throws IOException {
        InputStream inputStream = socket.getInputStream();
        DataInputStream in = new DataInputStream(inputStream);

        String filename = in.readUTF();
        long bytesExpected = in.readLong();

        String folderName = "client" + country_id;
        String fullPath = folderName + File.separator + filename;
        File folder = new File(folderName);
        if (!folder.exists()) {
            folder.mkdirs();
        }

        OutputStream outputStream = new FileOutputStream(fullPath, false);
        byte[] buffer = new byte[1024];
        int bytesRead;

        while (bytesExpected > 0
                && (bytesRead = in.read(buffer, 0,
                (int) Math.min(buffer.length, bytesExpected))) != -1) {
            outputStream.write(buffer, 0, bytesRead);
            bytesExpected -= bytesRead;
        }
        outputStream.close();
        System.out.println("Received final results");
    }
}
