import 'package:flutter/material.dart';
import 'registration_screen.dart';
import 'devices_list_screen.dart';
import 'services/api_service.dart';

/// LoginScreen - ekran logowania użytkownika
/// StatefulWidget bo musimy przechowywać dane z TextFieldów
/// i śledzić stan wysyłania (loading/error/success)
class LoginScreen extends StatefulWidget {
  const LoginScreen({super.key});

  @override
  State<LoginScreen> createState() => _LoginScreenState();
}

class _LoginScreenState extends State<LoginScreen> {
  // Kontrolery do TextFieldów - przechowują wartości wpisane przez użytkownika
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();

  // Flaga czy wysyłka jest w trakcie (do pokazania loading spinner)
  bool _isLoading = false;

  @override
  void dispose() {
    // Ważne! Usuwamy kontrolery żeby nie było memory leak'u
    _emailController.dispose();
    _passwordController.dispose();
    super.dispose();
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Charity planter',
          style: TextStyle(color: Colors.black),
        ),
        backgroundColor: Colors.white,
        centerTitle: false,
        actions: [
          TextButton(
            onPressed: () {
              Navigator.push(
                context,
                MaterialPageRoute(
                  builder: (context) => const RegistrationScreen(),
                ),
              );
            },
            child: const Text(
              'Rejestracja',
              style: TextStyle(color: Colors.black),
            ),
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(20.0),
        child: Column(
          mainAxisAlignment: MainAxisAlignment.center,
          crossAxisAlignment: CrossAxisAlignment.stretch,
          children: [
            const Text(
              'Logowanie',
              textAlign: TextAlign.center,
              style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
            ),
            const SizedBox(height: 30),
            
            // Pole: Email
            TextField(
              controller: _emailController,
              enabled: !_isLoading, // Wyłączamy pole podczas wysyłki
              decoration: const InputDecoration(
                labelText: 'Email',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 15),
            
            // Pole: Hasło
            TextField(
              controller: _passwordController,
              enabled: !_isLoading,
              obscureText: true, // Ukrywamy znaki hasła
              decoration: const InputDecoration(
                labelText: 'Hasło',
                border: OutlineInputBorder(),
              ),
            ),
            const SizedBox(height: 20),
            
            // Przycisk Zaloguj
            // Pokazujemy loading spinner gdy wysyłka w trakcie
            _isLoading
                ? const Center(
                    child: CircularProgressIndicator(),
                  )
                : ElevatedButton(
                    onPressed: _handleLogin,
                    style: ElevatedButton.styleFrom(
                      padding: const EdgeInsets.symmetric(vertical: 15),
                    ),
                    child: const Text('Zaloguj'),
                  ),
          ],
        ),
      ),
    );
  }

  /// Funkcja obsługi logowania
  /// Wysyła dane do backendu i obsługuje odpowiedź
  Future<void> _handleLogin() async {
    // Pobranie wartości z TextFieldów
    final email = _emailController.text.trim();
    final password = _passwordController.text;

    // Walidacja - czy pola nie są puste
    if (email.isEmpty || password.isEmpty) {
      // Pokazujemy error SnackBar (wyskakujący komunikat u dołu ekranu)
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Email i hasło są wymagane')),
      );
      return;
    }

    // Walidacja formatu email
    if (!ApiService.isValidEmail(email)) {
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(
          content: Text('Email ma nieprawidłowy format (np. user@example.com)'),
        ),
      );
      return;
    }

    // Ustawiamy flagę _isLoading na true - pokazuje się loading spinner
    setState(() {
      _isLoading = true;
    });

    try {
      // Wysyłamy żądanie logowania do backendu (funkcja z ApiService)
      final token = await ApiService.login(
        email: email,
        password: password,
      );

      // Sprawdzamy czy logowanie się powiodło
      if (token != null) {
        // Sukces - token został zapisany w SharedPreferences
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Zalogowano pomyślnie!')),
          );
          // Nawigujemy do ekranu listy urządzeń
          // pushAndRemoveUntil usuwa wszystkie poprzednie ekrany ze stosu
          // dzięki czemu użytkownik nie może wrócić przyciskiem "back"
          Navigator.of(context).pushAndRemoveUntil(
            MaterialPageRoute(
              builder: (context) => const DevicesListScreen(),
            ),
            (route) => false, // Usuwa wszystkie poprzednie ekrany
          );
        }
      } else {
        // Błąd - pokazujemy komunikat błędu
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Błąd logowania. Sprawdź email i hasło.'),
            ),
          );
        }
      }
    } catch (e) {
      // Jeśli coś poszło nie tak - pokazujemy error
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Błąd: $e')),
        );
      }
    } finally {
      // Niezależnie od wyniku - ukrywamy loading spinner
      if (mounted) {
        setState(() {
          _isLoading = false;
        });
      }
    }
  }
}