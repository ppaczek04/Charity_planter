import 'package:flutter/material.dart';
import 'devices_list_screen.dart';

class RegistrationScreen extends StatelessWidget {
  const RegistrationScreen({super.key});

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

              const TextField(
                decoration: InputDecoration(
                  labelText: 'Nazwa użytkownika',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 15),

              const TextField(
                decoration: InputDecoration(
                  labelText: 'Email',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 15),

              const TextField(
                obscureText: true,
                decoration: InputDecoration(
                  labelText: 'Hasło',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 15),

              const TextField(
                decoration: InputDecoration(
                  labelText: 'MAC Telefonu (np. AA:BB:CC...)',
                  border: OutlineInputBorder(),
                ),
              ),
              const SizedBox(height: 20),

              ElevatedButton(
                onPressed: () {
                  Navigator.push(
                    context,
                    MaterialPageRoute(
                      builder: (context) => const DevicesListScreen(),
                    ),
                  );
                },
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
}
