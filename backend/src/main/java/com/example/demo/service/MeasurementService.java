package com.example.demo.service;

import com.example.demo.config.MqttGateway;
import com.example.demo.model.*;
import com.example.demo.repository.*;
import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.springframework.stereotype.Service;

//@Service
//public class MeasurementService {
//
//    private final SoilMeasurementRepository repository;
//    private final ObjectMapper objectMapper;
//
//    public MeasurementService(SoilMeasurementRepository repository, ObjectMapper objectMapper) {
//        this.repository = repository;
//        this.objectMapper = objectMapper;
//    }
//
//    public void processAndSave(String payload) {
//        try {
//            JsonNode node = objectMapper.readTree(payload);
//
//            int soilPercent = node.path("soil_percent").asInt();
//            // Handle nulls safely
//            Double temperature = node.has("temperature_c") ? node.get("temperature_c").asDouble() : null;
//            Double pressure = node.has("pressure_hpa") ? node.get("pressure_hpa").asDouble() : null;
//
//            SoilMeasurement measurement = new SoilMeasurement();
//            measurement.setMoisture(soilPercent);
//            measurement.setTemperature(temperature);
//            measurement.setPressure(pressure);
//
//            repository.save(measurement);
//
//            System.out.println("[DB] 💾 Zapisano: Wilg=" + soilPercent + "%, Temp=" + temperature);
//        } catch (Exception e) {
//            System.err.println("[Service] ❌ Błąd przetwarzania JSON: " + e.getMessage());
//        }
//    }
//}

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

            if (!node.has("value")) {
                System.out.println("⚠️ Ignoruję wiadomość bez pola 'value' (Topic: " + topic + ")");
                return;
            }

            double value = node.get("value").asDouble();

            switch (sensorType) {

                case "temperature": {
                    Temperature t = new Temperature();
                    t.setValue(value);
                    t.setDeviceMac(deviceMac);
                    temperatureRepository.save(t);
                    System.out.println("🌡️ Zapisano Temp: " + value);
                    break;
                }

                case "pressure": {
                    Pressure p = new Pressure();
                    p.setValue(value);
                    p.setDeviceMac(deviceMac);
                    pressureRepository.save(p);
                    System.out.println("🧭 Zapisano Ciśnienie: " + value);
                    break;
                }

                case "soil": {
                    SoilMeasurement soil = new SoilMeasurement();
                    soil.setMoisture((int) value);
                    soilMeasurementRepository.save(soil);
                    System.out.println("🌱 Zapisano Soil: " + value);
                    break;
                }

                case "coin_inserted": {
                    CoinEvent c = new CoinEvent();
                    c.setValue(value);
                    c.setDeviceMac(deviceMac);
                    coinEventRepository.save(c);
                    System.out.println("🪙 Zapisano Monetę! Uruchamiam podlewanie...");

                    // wysyłamy komendę podlewania
                    String commandTopic = String.format("%s/%s/water", userMac, deviceMac);
                    String commandPayload = "{\"command\": \"WATER_ON\", \"duration_sec\": 5}";
                    mqttGateway.sendToMqtt(commandPayload, commandTopic);

                    System.out.println("📤 Wysłano rozkaz na temat: " + commandTopic);
                    break;
                }

                default:
                    System.out.println("⚠️ Nieznany typ sensora: " + sensorType);
            }

        } catch (Exception e) {
            System.err.println("❌ Błąd przetwarzania MQTT: " + e.getMessage());
        }
    }
}