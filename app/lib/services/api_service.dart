import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';

/// ApiService - klasa do komunikacji z backendem
/// Tutaj zbieramy logikę HTTP requests w jedno miejsce
/// Dzięki temu kod ekranów pozostaje czysty i czytelny
class ApiService {
  // URL backendu
  //
  // Przy pracy z hotspotem IP laptopa potrafi się zmieniać,
  // więc baseUrl da się teraz ustawić w aplikacji i jest trzymany w SharedPreferences.
  // Domyślnie zostawiamy wartość jak wcześniej, ale w razie potrzeby możesz ją zmienić.
  static const String _baseUrlKey = 'api_base_url';
  // Domyślnie ustawiamy localhost, bo przy pracy z fizycznym telefonem najpewniejsze
  // jest "adb reverse" (USB) i wtedy 127.0.0.1:8080 kieruje na port 8080 laptopa.
  // Jeśli uruchomisz aplikację BEZ kabla USB/adb reverse, to trzeba zmienić na IP laptopa.
  static const String _defaultBaseUrl = 'http://127.0.0.1:8080/api';

  static Future<String> getBaseUrl() async {
    final prefs = await SharedPreferences.getInstance();
    final stored = prefs.getString(_baseUrlKey);
    return (stored == null || stored.trim().isEmpty) ? _defaultBaseUrl : stored.trim();
  }

  static Future<void> setBaseUrl(String url) async {
    var normalized = url.trim();
    if (normalized.isEmpty) return;

    while (normalized.endsWith('/')) {
      normalized = normalized.substring(0, normalized.length - 1);
    }
    if (!normalized.endsWith('/api')) {
      normalized = '$normalized/api';
    }

    final prefs = await SharedPreferences.getInstance();
    await prefs.setString(_baseUrlKey, normalized);
  }

  static Future<Uri> _uri(String path) async {
    final base = await getBaseUrl();
    return Uri.parse('$base$path');
  }

  static const Duration _requestTimeout = Duration(seconds: 8);

  // Klucze w pamięci telefonu
  // - user: minimalny obiekt użytkownika (id/email/username)
  // - auth_token: legacy (zostawiamy, bo część kodu historycznie tego używała)
  static const String _userKey = 'user';
  static const String _tokenKey = 'auth_token';

  /// Normalizujemy usera tak, żeby nie zapisywać w telefonie wrażliwych danych.
  /// Backend w tym projekcie potrafi zwrócić cały obiekt User.
  static Map<String, dynamic> _normalizeUserJson(Map<String, dynamic> json) {
    return {
      'id': json['id']?.toString(),
      'email': json['email']?.toString(),
      'username': json['username']?.toString(),
    };
  }

  /// Pobiera aktualnie zalogowanego usera z telefonu.
  static Future<Map<String, dynamic>?> getCurrentUser() async {
    final prefs = await SharedPreferences.getInstance();
    final userStr = prefs.getString(_userKey);
    if (userStr == null || userStr.isEmpty) return null;
    return jsonDecode(userStr) as Map<String, dynamic>;
  }

  /// Walidacja formatu email
  /// Zwraca: true jeśli email ma poprawny format, false w przeciwnym razie
  static bool isValidEmail(String email) {
    // Regex dla email'a - najprostszy format: coś@coś.coś
    final emailRegex = RegExp(
      r'^[a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$',
    );
    return emailRegex.hasMatch(email);
  }

  /// Rejestracja nowego użytkownika
  /// 
  /// [username] - nazwa użytkownika
  /// [email] - email użytkownika
  /// [password] - hasło
  /// 
  /// Zwraca: true jeśli rejestracja się powiodła, false jeśli błąd
  static Future<bool> register({
    required String username,
    required String email,
    required String password,
  }) async {
    try {
      // Tworzymy JSON body z danymi do wysłania
      final requestBody = jsonEncode({
        'username': username,
        'email': email,
        'password': password,
      });

      // Wysyłamy POST request do backendu
      final response = await http
          .post(
            await _uri('/auth/register'),
        headers: {
          'Content-Type': 'application/json',
        },
        body: requestBody,
      )
          .timeout(_requestTimeout);

      // Jeśli status 200-299 to znaczy że sukces
      if (response.statusCode >= 200 && response.statusCode < 300) {
        return true;
      } else {
        // Jeśli inny kod to błąd
        print('Register error: ${response.body}');
        return false;
      }
    } catch (e) {
      // Jeśli coś poszło nie tak (brak internetu, timeout itp)
      print('Register exception: $e');
      throw Exception('Brak połączenia z backendem. Sprawdź API URL/hotspot/firewall. ($e)');
    }
  }

  /// Logowanie użytkownika
  ///
  /// [email] - email użytkownika
  /// [password] - hasło
  /// 
  /// Zwraca: obiekt user (Map) jeśli logowanie się powiodło, lub null jeśli błąd
  ///
  /// Uwaga: backend w tym projekcie zwraca obiekt User jako JSON.
  /// My zapisujemy minimalne dane usera w SharedPreferences, żeby potem pobierać
  /// urządzenia dla tego użytkownika.
  static Future<Map<String, dynamic>?> login({
    required String email,
    required String password,
  }) async {
    try {
      // Tworzymy JSON body z danymi logowania
      final requestBody = jsonEncode({
        'email': email,
        'password': password,
      });

      // Wysyłamy POST request do backendu
      final response = await http
          .post(
            await _uri('/auth/login'),
        headers: {
          'Content-Type': 'application/json',
        },
        body: requestBody,
      )
          .timeout(_requestTimeout);

      // Jeśli sukces (200)
      if (response.statusCode == 200) {
        final prefs = await SharedPreferences.getInstance();

        // Legacy: nadal zapisujemy body do auth_token, żeby nie psuć starego flow.
        await prefs.setString(_tokenKey, response.body);

        // Spróbuj sparsować JSON usera.
        // Jeśli backend zwraca coś innego (np. token), to nie będziemy mieli userId
        // i wtedy lista urządzeń nie zadziała – pokażemy to w UI.
        final decoded = jsonDecode(response.body);
        if (decoded is Map<String, dynamic>) {
          final user = _normalizeUserJson(decoded);
          if (user['id'] != null) {
            await prefs.setString(_userKey, jsonEncode(user));
          }
          return user;
        }

        // Nieoczekiwany format
        return null;
      } else {
        // Jeśli błąd logowania (401 = unauthorized)
        print('Login error: ${response.body}');
        return null;
      }
    } catch (e) {
      // Jeśli coś poszło nie tak (brak internetu, timeout itp)
      print('Login exception: $e');
      throw Exception('Brak połączenia z backendem. Sprawdź API URL/hotspot/firewall. ($e)');
    }
  }

  /// Pobranie zapisanego tokenu z telefonu (legacy)
  static Future<String?> getToken() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString(_tokenKey);
  }

  /// Wylogowanie (usunięcie tokenu z telefonu)
  static Future<void> logout() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove(_tokenKey);
    await prefs.remove(_userKey);
  }

  /// Pobierz listę urządzeń użytkownika
  /// Endpoint backendu: GET /api/devices/user/{userId}
  static Future<List<Map<String, dynamic>>> getUserDevices({
    required String userId,
  }) async {
    try {
      final response = await http
          .get(
            await _uri('/devices/user/$userId'),
            headers: {
              'Content-Type': 'application/json',
            },
          )
          .timeout(_requestTimeout);

      if (response.statusCode == 200) {
        final decoded = jsonDecode(response.body);
        if (decoded is List) {
          return decoded
              .whereType<Map<String, dynamic>>()
              .toList(growable: false);
        }
      }

      print('Get devices error: ${response.statusCode} ${response.body}');
      return [];
    } catch (e) {
      print('Get devices exception: $e');
      throw Exception('Nie udało się pobrać urządzeń. ($e)');
    }
  }

  /// Pobierz pomiary temperatury z ostatnich 24h
  /// Endpoint backendu: GET /api/temperatures
  static Future<List<Map<String, dynamic>>> getTemperatures({
    required String deviceMac,
    required String ownerId,
  }) async {
    try {
      final from = DateTime.now()
          .subtract(const Duration(hours: 24))
          .toUtc()
          .toIso8601String();
      final uri = (await _uri('/temperatures')).replace(queryParameters: {
        'deviceMac': deviceMac,
        'ownerId': ownerId,
        'from': from,
      });
      final response = await http
          .get(
            uri,
            headers: {
              'Content-Type': 'application/json',
            },
          )
          .timeout(_requestTimeout);

      if (response.statusCode == 200) {
        final decoded = jsonDecode(response.body);
        if (decoded is List) {
          return decoded.whereType<Map<String, dynamic>>().toList();
        }
      }
      return [];
    } catch (e) {
      print('Get temperatures exception: $e');
      return [];
    }
  }

  /// Pobierz pomiary ciśnienia z ostatnich 24h
  /// Endpoint backendu: GET /api/pressures
  static Future<List<Map<String, dynamic>>> getPressures({
    required String deviceMac,
    required String ownerId,
  }) async {
    try {
      final from = DateTime.now()
          .subtract(const Duration(hours: 24))
          .toUtc()
          .toIso8601String();
      final uri = (await _uri('/pressures')).replace(queryParameters: {
        'deviceMac': deviceMac,
        'ownerId': ownerId,
        'from': from,
      });
      final response = await http
          .get(
            uri,
            headers: {
              'Content-Type': 'application/json',
            },
          )
          .timeout(_requestTimeout);

      if (response.statusCode == 200) {
        final decoded = jsonDecode(response.body);
        if (decoded is List) {
          return decoded.whereType<Map<String, dynamic>>().toList();
        }
      }
      return [];
    } catch (e) {
      print('Get pressures exception: $e');
      return [];
    }
  }

  /// Pobierz pomiary wilgotności gleby z ostatnich 24h
  /// Endpoint backendu: GET /api/soil-measurements
  static Future<List<Map<String, dynamic>>> getSoilMeasurements({
    required String deviceMac,
    required String ownerId,
  }) async {
    try {
      final from = DateTime.now()
          .subtract(const Duration(hours: 24))
          .toUtc()
          .toIso8601String();
      final uri = (await _uri('/soil-measurements')).replace(queryParameters: {
        'deviceMac': deviceMac,
        'ownerId': ownerId,
        'from': from,
      });
      final response = await http
          .get(
            uri,
            headers: {
              'Content-Type': 'application/json',
            },
          )
          .timeout(_requestTimeout);

      if (response.statusCode == 200) {
        final decoded = jsonDecode(response.body);
        if (decoded is List) {
          return decoded.whereType<Map<String, dynamic>>().toList();
        }
      }
      return [];
    } catch (e) {
      print('Get soil measurements exception: $e');
      return [];
    }
  }

  /// Pobierz zdarzenia monet z ostatnich 24h
  /// Endpoint backendu: GET /api/coin-events
  static Future<List<Map<String, dynamic>>> getCoinEvents({
    required String deviceMac,
    required String ownerId,
  }) async {
    try {
      final from = DateTime.now()
          .subtract(const Duration(hours: 24))
          .toUtc()
          .toIso8601String();
      final uri = (await _uri('/coin-events')).replace(queryParameters: {
        'deviceMac': deviceMac,
        'ownerId': ownerId,
        'from': from,
      });
      final response = await http
          .get(
            uri,
            headers: {
              'Content-Type': 'application/json',
            },
          )
          .timeout(_requestTimeout);

      if (response.statusCode == 200) {
        final decoded = jsonDecode(response.body);
        if (decoded is List) {
          return decoded.whereType<Map<String, dynamic>>().toList();
        }
      }
      return [];
    } catch (e) {
      print('Get coin events exception: $e');
      return [];
    }
  }

  /// Wyślij komendę podlewania
  /// Endpoint backendu: POST /api/devices/{deviceId}/water
  static Future<void> waterDevice({
    required int deviceId,
    required double duration,
  }) async {
    try {
      final response = await http
          .post(
            await _uri('/devices/$deviceId/water'),
            headers: {
              'Content-Type': 'application/json',
            },
            body: jsonEncode({'duration': duration}),
          )
          .timeout(_requestTimeout);

      if (response.statusCode < 200 || response.statusCode >= 300) {
        throw Exception('Błąd podlewania: ${response.body}');
      }
    } catch (e) {
      throw Exception('Nie udało się wysłać komendy podlewania. ($e)');
    }
  }

  /// Zaktualizuj ustawienia urządzenia
  /// Endpoint backendu: PUT /api/devices/{deviceId}/settings
  static Future<void> updateDeviceSettings({
    required int deviceId,
    required int interval,
    required bool holidayMode,
    required int soilMin,
    required int soilMax,
  }) async {
    try {
      final response = await http
          .put(
            await _uri('/devices/$deviceId/settings'),
            headers: {
              'Content-Type': 'application/json',
            },
            body: jsonEncode({
              'interval': interval,
              'holidayMode': holidayMode,
              'soilMin': soilMin,
              'soilMax': soilMax,
            }),
          )
          .timeout(_requestTimeout);

      if (response.statusCode < 200 || response.statusCode >= 300) {
        throw Exception('Błąd aktualizacji ustawień: ${response.body}');
      }
    } catch (e) {
      throw Exception('Nie udało się zaktualizować ustawień. ($e)');
    }
  }
}
