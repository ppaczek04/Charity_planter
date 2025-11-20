package com.example.demo;

import org.springframework.data.jpa.repository.JpaRepository;
import java.time.Instant;

public interface SoilMeasurementRepository extends JpaRepository<SoilMeasurement, Instant> {
    // Na razie puste — zapis działa automatycznie przez JPA
}
