package com.example.demo.controller;

import com.example.demo.config.MqttGateway;
import com.example.demo.dto.ClaimDeviceRequest;
import com.example.demo.dto.DeviceDto;
import com.example.demo.dto.SettingsRequest;
import com.example.demo.dto.UpdateDeviceRequest;
import com.example.demo.model.Device;
import com.example.demo.repository.DeviceRepository;
import com.example.demo.repository.TemperatureRepository;
import com.example.demo.repository.UserRepository;
import com.example.demo.service.MeasurementService;
import org.springframework.http.ResponseEntity;
import org.springframework.web.bind.annotation.*;

import java.util.ArrayList;
import java.util.Set;
import java.util.stream.Collectors;

import java.util.List;
import java.util.Optional;

@RestController
@RequestMapping("/api/devices")
@CrossOrigin
public class DeviceController {

    private final DeviceRepository deviceRepository;
    private final TemperatureRepository tempRepository;
    private final MeasurementService measurementService;

    public DeviceController(DeviceRepository deviceRepository, TemperatureRepository tempRepository, MeasurementService measurementService) {
        this.deviceRepository = deviceRepository;
        this.tempRepository = tempRepository;
        this.measurementService = measurementService;
    }

    // POST /api/devices/claim
    @PostMapping("/claim")
    public ResponseEntity<?> claimDevice(@RequestBody ClaimDeviceRequest request) {

        Optional<Device> existingDevice = deviceRepository.findByMac(request.getDeviceMac());

        if (existingDevice.isPresent()) {
            Device device = existingDevice.get();
            device.setOwnerId(request.getUserId());
            deviceRepository.save(device);
            return ResponseEntity.ok("Urządzenie przejęte pomyślnie.");
        } else {
            Device newDevice = new Device();
            newDevice.setMac(request.getDeviceMac());
            newDevice.setOwnerId(request.getUserId());
            newDevice.setName("Moja nowa doniczka");
            deviceRepository.save(newDevice);
            return ResponseEntity.ok("Urządzenie dodane pomyślnie.");
        }
    }

    // PUT /api/devices/{deviceId}
    @PutMapping("/{deviceId}")
    public ResponseEntity<?> renameDevice(@PathVariable Long deviceId, @RequestBody UpdateDeviceRequest request) {
        return deviceRepository.findById(deviceId)
                .map(device -> {
                    device.setName(request.getNewName());
                    deviceRepository.save(device);
                    return ResponseEntity.ok("Nazwa zmieniona pomyślnie.");
                })
                .orElse(ResponseEntity.notFound().build());
    }

    // GET /api/devices/user/{userId}
    @GetMapping("/user/{userId}")
    public ResponseEntity<List<Device>> getUserDevices(@PathVariable String userId) {
        List<Device> devices = deviceRepository.findAllByOwnerId(userId);
        return ResponseEntity.ok(devices);
    }

    @GetMapping("/user/{userId}/archived")
    public ResponseEntity<List<DeviceDto>> getArchivedDevices(@PathVariable String userId) {

        List<Device> activeDevices = deviceRepository.findAllByOwnerId(userId);
        Set<String> activeMacs = activeDevices.stream()
                .map(Device::getMac)
                .collect(Collectors.toSet());

        List<String> historicalMacs = tempRepository.findMacsByOwnerId(userId);

        List<DeviceDto> archivedDevices = new ArrayList<>();

        for (String mac : historicalMacs) {
            if (!activeMacs.contains(mac)) {
                DeviceDto dto = new DeviceDto();
                dto.setMac(mac);
                dto.setName("Doniczka " + mac);
                dto.setOwnerId(userId);
                dto.setArchived(true);
                archivedDevices.add(dto);
            }
        }

        return ResponseEntity.ok(archivedDevices);
    }

    // PUT /api/devices/{deviceId}/settings
    @PutMapping("/{deviceId}/settings")
    public ResponseEntity<?> updateSettings(@PathVariable Long deviceId, @RequestBody SettingsRequest request) {
        return deviceRepository.findById(deviceId)
                .map(device -> {

                    if (request.getInterval() != null) {
                        if (request.getInterval() < 5) return ResponseEntity.badRequest().body("Min interval 5s");

                        boolean success = measurementService.updateConfigAndWait(
                                device.getOwnerId(),
                                device.getMac(),
                                request.getInterval()
                        );

                        if (!success) {
                            return ResponseEntity.status(504).body("Urządzenie nie potwierdziło zmiany interwału (Offline?).");
                        }

                        device.setMeasurementInterval(request.getInterval());
                    }

                    if (request.getHolidayMode() != null) device.setHolidayMode(request.getHolidayMode());
                    if (request.getSoilMin() != null) device.setSoilMin(request.getSoilMin());
                    if (request.getSoilMax() != null) device.setSoilMax(request.getSoilMax());

                    deviceRepository.save(device);

                    return ResponseEntity.ok(device);
                })
                .orElse(ResponseEntity.notFound().build());
    }
}