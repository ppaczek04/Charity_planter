package com.example.demo.repository;

import com.example.demo.model.Pressure;
import org.springframework.data.jpa.repository.JpaRepository;

import java.time.Instant;
import java.util.List;

public interface PressureRepository extends JpaRepository<Pressure, Long> {
    Pressure findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(String deviceMac, String ownerId);
    List<Pressure> findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
            String deviceMac, String ownerId, Instant start, Instant end);
    List<Pressure> findByOwnerIdOrderByTimestampDesc(String ownerId);
}
