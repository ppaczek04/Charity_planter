package com.example.demo.dto;
import lombok.Data;

@Data
public class DeviceDto {
    public String mac;
    public String name;
    public boolean isArchived;
    public String ownerId;
}
