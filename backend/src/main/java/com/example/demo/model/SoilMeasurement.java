package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "soil_measurements")
@Data // Lombok tworzy gettery, settery, toString automatycznie
@NoArgsConstructor
@AllArgsConstructor
public class SoilMeasurement {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id; // Bezpieczniejsze ID

    @Column(nullable = false)
    private Instant timestamp;

    @Column(nullable = false)
    private Integer moisture;

    private Double temperature;
    private Double pressure;

    @PrePersist
    public void prePersist() {
        if (timestamp == null) {
            timestamp = Instant.now();
        }
    }
}
