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
}
