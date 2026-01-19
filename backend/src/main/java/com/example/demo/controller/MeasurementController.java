package com.example.demo.controller;

import com.example.demo.config.MqttGateway;
import com.example.demo.dto.WateringRequest;
import com.example.demo.model.*;
import com.example.demo.repository.*;
import com.example.demo.service.MeasurementService;
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
    private final MeasurementService measurementService;
    private final DeviceRepository deviceRepository;

    public MeasurementController(TemperatureRepository tempRepo,
                                 PressureRepository pressureRepo,
                                 SoilMeasurementRepository soilRepo,
                                 CoinEventRepository coinRepo,
                                 MqttGateway mqttGateway,
                                 MeasurementService measurementService,
                                 DeviceRepository deviceRepository) {
        this.tempRepo = tempRepo;
        this.pressureRepo = pressureRepo;
        this.soilRepo = soilRepo;
        this.coinRepo = coinRepo;
        this.mqttGateway = mqttGateway;
        this.measurementService = measurementService;
        this.deviceRepository = deviceRepository;
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

    @PostMapping("/devices/{deviceId}/water")
    public ResponseEntity<?> triggerWatering(@PathVariable Long deviceId, @RequestBody WateringRequest request) {
        return deviceRepository.findById(deviceId)
                .map(device -> {
                    double duration = (request.getDuration() != null) ? request.getDuration() : 1.0;
                    if (duration > 5.0) duration = 5.0;
                    if (duration < 0.5) duration = 0.5;

                    boolean success = measurementService.triggerWateringAndWait(device.getOwnerId(), device.getMac(), duration);

                    if (success) {
                        return ResponseEntity.ok("Podlano pomyślnie (potwierdzone przez urządzenie).");
                    } else {
                        return ResponseEntity.status(504).body("Urządzenie nie odpowiada (Offline). Sprawdź zasilanie.");
                    }
                })
                .orElse(ResponseEntity.notFound().build());
    }
}
