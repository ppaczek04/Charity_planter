import 'package:http/http.dart' as http;
import 'dart:convert';
import 'package:shared_preferences/shared_preferences.dart';

/// ApiService - klasa do komunikacji z backendem
/// Tutaj zbieramy logikę HTTP requests w jedno miejsce
/// Dzięki temu kod ekranów pozostaje czysty i czytelny
class ApiService {
  // URL backendu - używamy IP komputera bo aplikacja mobilna łączy się z Docker'em na hoście
  // localhost by się nie rozpoznał w emulatorze - musimy IP maszyny hosta
  // wpisz w cmd ipconfig -> IPv4 -> wpisz w baseUrl twój adres IP
  static const String baseUrl = 'http://192.168.0.17:8080/api';

  /// Walidacja formatu email
  /// Zwraca: true jeśli email ma poprawny format, false w przeciwnym razie
  static bool isValidEmail(String email) {
    // Regex dla email'a - najprostszy format: coś@coś.coś
    final emailRegex = RegExp(
      r'^[a-zA-Z0-9._%-]+@[a-zA-Z0-9.-]+\.[a-zA-Z]{2,}$',
    );
    return emailRegex.hasMatch(email);
  }

  /// Walidacja formatu MAC Address
  /// Poprawny format: AA:BB:CC:DD:EE:FF lub aa:bb:cc:dd:ee:ff
  /// Zwraca: true jeśli MAC ma poprawny format, false w przeciwnym razie
  static bool isValidMacAddress(String mac) {
    // Regex dla MAC address'u: 6 par heksadecymalnych rozdzielonych dwukropkami
    final macRegex = RegExp(r'^([0-9A-Fa-f]{2}[:]){5}([0-9A-Fa-f]{2})$');
    return macRegex.hasMatch(mac);
  }

  /// Rejestracja nowego użytkownika
  /// 
  /// [username] - nazwa użytkownika
  /// [email] - email użytkownika
  /// [password] - hasło
  /// [mobileMacAddress] - MAC adres telefonu (opcjonalnie)
  /// 
  /// Zwraca: true jeśli rejestracja się powiodła, false jeśli błąd
  static Future<bool> register({
    required String username,
    required String email,
    required String password,
    required String mobileMacAddress,
  }) async {
    try {
      // Tworzymy JSON body z danymi do wysłania
      final requestBody = jsonEncode({
        'username': username,
        'email': email,
        'password': password,
        'mobileMacAddress': mobileMacAddress,
      });

      // Wysyłamy POST request do backendu
      final response = await http.post(
        Uri.parse('$baseUrl/auth/register'),
        headers: {
          'Content-Type': 'application/json',
        },
        body: requestBody,
      );

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
      return false;
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
      final response = await http.post(
        Uri.parse('$baseUrl/auth/login'),
        headers: {
          'Content-Type': 'application/json',
        },
        body: requestBody,
      );

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
      return null;
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
