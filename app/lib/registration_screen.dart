import 'package:flutter/material.dart';
import 'devices_list_screen.dart';
import 'services/api_service.dart';

/// RegistrationScreen - ekran rejestracji użytkownika
/// StatefulWidget bo musimy przechowywać dane z TextFieldów
/// i śledzić stan wysyłania (loading/error/success)
class RegistrationScreen extends StatefulWidget {
  const RegistrationScreen({super.key});

  @override
  State<RegistrationScreen> createState() => _RegistrationScreenState();
}

class _RegistrationScreenState extends State<RegistrationScreen> {
  // Kontrolery do TextFieldów - przechowują wartości wpisane przez użytkownika
  final _usernameController = TextEditingController();
  final _emailController = TextEditingController();
  final _passwordController = TextEditingController();

  // Flaga czy wysyłka jest w trakcie (do pokazania loading spinner)
  bool _isLoading = false;

  @override
  void dispose() {
    // Ważne! Usuwamy kontrolery żeby nie było memory leak'u
    _usernameController.dispose();
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
        automaticallyImplyLeading: false,
        actions: [
          TextButton(
            onPressed: () {
              Navigator.pop(context);
            },
            child: const Text(
              'Logowanie',
              style: TextStyle(color: Colors.black),
            ),
          ),
        ],
      ),
      body: Center(
        child: SingleChildScrollView(
          padding: const EdgeInsets.all(20.0),
          child: Column(
            mainAxisAlignment: MainAxisAlignment.center,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              const Text(
                'Rejestracja',
                textAlign: TextAlign.center,
                style: TextStyle(fontSize: 24, fontWeight: FontWeight.bold),
              ),
              const SizedBox(height: 30),

              // Pole: Nazwa użytkownika
              TextField(
                controller: _usernameController,
                enabled: !_isLoading, // Wyłączamy pole podczas wysyłki
                decoration: const InputDecoration(
                  labelText: 'Nazwa użytkownika',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 15),

              // Pole: Email
              TextField(
                controller: _emailController,
                enabled: !_isLoading,
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

              // Przycisk Zarejestruj się
              // Pokazujemy loading spinner gdy wysyłka w trakcie
              _isLoading
                  ? const Center(
                      child: CircularProgressIndicator(),
                    )
                  : ElevatedButton(
                      onPressed: _handleRegister,
                      style: ElevatedButton.styleFrom(
                        padding: const EdgeInsets.symmetric(vertical: 15),
                      ),
                      child: const Text('Zarejestruj się'),
                    ),
            ],
          ),
        ),
      ),
    );
  }

  /// Funkcja obsługi rejestracji
  /// Wysyła dane do backendu i obsługuje odpowiedź
  Future<void> _handleRegister() async {
    // Pobranie wartości z TextFieldów
    final username = _usernameController.text.trim();
    final email = _emailController.text.trim();
    final password = _passwordController.text;

    // Walidacja - czy pola nie są puste
    if (username.isEmpty || email.isEmpty || password.isEmpty) {
      // Pokazujemy error SnackBar (wyskakujący komunikat u dołu ekranu)
      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Wszystkie pola są wymagane')),
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
      // Wysyłamy żądanie rejestracji do backendu (funkcja z ApiService)
      final success = await ApiService.register(
        username: username,
        email: email,
        password: password,
      );

      // Sprawdzamy czy rejestracja się powiodła
      if (success) {
        // Sukces - rejestracja powiodła się
        // Teraz logujemy użytkownika automatycznie (bez powrotu do LoginScreen)
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Zarejestrowano pomyślnie! Logowanie...')),
          );

          // Po 500ms robimy auto-login
          Future.delayed(const Duration(milliseconds: 500), () async {
            if (mounted) {
              // Wysyłamy żądanie logowania
              final token = await ApiService.login(
                email: email,
                password: password,
              );

              // Jeśli logowanie się powiodło
              if (token != null && mounted) {
                ScaffoldMessenger.of(context).showSnackBar(
                  const SnackBar(content: Text('Zalogowano pomyślnie!')),
                );
                // Nawigujemy do ekranu listy urządzeń
                Navigator.of(context).pushAndRemoveUntil(
                  MaterialPageRoute(
                    builder: (context) => const DevicesListScreen(),
                  ),
                  (route) => false, // Usuwamy wszystkie poprzednie ekrany
                );
              } else {
                // Logowanie nie powiodło się - wracamy do logowania
                if (mounted) {
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Rejestracja ok, ale logowanie nie powiodło się')),
                  );
                  Navigator.pop(context);
                }
              }
            }
          });
        }
      } else {
        // Błąd - pokazujemy komunikat błędu
        if (mounted) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Błąd rejestracji. Sprawdź dane i spróbuj ponownie.'),
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