package com.example.demo.repository;

import com.example.demo.model.SoilMeasurement;
import org.springframework.data.jpa.repository.JpaRepository;
import java.time.Instant;

public interface SoilMeasurementRepository extends JpaRepository<SoilMeasurement, Long> {
}
