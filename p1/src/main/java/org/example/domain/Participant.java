package org.example.domain;

public class Participant implements Comparable<Participant>{
    private int country_id;
    private int id;
    private int score;

    public Participant(int country_id, int id, int score) {
        this.country_id = country_id;
        this.id = id;
        this.score = score;
    }

    public int getCountry_id() {
        return country_id;
    }

    public void setCountry_id(int country_id) {
        this.country_id = country_id;
    }

    public int getId() {
        return id;
    }

    public void setId(int id) {
        this.id = id;
    }

    public int getScore() {
        return score;
    }

    public void setScore(int score) {
        this.score = score;
    }

    @Override
    public String toString() {
        return "{" + "country_id=" + country_id + ", id=" + id + ", score=" + score + '}';
    }

    @Override
    public int compareTo(Participant other) {
        if (this.score == other.score) {
            return -1 * Integer.compare(this.id, other.id);
        }
        return Integer.compare(this.score, other.score);
    }
}
