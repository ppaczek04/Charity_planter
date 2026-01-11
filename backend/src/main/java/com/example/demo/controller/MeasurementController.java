package com.example.demo.controller;

import com.example.demo.config.MqttGateway;
import com.example.demo.model.*;
import com.example.demo.repository.*;
import org.springframework.format.annotation.DateTimeFormat;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.time.Instant;
import java.time.temporal.ChronoUnit;
import java.util.List;
import java.util.Map;
import java.util.HashMap;

@RestController
@RequestMapping("/api")
@CrossOrigin(origins = "*")
public class MeasurementController {

    private final TemperatureRepository tempRepo;
    private final PressureRepository pressureRepo;
    private final SoilMeasurementRepository soilRepo;
    private final CoinEventRepository coinRepo;
    private final MqttGateway mqttGateway;

    public MeasurementController(TemperatureRepository tempRepo,
                                 PressureRepository pressureRepo,
                                 SoilMeasurementRepository soilRepo,
                                 CoinEventRepository coinRepo,
                                 MqttGateway mqttGateway) {
        this.tempRepo = tempRepo;
        this.pressureRepo = pressureRepo;
        this.soilRepo = soilRepo;
        this.coinRepo = coinRepo;
        this.mqttGateway = mqttGateway;
    }

    // GET /api/temperatures?deviceMac=AA:BB:CC&from=2023-01-01T00:00:00Z&to=2023-01-02T00:00:00Z
    @GetMapping("/temperatures")
    public List<Temperature> getTemperatures(
            @RequestParam String deviceMac,
            @RequestParam String ownerId,
            @RequestParam(required = false) Instant from,
            @RequestParam(required = false) Instant to) {

        if (from == null) from = Instant.now().minus(1, ChronoUnit.DAYS);
        if (to == null) to = Instant.now();

        return tempRepo.findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
                deviceMac, ownerId, from, to);
    }

    // GET /api/pressures ...
    @GetMapping("/pressures")
    public List<Pressure> getPressures(
            @RequestParam String deviceMac,
            @RequestParam String ownerId,
            @RequestParam(required = false) Instant from,
            @RequestParam(required = false) Instant to) {

        if (from == null) from = Instant.now().minus(1, ChronoUnit.DAYS);
        if (to == null) to = Instant.now();

        return pressureRepo.findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
                deviceMac, ownerId, from, to);
    }

    // GET /api/soil-measurements
    @GetMapping("/soil-measurements")
    public List<SoilMeasurement> getSoils(
            @RequestParam String deviceMac,
            @RequestParam String ownerId,
            @RequestParam(required = false) Instant from,
            @RequestParam(required = false) Instant to) {

        if (from == null) from = Instant.now().minus(1, ChronoUnit.DAYS);
        if (to == null) to = Instant.now();

        return soilRepo.findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
                deviceMac, ownerId, from, to);
    }

    @GetMapping("/coin-events")
    public List<CoinEvent> getCoinEvents(
            @RequestParam String deviceMac,
            @RequestParam String ownerId,
            @RequestParam(required = false) Instant from,
            @RequestParam(required = false) Instant to) {

        if (from == null) from = Instant.now().minus(1, ChronoUnit.DAYS);
        if (to == null) to = Instant.now();

        return coinRepo.findByDeviceMacAndOwnerIdAndTimestampBetweenOrderByTimestampDesc(
                deviceMac, ownerId, from, to);
    }

    // GET /api/devices/{deviceMac}/users/{ownerId}/latest
    @GetMapping("/devices/{deviceMac}/users/{ownerId}/latest")
    public ResponseEntity<Map<String, Object>> getLatestStatus(
            @PathVariable String deviceMac,
            @PathVariable String ownerId) {

        Temperature lastTemp = tempRepo.findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(deviceMac, ownerId);
        Pressure lastPressure = pressureRepo.findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(deviceMac, ownerId);
        SoilMeasurement lastSoil = soilRepo.findTopByDeviceMacAndOwnerIdOrderByTimestampDesc(deviceMac, ownerId);

        Map<String, Object> response = new HashMap<>();
        response.put("deviceMac", deviceMac);
        response.put("ownerId", ownerId);

        response.put("temperature", lastTemp != null ? lastTemp : "Brak danych");
        response.put("pressure", lastPressure != null ? lastPressure : "Brak danych");
        response.put("soil", lastSoil != null ? lastSoil : "Brak danych");

        return ResponseEntity.ok(response);
    }

    @PostMapping("/devices/{deviceMac}/water")
    public ResponseEntity<String> triggerWatering(
            @PathVariable String deviceMac,
            @RequestParam String userMac,
            @RequestParam(defaultValue = "1.0") double duration) {

        String commandTopic = String.format("%s/%s/water", userMac, deviceMac);
        String payload = String.format("{\"command\": \"WATER_ON\", \"duration_sec\": %.1f}", duration);

        mqttGateway.sendToMqtt(payload, commandTopic);

        return ResponseEntity.ok("Wysłano komendę podlewania na temat: " + commandTopic);
    }
}
