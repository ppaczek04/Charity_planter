package com.example.demo.repository;

import com.example.demo.model.CoinEvent;
import org.springframework.data.jpa.repository.JpaRepository;

import java.time.Instant;
import java.util.List;

public interface CoinEventRepository extends JpaRepository<CoinEvent, Long> {
    CoinEvent findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(String deviceMac, String ownerId);
    List<CoinEvent> findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
            String deviceMac, String ownerId, Instant start, Instant end);
    List<CoinEvent> findByOwnerIdOrderByTimestampDesc(String ownerId);
}