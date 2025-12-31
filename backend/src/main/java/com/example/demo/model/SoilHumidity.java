package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "soil_humidities")
@Data @NoArgsConstructor @AllArgsConstructor
public class SoilHumidity {
    @Id @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Column(name = "reading_value")
    private Integer value; // Procenty
    private Instant timestamp;
    private String deviceMac;

    @PrePersist void prePersist() { if (timestamp == null) timestamp = Instant.now(); }
}
