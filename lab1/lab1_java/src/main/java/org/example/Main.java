package org.example;

import java.io.File;
import java.io.FileNotFoundException;
import java.util.Random;
import java.util.Scanner;

public class Main {
    static int NUMBER_LIMIT = 10;
    static int rows, columns;
    static int convolusion_rows, convolusion_columns;
    static int no_threads;
    static long time;

    public static void read_file(String[] args) throws FileNotFoundException {
        Scanner scanner = new Scanner(new File("D:\\Facultate\\ppd\\lab1_java\\src\\main\\java\\org\\example\\input.txt"));
        rows = scanner.nextInt();
        columns = scanner.nextInt();
        convolusion_rows = scanner.nextInt();
        convolusion_columns = scanner.nextInt();
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

    public static boolean are_equal(int[][] matrix1, int[][] matrix2, int rows, int columns) {
        for (int i = 0; i < rows; i++)
            for (int j = 0; j < columns; j++)
                if (matrix1[i][j] != matrix2[i][j])
                    return false;

        return true;
    }

    public static int[][] sequential_run(int[][] matrix, int[][] convolusion) {
        int[][] result = new int[rows][columns];
        long start = System.nanoTime();

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

        long end = System.nanoTime();
        time = end - start;

        return result;
    }

    public static int[][] row_threads_run(int[][] matrix, int[][] convolusion) {
        int[][] result = new int[rows][columns];
        Thread[] line_threads = new LineThread[no_threads];
        long start_time = System.nanoTime();

        int start = 0;
        int end = rows / no_threads;
        int rest = rows % no_threads;
        int step = rows / no_threads;

        for (int i = 0; i < no_threads; i++) {
            if (rest > 0) {
                end++;
                rest--;
            }

            line_threads[i] = new LineThread(matrix, convolusion, result, rows, columns, convolusion_rows, convolusion_columns, start, end);
            line_threads[i].start();

            start = end;
            end = start + step;
        }

        for (int i = 0; i < no_threads; i++) {
            try{
                line_threads[i].join();
            }
            catch (InterruptedException e){
                throw new RuntimeException(e);
            }
        }

        long end_time = System.nanoTime();
        time = end_time - start_time;
        return result;
    }

    public static int[][] column_threads_run(int[][] matrix, int[][] convolusion){
        int[][] result = new int[rows][columns];
        Thread[] column_threads = new ColumnThread[no_threads];
        long start_time = System.nanoTime();

        int start = 0;
        int end = columns / no_threads;
        int rest = columns % no_threads;
        int step = columns / no_threads;

        for (int i = 0; i < no_threads; i++) {
            if (rest > 0) {
                end++;
                rest--;
            }

            column_threads[i] = new ColumnThread(matrix, convolusion, result, rows, columns, convolusion_rows, convolusion_columns, start, end);
            column_threads[i].start();

            start = end;
            end = start + step;
        }

        for (int i = 0; i < no_threads; i++) {
            try{
                column_threads[i].join();
            }
            catch (InterruptedException e){
                throw new RuntimeException(e);
            }
        }

        long end_time = System.nanoTime();
        time = end_time - start_time;
        return result;
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

        int[][] seq_result = sequential_run(matrix, convolusion);
        if(no_threads == 0){
            System.out.println((double)time/1E6);
            return;
        }

        try{
            //int[][] thread_result = row_threads_run(matrix, convolusion);
            int[][] thread_result = column_threads_run(matrix, convolusion);

            if(!are_equal(thread_result, seq_result, rows, columns)){
                System.out.println("Error - Matrixes are not equal");
                return;
            }
        }
        catch (RuntimeException e){
            System.out.println(e.getMessage());
        }

        System.out.println((double)time/1E6);
    }
}

class LineThread extends Thread {
    private final int[][] matrix;
    private final int[][] convolusion;
    private int[][] result;
    private final int rows, columns, convolusion_rows, convolusion_columns;
    private final int start_row, end_row;

    public LineThread(int[][] matrix, int[][] convolusion, int[][] result, int rows, int columns, int convolusion_rows, int convolusion_columns, int start_row, int end_row) {
        this.matrix = matrix;
        this.convolusion = convolusion;
        this.result = result;
        this.rows = rows;
        this.columns = columns;
        this.convolusion_rows = convolusion_rows;
        this.convolusion_columns = convolusion_columns;
        this.start_row = start_row;
        this.end_row = end_row;
    }

    @Override
    public void run() {
        for (int i = start_row; i < end_row; i++) {
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

                        this.result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2][convolusion_j - j + convolusion_columns / 2];
                    }
                }
            }
        }
    }
}

class ColumnThread extends Thread {
    private final int[][] matrix;
    private final int[][] convolusion;
    private int[][] result;
    private final int rows, columns, convolusion_rows, convolusion_columns;
    private final int start_column, end_column;

    public ColumnThread(int[][] matrix, int[][] convolusion, int[][] result, int rows, int columns, int convolusion_rows, int convolusion_columns, int start_column, int end_column) {
        this.matrix = matrix;
        this.convolusion = convolusion;
        this.result = result;
        this.rows = rows;
        this.columns = columns;
        this.convolusion_rows = convolusion_rows;
        this.convolusion_columns = convolusion_columns;
        this.start_column = start_column;
        this.end_column = end_column;
    }

    @Override
    public void run() {
        for (int i = 0; i < rows; i++) {
            for (int j = start_column; j < end_column; j++) {
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

                        this.result[i][j] += matrix[final_i][final_j] * convolusion[convolusion_i - i + convolusion_rows / 2][convolusion_j - j + convolusion_columns / 2];
                    }
                }
            }
        }
    }
}