package com.example.demo.service;

import com.example.demo.config.MqttGateway;
import com.example.demo.model.*;
import com.example.demo.repository.*;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Service;
import java.time.Instant;
import java.util.Optional;

@Service
public class MeasurementService {

    private final TemperatureRepository temperatureRepository;
    private final PressureRepository pressureRepository;
    private final SoilMeasurementRepository soilMeasurementRepository;
    private final CoinEventRepository coinEventRepository;
    private final DeviceRepository deviceRepository;
    private final ObjectMapper objectMapper;
    private final MqttGateway mqttGateway;

    public MeasurementService(
            TemperatureRepository temperatureRepository,
            PressureRepository pressureRepository,
            SoilMeasurementRepository soilMeasurementRepository,
            CoinEventRepository coinEventRepository,
            DeviceRepository deviceRepository,
            ObjectMapper objectMapper,
            MqttGateway mqttGateway
    ) {
        this.temperatureRepository = temperatureRepository;
        this.pressureRepository = pressureRepository;
        this.soilMeasurementRepository = soilMeasurementRepository;
        this.coinEventRepository = coinEventRepository;
        this.deviceRepository = deviceRepository;
        this.objectMapper = objectMapper;
        this.mqttGateway = mqttGateway;
    }

    public void processMessage(String topic, String payload) {
        try {
            // topic: user_mac/device_mac/sensor
            String[] parts = topic.split("/");
            if (parts.length < 3) {
                System.out.println("Nieprawidłowy topic: " + topic);
                return;
            }

            String userId = parts[0];
            String deviceMac = parts[1];
            String sensorType = parts[2];

            if ("water".equals(sensorType)) {
                return;
            }

            JsonNode node = objectMapper.readTree(payload);

            if (!node.has("value") || !node.has("timestamp")) {
                System.out.println("Ignoruję wiadomość bez value lub timestamp (Topic: " + topic + ")");
                return;
            }

            double value = node.get("value").asDouble();
            long timestamp = node.get("timestamp").asLong();
            Instant instant = Instant.ofEpochSecond(timestamp);

            switch (sensorType) {

                case "temperature": {
                    Temperature t = new Temperature();
                    t.setValue(value);
                    t.setDeviceMac(deviceMac);
                    t.setOwnerId(userId);
                    t.setTimestamp(instant);
                    temperatureRepository.save(t);
                    System.out.println("Zapisano Temp: " + value + " @ " + instant);
                    break;
                }

                case "pressure": {
                    Pressure p = new Pressure();
                    p.setValue(value);
                    p.setDeviceMac(deviceMac);
                    p.setOwnerId(userId);
                    p.setTimestamp(instant);
                    pressureRepository.save(p);
                    System.out.println("Zapisano Ciśnienie: " + value + " @ " + instant);
                    break;
                }

                case "soil": {
                    SoilMeasurement soil = new SoilMeasurement();
                    soil.setValue((int) value);
                    soil.setDeviceMac(deviceMac);
                    soil.setOwnerId(userId);
                    soil.setTimestamp(instant);
                    soilMeasurementRepository.save(soil);
                    System.out.println("Zapisano Soil: " + value + " @ " + instant);
                    break;
                }

                case "coin_inserted": {
                    CoinEvent c = new CoinEvent();
                    c.setValue(value);
                    c.setDeviceMac(deviceMac);
                    c.setOwnerId(userId);
                    c.setTimestamp(instant);
                    coinEventRepository.save(c);
                    System.out.println("Zapisano Monetę @ " + instant);

                    Optional<Device> deviceOpt = deviceRepository.findByMac(deviceMac);

                    if (deviceOpt.isPresent()) {
                        Device device = deviceOpt.get();
                        int maxAllowedMoisture = device.getSoilMax(); // np. 70%

                        SoilMeasurement lastSoil = soilMeasurementRepository.findTopByDeviceMacOrderByTimestampDesc(deviceMac);

                        if (lastSoil == null || lastSoil.getValue() < maxAllowedMoisture) {
                            sendWaterCommand(userId, deviceMac, 1.0);
                        } else {
                            System.out.println("BLOKADA PODLEWANIA: Wilgotność " + lastSoil.getValue() +
                                    "% jest wyższa niż limit " + maxAllowedMoisture + "%. Nie przelewamy rośliny.");
                        }
                    } else {
                        System.out.println("Nie znaleziono urządzenia w bazie, wykonuję standardowe podlewanie.");
                        sendWaterCommand(userId, deviceMac, 1.0);
                    }
                    break;
                }

                default:
                    System.out.println("Nieznany typ sensora: " + sensorType);
            }

        } catch (Exception e) {
            System.err.println("Błąd przetwarzania MQTT: " + e.getMessage());
        }
    }

    public void sendWaterCommand(String userId, String deviceMac, double duration) {
        String commandTopic = String.format("%s/%s/water", userId, deviceMac);
        String commandPayload = String.format("{\"command\": \"WATER_ON\", \"duration_sec\": %.1f}", duration);

        mqttGateway.sendToMqtt(commandPayload, commandTopic);
        System.out.println("Wysłano rozkaz podlewania na: " + commandTopic);
    }
}