package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "coin_events")
@Data @NoArgsConstructor @AllArgsConstructor
public class CoinEvent {
    @Id @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Column(name = "reading_value")
    private Double value; // Np. nominał monety (1.0)
    private Instant timestamp;
    private String deviceMac;

    @PrePersist void prePersist() { if (timestamp == null) timestamp = Instant.now(); }
}