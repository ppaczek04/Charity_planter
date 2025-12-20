package com.example.demo;

import jakarta.persistence.*;
import java.time.Instant;

@Entity
@Table(name = "soil_measurements")
public class SoilMeasurement {

    @Id
    @Column(name = "timestamp", nullable = false)
    private Instant timestamp;

    @Column(name = "moisture", nullable = false)
    private Integer moisture;

    // >>> ZMIANA: nowe kolumny (mogą być null, jeśli payload nie zawiera tych pól)
    @Column(name = "temperature", nullable = true)
    private Double temperature;

    @Column(name = "pressure", nullable = true)
    private Double pressure;

    @PrePersist
    public void prePersist() {
        if (timestamp == null) {
            timestamp = Instant.now();
        }
    }

    // --- gettery/settery ---

    public Instant getTimestamp() {
        return timestamp;
    }

    public void setTimestamp(Instant timestamp) {
        this.timestamp = timestamp;
    }

    public Integer getMoisture() {
        return moisture;
    }

    public void setMoisture(Integer moisture) {
        this.moisture = moisture;
    }

    // >>> ZMIANA: gettery/settery temperatury i ciśnienia
    public Double getTemperature() {
        return temperature;
    }

    public void setTemperature(Double temperature) {
        this.temperature = temperature;
    }

    public Double getPressure() {
        return pressure;
    }

    public void setPressure(Double pressure) {
        this.pressure = pressure;
    }
}
