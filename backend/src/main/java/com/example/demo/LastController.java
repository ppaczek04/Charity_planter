package com.example.demo;

import org.springframework.web.bind.annotation.GetMapping;
import org.springframework.web.bind.annotation.RestController;
import java.util.Map;
import java.util.concurrent.atomic.AtomicReference;

@RestController
public class LastController {

    private final AtomicReference<String> last;

    public LastController(AtomicReference<String> last) {
        this.last = last;
    }

    @GetMapping("/last")
    public Map<String, String> getLastMessage() {
        return Map.of("last", last.get());
    }
}
