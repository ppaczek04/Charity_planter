package com.example.demo.service;

import com.example.demo.model.SoilMeasurement;
import com.example.demo.repository.SoilMeasurementRepository;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Service;

@Service
public class MeasurementService {

    private final SoilMeasurementRepository repository;
    private final ObjectMapper objectMapper;

    public MeasurementService(SoilMeasurementRepository repository, ObjectMapper objectMapper) {
        this.repository = repository;
        this.objectMapper = objectMapper;
    }

    public void processAndSave(String payload) {
        try {
            JsonNode node = objectMapper.readTree(payload);

            int soilPercent = node.path("soil_percent").asInt();
            // Handle nulls safely
            Double temperature = node.has("temperature_c") ? node.get("temperature_c").asDouble() : null;
            Double pressure = node.has("pressure_hpa") ? node.get("pressure_hpa").asDouble() : null;

            SoilMeasurement measurement = new SoilMeasurement();
            measurement.setMoisture(soilPercent);
            measurement.setTemperature(temperature);
            measurement.setPressure(pressure);

            repository.save(measurement);

            System.out.println("[DB] 💾 Zapisano: Wilg=" + soilPercent + "%, Temp=" + temperature);
        } catch (Exception e) {
            System.err.println("[Service] ❌ Błąd przetwarzania JSON: " + e.getMessage());
        }
    }
}