package com.example.demo.config;

import com.example.demo.service.MeasurementService;
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
import org.springframework.integration.mqtt.support.MqttHeaders;

@Configuration
public class MqttConfig {

    @Value("${mqtt.broker}")
    private String brokerUrl;

    @Value("${mqtt.topic}")
    private String topic;

    @Bean
    public MqttPahoClientFactory mqttClientFactory() {
        DefaultMqttPahoClientFactory factory = new DefaultMqttPahoClientFactory();
        MqttConnectOptions options = new MqttConnectOptions();
        options.setServerURIs(new String[]{ brokerUrl });
        options.setAutomaticReconnect(true); // Warto dodać
        factory.setConnectionOptions(options);
        return factory;
    }

    @Bean
    public MessageChannel mqttInputChannel() {
        return new DirectChannel();
    }

//    @Bean
//    public MessageProducer inbound(MqttPahoClientFactory clientFactory) {
//        MqttPahoMessageDrivenChannelAdapter adapter =
//                new MqttPahoMessageDrivenChannelAdapter("backend-subscriber", clientFactory, topic);
//        adapter.setQos(1);
//        adapter.setConverter(new DefaultPahoMessageConverter());
//        adapter.setOutputChannel(mqttInputChannel());
//        return adapter;
//    }
//
//    // Tutaj wstrzykujemy nasz nowy MeasurementService!
//    @Bean
//    @ServiceActivator(inputChannel = "mqttInputChannel")
//    public MessageHandler handler(MeasurementService measurementService) {
//        return message -> {
//            String payload = String.valueOf(message.getPayload());
//            System.out.println("[MQTT] 📩 Wiadomość: " + payload);
//            // Zlecamy pracę serwisowi
//            measurementService.processAndSave(payload);
//        };
//    }
    @Bean
    public MessageProducer inbound(MqttPahoClientFactory clientFactory) {
        // Nasłuchujemy na WSZYSTKO co ma 3 poziomy (user/device/sensor)
        MqttPahoMessageDrivenChannelAdapter adapter =
                new MqttPahoMessageDrivenChannelAdapter("backend-sub", clientFactory, "+/+/+");

        adapter.setQos(1);
        adapter.setConverter(new DefaultPahoMessageConverter());
        adapter.setOutputChannel(mqttInputChannel());
        return adapter;
    }

    @Bean
    @ServiceActivator(inputChannel = "mqttInputChannel")
    public MessageHandler handler(MeasurementService service) {
        return message -> {
            // Wyciągamy temat z nagłówka wiadomości
            String topic = (String) message.getHeaders().get(MqttHeaders.RECEIVED_TOPIC);
            String payload = String.valueOf(message.getPayload());

            // Przekazujemy do serwisu
            service.processMessage(topic, payload);
        };
    }
}


