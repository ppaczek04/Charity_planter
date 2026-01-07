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
    private Double value;
    private Instant timestamp;
    private String deviceMac;
    @Column(name = "owner_id")
    private String ownerId;

    @PrePersist void prePersist() { if (timestamp == null) timestamp = Instant.now(); }
}