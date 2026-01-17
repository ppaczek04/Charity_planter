package com.example.demo.model;

import jakarta.persistence.*;
import lombok.*;
import java.time.Instant;

@Entity
@Table(name = "temperatures")
@Data
@NoArgsConstructor
@AllArgsConstructor
public class Temperature {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "reading_value", nullable = false)
    private Double value;

    @Column(nullable = false)
    private Instant timestamp;

    @Column(nullable = false)
    private String deviceMac;

    @Column(name = "owner_id", nullable = false)
    private String ownerId;
}