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

    private final TemperatureRepository tempRepo;
    private final PressureRepository pressRepo;
    private final SoilHumidityRepository soilRepo;
    private final CoinEventRepository coinRepo;
    private final ObjectMapper objectMapper;
    private final MqttGateway mqttGateway;

    public MeasurementService(TemperatureRepository tempRepo, PressureRepository pressRepo,
                              SoilHumidityRepository soilRepo, CoinEventRepository coinRepo,
                              ObjectMapper objectMapper, MqttGateway mqttGateway) {
        this.tempRepo = tempRepo;
        this.pressRepo = pressRepo;
        this.soilRepo = soilRepo;
        this.coinRepo = coinRepo;
        this.objectMapper = objectMapper;
        this.mqttGateway = mqttGateway;
    }

    public void processMessage(String topic, String payload) {
        try {
            // Topic format: user_mac/device_mac/sensor_type
            String[] parts = topic.split("/");
            if (parts.length < 3) return;

            String userMac = parts[0];
            String deviceMac = parts[1];
            String sensorType = parts[2]; // temperature, pressure, soil_humidity, coin_inserted

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
                case "temperature":
                    Temperature t = new Temperature();
                    t.setValue(value);
                    t.setDeviceMac(deviceMac);
                    tempRepo.save(t);
                    System.out.println("🌡️ Zapisano Temp: " + value);
                    break;

                case "pressure":
                    Pressure p = new Pressure();
                    p.setValue(value);
                    p.setDeviceMac(deviceMac);
                    pressRepo.save(p);
                    System.out.println("⏲️ Zapisano Ciśnienie: " + value);
                    break;

                case "soil_humidity":
                    SoilHumidity s = new SoilHumidity();
                    s.setValue((int) value);
                    s.setDeviceMac(deviceMac);
                    soilRepo.save(s);
                    System.out.println("💧 Zapisano Wilgotność: " + value);
                    break;

                case "coin_inserted":
                    CoinEvent c = new CoinEvent();
                    c.setValue(value);
                    c.setDeviceMac(deviceMac);
                    coinRepo.save(c);
                    System.out.println(" Zapisano Monetę! Uruchamiam podlewanie...");

                    // Budujemy temat odpowiedzi: user123/esp32_01/water
                    String commandTopic = String.format("%s/%s/water", userMac, deviceMac);

                    // Budujemy prosty JSON z rozkazem
                    String commandPayload = "{\"command\": \"WATER_ON\", \"duration_sec\": 5}";

                    // Wysyłamy przez bramkę
                    mqttGateway.sendToMqtt(commandPayload, commandTopic);

                    System.out.println("📤 Wysłano rozkaz na temat: " + commandTopic);
                    break;
                default:
                    System.out.println("⚠️ Nieznany typ sensora: " + sensorType);
            }
        } catch (Exception e) {
            System.err.println("❌ Błąd przetwarzania: " + e.getMessage());
        }
    }
}