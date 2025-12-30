package com.example.demo.repository;

import com.example.demo.model.CoinEvent;
import org.springframework.data.jpa.repository.JpaRepository;

public interface CoinEventRepository extends JpaRepository<CoinEvent, Long> {}