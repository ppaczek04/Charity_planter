package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "soil_measurements")
@Data
@NoArgsConstructor
@AllArgsConstructor
public class SoilMeasurement {
    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Column(name = "reading_value", nullable = false)
    private Integer value;
    private Instant timestamp;
    private String deviceMac;
    @Column(name = "owner_id")
    private String ownerId;

    @PrePersist
    public void prePersist() {
        if (timestamp == null) {
            timestamp = Instant.now();
        }
    }
}
