package com.example.demo.repository;

import com.example.demo.model.SoilMeasurement;
import org.springframework.data.jpa.repository.JpaRepository;
import java.time.Instant;
import java.util.List;

public interface SoilMeasurementRepository extends JpaRepository<SoilMeasurement, Long> {
    SoilMeasurement findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(String deviceMac, String ownerId);
    List<SoilMeasurement> findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
            String deviceMac, String ownerId, Instant start, Instant end);
    List<SoilMeasurement> findByOwnerIdOrderByTimestampDesc(String ownerId);
}
