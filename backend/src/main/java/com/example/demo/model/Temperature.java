package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "temperatures")
@Data @NoArgsConstructor @AllArgsConstructor
public class Temperature {
    @Id @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;
    @Column(name = "reading_value")
    private Double value;
    private Instant timestamp;
    private String deviceMac;

    @PrePersist void prePersist() { if (timestamp == null) timestamp = Instant.now(); }
}