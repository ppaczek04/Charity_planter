package com.example.demo.dto;

import lombok.Data;

@Data
public class ClaimDeviceRequest {
    private String deviceMac;
    private String userId;
}