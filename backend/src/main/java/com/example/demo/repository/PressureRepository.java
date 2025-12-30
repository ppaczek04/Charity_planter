package com.example.demo.repository;

import com.example.demo.model.Pressure;
import org.springframework.data.jpa.repository.JpaRepository;

public interface PressureRepository extends JpaRepository<Pressure, Long> {}
