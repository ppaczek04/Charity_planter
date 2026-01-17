package com.example.demo.model;

import jakarta.persistence.*;
import lombok.AllArgsConstructor;
import lombok.Data;
import lombok.NoArgsConstructor;

@Entity
@Table(name = "devices")
@Data
@NoArgsConstructor
@AllArgsConstructor
public class Device {

    @Id
    @GeneratedValue(strategy = GenerationType.IDENTITY)
    private Long id;

    @Column(name = "mac_address", nullable = false, unique = true)
    private String mac;

    @Column(name = "owner_id", nullable = false)
    private String ownerId;

    @Column(name = "device_name")
    private String name;

    private boolean holidayMode;
    private Integer measurementInterval = 5;
    private Integer soilMin = 30;
    private Integer soilMax = 70;
}