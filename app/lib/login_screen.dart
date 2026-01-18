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

  Future<void> _openApiSettings() async {
    final current = await ApiService.getBaseUrl();
    final controller = TextEditingController(text: current);

    if (!mounted) return;
    await showDialog<void>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Adres backendu (API URL)'),
          content: Column(
            mainAxisSize: MainAxisSize.min,
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              const Text(
                'Podaj adres backendu.\n\n'
                'Domyślnie działa pod USB (adb reverse): http://127.0.0.1:8080/api\n'
                'Przy hotspot/WiFi: użyj IP LAPTOPA z ipconfig (nie IP telefonu).\n'
                'Np. http://10.160.120.188:8080/api',
              ),
              const SizedBox(height: 12),
              TextField(
                controller: controller,
                decoration: const InputDecoration(
                  labelText: 'API URL',
                  hintText: 'http://127.0.0.1:8080/api',
                ),
              ),
            ],
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.pop(context),
              child: const Text('Anuluj'),
            ),
            ElevatedButton(
              onPressed: () async {
                await ApiService.setBaseUrl(controller.text);
                if (context.mounted) {
                  Navigator.pop(context);
                  ScaffoldMessenger.of(context).showSnackBar(
                    const SnackBar(content: Text('Zapisano adres backendu.')),
                  );
                }
              },
              child: const Text('Zapisz'),
            ),
          ],
        );
      },
    );
  }

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
          IconButton(
            icon: const Icon(Icons.settings, color: Colors.black),
            onPressed: _isLoading ? null : _openApiSettings,
          ),
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
      final user = await ApiService.login(
        email: email,
        password: password,
      );

      // Sprawdzamy czy logowanie się powiodło
      if (user != null && user['id'] != null) {
        // Sukces - user został zapisany w SharedPreferences
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
          SnackBar(content: Text(e.toString())),
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