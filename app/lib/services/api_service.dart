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
  /// Zwraca: token JWT jeśli logowanie się powiodło, lub null jeśli błąd
  static Future<String?> login({
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

      // Jeśli sukces (200) to backend zwraca token
      if (response.statusCode == 200) {
        // Backend zwraca token w body
        final token = response.body;
        
        // Zapisujemy token w SharedPreferences (lokalnie na telefonie)
        final prefs = await SharedPreferences.getInstance();
        await prefs.setString('auth_token', token);
        
        return token;
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

  /// Pobranie zapisanego tokenu z telefonu
  static Future<String?> getToken() async {
    final prefs = await SharedPreferences.getInstance();
    return prefs.getString('auth_token');
  }

  /// Wylogowanie (usunięcie tokenu z telefonu)
  static Future<void> logout() async {
    final prefs = await SharedPreferences.getInstance();
    await prefs.remove('auth_token');
  }
}
