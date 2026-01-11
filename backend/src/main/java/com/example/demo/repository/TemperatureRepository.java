package com.example.demo.repository;

import com.example.demo.model.Temperature;
import org.springframework.data.jpa.repository.JpaRepository;

import java.time.Instant;
import java.util.List;

public interface TemperatureRepository extends JpaRepository<Temperature, Long> {
    Temperature findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(String deviceMac, String ownerId);
    List<Temperature> findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
            String deviceMac, String ownerId, Instant start, Instant end);
    List<Temperature> findByOwnerIdOrderByTimestampDesc(String ownerId);
}
