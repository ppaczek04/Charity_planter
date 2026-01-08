package com.example.demo.service;

import com.example.demo.config.MqttGateway;
import com.example.demo.model.*;
import com.example.demo.repository.*;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Service;
import java.time.Instant;

@Service
public class MeasurementService {

    private final TemperatureRepository temperatureRepository;
    private final PressureRepository pressureRepository;
    private final SoilMeasurementRepository soilMeasurementRepository;
    private final CoinEventRepository coinEventRepository;
    private final ObjectMapper objectMapper;
    private final MqttGateway mqttGateway;

    public MeasurementService(
            TemperatureRepository temperatureRepository,
            PressureRepository pressureRepository,
            SoilMeasurementRepository soilMeasurementRepository,
            CoinEventRepository coinEventRepository,
            ObjectMapper objectMapper,
            MqttGateway mqttGateway
    ) {
        this.temperatureRepository = temperatureRepository;
        this.pressureRepository = pressureRepository;
        this.soilMeasurementRepository = soilMeasurementRepository;
        this.coinEventRepository = coinEventRepository;
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

            String userMac = parts[0];
            String deviceMac = parts[1];
            String sensorType = parts[2];

            // komendy sterujące pomijamy
            if ("water".equals(sensorType)) {
                return;
            }

            JsonNode node = objectMapper.readTree(payload);

            // wymagamy value + timestamp
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
                    t.setOwnerId(userMac);
                    t.setTimestamp(instant);

                    temperatureRepository.save(t);
                    System.out.println("Zapisano Temp: " + value + " @ " + instant);
                    break;
                }

                case "pressure": {
                    Pressure p = new Pressure();
                    p.setValue(value);
                    p.setDeviceMac(deviceMac);
                    p.setOwnerId(userMac);
                    p.setTimestamp(instant);

                    pressureRepository.save(p);
                    System.out.println("Zapisano Ciśnienie: " + value + " @ " + instant);
                    break;
                }

                case "soil": {
                    SoilMeasurement soil = new SoilMeasurement();
                    soil.setValue((int) value);
                    soil.setDeviceMac(deviceMac);
                    soil.setOwnerId(userMac);
                    soil.setTimestamp(instant);

                    soilMeasurementRepository.save(soil);
                    System.out.println("Zapisano Soil: " + value + " @ " + instant);
                    break;
                }

                case "coin_inserted": {
                    CoinEvent c = new CoinEvent();
                    c.setValue(value);
                    c.setDeviceMac(deviceMac);
                    c.setOwnerId(userMac);
                    c.setTimestamp(instant);

                    coinEventRepository.save(c);
                    System.out.println("Zapisano Monetę @ " + instant);

                    // wysyłamy komendę podlewania
                    String commandTopic = String.format("%s/%s/water", userMac, deviceMac);
                    String commandPayload = "{\"command\": \"WATER_ON\", \"duration_sec\": 1.0}";
                    mqttGateway.sendToMqtt(commandPayload, commandTopic);

                    System.out.println("Wysłano rozkaz podlewania na: " + commandTopic);
                    break;
                }

                default:
                    System.out.println("Nieznany typ sensora: " + sensorType);
            }

        } catch (Exception e) {
            System.err.println("Błąd przetwarzania MQTT: " + e.getMessage());
        }
    }
}