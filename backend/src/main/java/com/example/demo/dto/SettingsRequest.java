package com.example.demo.dto;

import lombok.Data;

@Data
public class SettingsRequest {
    private Integer interval;
    private Boolean holidayMode;
    private Integer soilMin;
    private Integer soilMax;
}
