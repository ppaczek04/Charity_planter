package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "pressures")
@Data
@NoArgsConstructor
@AllArgsConstructor
public class Pressure {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "reading_value", nullable = false)
    private Double value; // hPa

    @Column(nullable = false)
    private Instant timestamp;

    @Column(nullable = false)
    private String deviceMac;

    @Column(name = "owner_id", nullable = false)
    private String ownerId;
}

