package com.example.demo;

import com.fasterxml.jackson.databind.JsonNode;
import com.fasterxml.jackson.databind.ObjectMapper;
import org.eclipse.paho.client.mqttv3.MqttConnectOptions;
import org.springframework.beans.factory.annotation.Value;
import org.springframework.context.annotation.Bean;
import org.springframework.context.annotation.Configuration;
import org.springframework.integration.annotation.ServiceActivator;
import org.springframework.integration.channel.DirectChannel;
import org.springframework.integration.core.MessageProducer;
import org.springframework.integration.mqtt.core.DefaultMqttPahoClientFactory;
import org.springframework.integration.mqtt.core.MqttPahoClientFactory;
import org.springframework.integration.mqtt.inbound.MqttPahoMessageDrivenChannelAdapter;
import org.springframework.integration.mqtt.support.DefaultPahoMessageConverter;
import org.springframework.messaging.MessageChannel;
import org.springframework.messaging.MessageHandler;

import java.util.concurrent.atomic.AtomicReference;

@Configuration
public class MqttConfig {

    @Value("${mqtt.broker}")
    private String brokerUrl;

    @Value("${mqtt.clientId}")
    private String clientId;

    @Value("${mqtt.topic}")
    private String topic;

    // Tutaj przechowujemy ostatnią wiadomość (na start pusta)
    private final AtomicReference<String> lastMessage = new AtomicReference<>("");

    @Bean
    public AtomicReference<String> lastMessageRef() {
        return lastMessage;
    }

    @Bean
    public MqttPahoClientFactory mqttClientFactory() {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();

        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{ brokerUrl }); // np. "tcp://mosquitto:1883" albo "tcp://172.19.107.49:1883"
        // Jeśli masz użytkownika/hasło:
        // options.setUserName("user");
        // options.setPassword("pass".toCharArray());

        factory.setConnectionOptions(options);
        return factory;
    }

    @Bean
    public MessageChannel mqttInputChannel() {
        return new DirectChannel();
    }


    @Bean
    @ServiceActivator(inputChannel = "mqttInputChannel")
    public MessageHandler handler(AtomicReference<String> lastRef,
                                  SoilMeasurementRepository repository,
                                  ObjectMapper objectMapper) {

        return message -> {
            String payload = String.valueOf(message.getPayload());
            lastRef.set(payload);
            System.out.println("[MQTT] ✅ Otrzymano wiadomość: " + payload);

            try {
                JsonNode node = objectMapper.readTree(payload);

                // wyciągamy wilgotność procentową
                int soilPercent = node.path("soil_percent").asInt();

                SoilMeasurement measurement = new SoilMeasurement();
                measurement.setMoisture(soilPercent);
                // timestamp doda się sam w @PrePersist

                repository.save(measurement);

                System.out.println("[DB] 💾 Zapisano pomiar: " + soilPercent + " %");

            } catch (Exception e) {
                System.err.println("[MQTT] ❌ Błąd przy parsowaniu / zapisie do DB: " + e.getMessage());
            }
        };

    }
    @Bean
    public MessageProducer inbound(MqttPahoClientFactory clientFactory,
                                   @Value("${mqtt.topic}") String topic) {

        MqttPahoMessageDrivenChannelAdapter adapter =
                new MqttPahoMessageDrivenChannelAdapter(
                        "backend-subscriber",
                        clientFactory,
                        topic
                );

        adapter.setQos(1);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setOutputChannel(mqttInputChannel()); // <-- WAŻNE
        return adapter;
    };



}


