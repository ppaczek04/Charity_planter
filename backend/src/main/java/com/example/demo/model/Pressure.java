package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "pressures")
@Data @NoArgsConstructor @AllArgsConstructor
public class Pressure {
    @Id @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Column(name = "reading_value")
    private Double value; // hPa
    private Instant timestamp;
    private String deviceMac;
    @Column(name = "owner_id")
    private String ownerId;

    @PrePersist void prePersist() { if (timestamp == null) timestamp = Instant.now(); }
}
