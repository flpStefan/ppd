package org.example;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Random;
import java.util.Scanner;
import java.util.concurrent.CyclicBarrier;

public class Main {
    static int NUMBER_LIMIT = 10;
    static int rows, columns;
    final static int convolusion_rows = 3, convolusion_columns = 3;
    static int no_threads;
    static long time;

    public static void read_file(String[] args) throws FileNotFoundException {
        Scanner scanner = new Scanner(new File("D:\\Facultate\\ppd\\lab2\\lab2\\src\\main\\java\\org\\example\\input.txt"));
        rows = scanner.nextInt();
        columns = scanner.nextInt();
        no_threads = Integer.parseInt(args[0]);
    }

    public static void print_matrix(int rows, int columns, int[][] matrix) {
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < columns; j++) {
                System.out.print(matrix[i][j]);
                System.out.print(" ");
            }
            System.out.println();
        }
    }

    public static int[][] generate_matrix(int rows, int columns, int limit) {
        int[][] matrix = new int[rows][columns];

        Random random = new Random();
        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < columns; j++) {
                matrix[i][j] = random.nextInt(limit);
            }
        }

        return matrix;
    }

    public static int[][] verification_matrix(int[][] matrix, int[][] convolusion) {
        int[][] result = new int[rows][columns];

        for (int i = 0; i < rows; i++) {
            for (int j = 0; j < columns; j++) {
                for (int convolusion_i = i - convolusion_rows / 2; convolusion_i <= i + convolusion_rows / 2; convolusion_i++) {
                    int final_i;
                    if (convolusion_i < 0) final_i = 0;
                    else if (convolusion_i >= rows) final_i = rows - 1;
                    else final_i = convolusion_i;

                    for (int convolusion_j = j - convolusion_columns / 2; convolusion_j <= j + convolusion_columns / 2; convolusion_j++) {
                        int final_j;
                        if (convolusion_j < 0) final_j = 0;
                        else if (convolusion_j >= columns) final_j = columns - 1;
                        else final_j = convolusion_j;

                        result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2][convolusion_j - j + convolusion_columns / 2];
                    }
                }
            }
        }

        return result;
    }

    public static boolean are_equal(int[][] matrix1, int[][] matrix2, int rows, int columns) {
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < columns; j++)
                if (matrix1[i][j] != matrix2[i][j])
                    return false;

        return true;
    }

    public static void run_sequential(int[][] matrix, int[][] convolusion){
        int[][] aux = new int[3][columns];

        for(int i = 0; i < columns; i++){
            aux[0][i] = matrix[0][i];
            aux[2][i] = matrix[rows - 1][i];
        }

        boolean isNextRow = false;
        for(int i = 0; i < rows - 1; i++){
            if(!isNextRow){
                for(int j = 0; j < columns; j++){
                    aux[1][j] = matrix[i][j];
                    if(j + 1 < columns) aux[1][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for(int k = 0; k < 3; k++){
                        int final_index = j + k - 1;
                        if(final_index < 0) final_index = 0;
                        if(final_index >= columns) final_index = columns - 1;

                        sum += convolusion[0][k] * aux[0][final_index];
                        sum += convolusion[1][k] * aux[1][final_index];
                        sum += convolusion[2][k] * matrix[i + 1][final_index];
                    }
                    matrix[i][j] = sum;
                }

            }
            else{
                for(int j = 0; j < columns; j++){
                    aux[0][j] = matrix[i][j];
                    if(j + 1 < columns) aux[0][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for(int k = 0; k < 3; k++){
                        int final_index = j + k - 1;
                        if(final_index < 0) final_index = 0;
                        if(final_index >= columns) final_index = columns - 1;

                        sum += convolusion[0][k] * aux[1][final_index];
                        sum += convolusion[1][k] * aux[0][final_index];
                        sum += convolusion[2][k] * matrix[i + 1][final_index];
                    }
                    matrix[i][j] = sum;
                }
            }

            isNextRow = !isNextRow;
        }

        for(int j = 0; j < columns; j++){
            int sum = 0;
            for(int k = 0; k < 3; k++){
                int final_index = j + k - 1;
                if(final_index < 0) final_index = 0;
                if(final_index >= columns) final_index = columns - 1;

                sum += convolusion[0][k] * aux[isNextRow ? 1 : 0][final_index];
                sum += convolusion[1][k] * aux[2][final_index];
                sum += convolusion[2][k] * aux[2][final_index];
            }

            matrix[rows - 1][j] = sum;
        }
    }

    public static void run_threads(int[][] matrix, int[][] convulsion){
        MyBarrier barrier = new MyBarrier(no_threads);
        Thread[] threads = new Thread[no_threads];


        int start = 0;
        int end = rows / no_threads;
        int rest = rows % no_threads;
        int step = rows / no_threads;

        for(int i = 0; i < no_threads; i++){
            if (rest > 0) {
                end++;
                rest--;
            }

            threads[i] = new MyThread(matrix, convulsion, rows, columns, start, end, barrier);
            threads[i].start();

            start = end;
            end = start + step;
        }

        for(int i = 0; i < no_threads; i++){
            try{
                threads[i].join();
            }
            catch (Exception e){
                e.printStackTrace();
            }
        }
    }


    public static void main(String[] args) {
        try {
            read_file(args);
        } catch (FileNotFoundException e) {
            System.out.println(e.getMessage());
            return;
        }

        int[][] matrix = generate_matrix(rows, columns, NUMBER_LIMIT);
        int[][] convolusion = generate_matrix(convolusion_rows, convolusion_columns, NUMBER_LIMIT);
        int[][] verification = verification_matrix(matrix, convolusion);

        if(no_threads == 0){
            long start_time = System.nanoTime();
            run_sequential(matrix, convolusion);
            long stop_time = System.nanoTime();
            time = stop_time - start_time;

            if(!are_equal(matrix, verification, rows, columns)){
                System.out.println("Error - Matrixes not equal");
                return;
            }
        }
        else{
            try{
                long start_time = System.nanoTime();
                run_threads(matrix, convolusion);
                long stop_time = System.nanoTime();
                time = stop_time - start_time;

                if(!are_equal(matrix, verification, rows, columns)){
                    System.out.println("Error - Matrixes not equal");
                    return;
                }
            }
            catch (RuntimeException e){
                System.out.println(e.getMessage());
            }
        }

        System.out.println((double)time/1E6);
    }
}

class MyThread extends Thread {
    private final int[][] matrix;
    private final int[][] convolusion;
    private final MyBarrier barrier;
    private final int rows, columns;
    private final int start, end;

    public MyThread(int[][] matrix, int[][] convolusion, int rows, int columns, int start, int end, MyBarrier barrier) {
        this.matrix = matrix;
        this.convolusion = convolusion;
        this.rows = rows;
        this.columns = columns;
        this.start = start;
        this.end = end;
        this.barrier = barrier;
    }

    @Override
    public void run() {
        int[][] aux = new int[3][columns];

        int row_above = start > 0 ? (start - 1) : start;
        int row_below = Math.min(end, (rows - 1));
        for(int i = 0; i < columns; i++){
            aux[0][i] = matrix[row_above][i];
            aux[2][i] = matrix[row_below][i];
        }
        barrier.waitBarrier();

        boolean isNextRow = false;
        for(int i = start; i < end - 1; i++){
            if(!isNextRow){
                for(int j = 0; j < columns; j++){
                    aux[1][j] = matrix[i][j];
                    if(j + 1 < columns) aux[1][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for(int k = 0; k < 3; k++){
                        int final_index = j + k - 1;
                        if(final_index < 0) final_index = 0;
                        if(final_index >= columns) final_index = columns - 1;

                        sum += convolusion[0][k] * aux[0][final_index];
                        sum += convolusion[1][k] * aux[1][final_index];
                        sum += convolusion[2][k] * matrix[i + 1][final_index];
                    }
                    matrix[i][j] = sum;
                }

            }
            else{
                for(int j = 0; j < columns; j++){
                    aux[0][j] = matrix[i][j];
                    if(j + 1 < columns) aux[0][j + 1] = matrix[i][j + 1];

                    int sum = 0;
                    for(int k = 0; k < 3; k++){
                        int final_index = j + k - 1;
                        if(final_index < 0) final_index = 0;
                        if(final_index >= columns) final_index = columns - 1;

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
        for(int j = 0;j < columns; j++){
            aux[second_index][j] = matrix[end - 1][j];
        }

        for(int j = 0; j < columns; j++){
            int sum = 0;
            for(int k = 0; k < 3; k++){
                int final_index = j + k - 1;
                if(final_index < 0) final_index = 0;
                if(final_index >= columns) final_index = columns - 1;

                sum += convolusion[0][k] * aux[first_index][final_index];
                sum += convolusion[1][k] * aux[second_index][final_index];
                sum += convolusion[2][k] * aux[2][final_index];
            }

            matrix[end - 1][j] = sum;
        }
    }
}

class MyBarrier {
    private final CyclicBarrier barrier;


    public MyBarrier(int count){
        barrier = new CyclicBarrier(count);
    }

    public void waitBarrier(){
        try{
            barrier.await();
        }
        catch(Exception e){
            e.printStackTrace();
        }
    }
}