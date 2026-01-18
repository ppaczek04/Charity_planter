import 'package:flutter/material.dart';
import 'services/api_service.dart';
import 'login_screen.dart';

class DevicesListScreen extends StatefulWidget {
  const DevicesListScreen({super.key});

  @override
  State<DevicesListScreen> createState() => _DevicesListScreenState();
}

class _DevicesListScreenState extends State<DevicesListScreen> {
  Future<List<Map<String, dynamic>>>? _devicesFuture;

  @override
  void initState() {
    super.initState();
    _devicesFuture = _loadDevices();
  }

  Future<List<Map<String, dynamic>>> _loadDevices() async {
    final user = await ApiService.getCurrentUser();
    final userId = user?['id'] as String?;

    if (userId == null || userId.isEmpty) {
      if (mounted) {
        Navigator.of(context).pushAndRemoveUntil(
          MaterialPageRoute(builder: (context) => const LoginScreen()),
          (route) => false,
        );
      }
      return [];
    }

    return ApiService.getUserDevices(userId: userId);
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: const Text(
          'Moje urządzenia',
          style: TextStyle(color: Colors.black),
        ),
        backgroundColor: Colors.white,
        centerTitle: false,
        automaticallyImplyLeading: false,
        actions: [
          IconButton(
            icon: const Icon(Icons.logout, color: Colors.black),
            onPressed: () async {
              // Wylogowanie użytkownika
              await ApiService.logout();
              
              // Sprawdzamy czy widget jest nadal w drzewie (bezpieczeństwo)
              if (context.mounted) {
                // Przekierowanie do ekranu logowania i usunięcie historii
                Navigator.of(context).pushAndRemoveUntil(
                  MaterialPageRoute(
                    builder: (context) => const LoginScreen(),
                  ),
                  (route) => false,
                );
              }
            },
          ),
        ],
      ),
      body: Padding(
        padding: const EdgeInsets.all(16.0),
        child: FutureBuilder<List<Map<String, dynamic>>>(
          future: _devicesFuture,
          builder: (context, snapshot) {
            if (snapshot.connectionState == ConnectionState.waiting) {
              return const Center(child: CircularProgressIndicator());
            }

            if (snapshot.hasError) {
              return Center(
                child: Column(
                  mainAxisSize: MainAxisSize.min,
                  children: [
                    Text('Błąd pobierania urządzeń: ${snapshot.error}'),
                    const SizedBox(height: 12),
                    ElevatedButton(
                      onPressed: () {
                        setState(() {
                          _devicesFuture = _loadDevices();
                        });
                      },
                      child: const Text('Spróbuj ponownie'),
                    ),
                  ],
                ),
              );
            }

            final devices = snapshot.data ?? [];

            if (devices.isEmpty) {
              return Column(
                crossAxisAlignment: CrossAxisAlignment.stretch,
                children: [
                  const Text(
                    'Nie masz jeszcze żadnych doniczek.',
                    style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
                  ),
                  const SizedBox(height: 12),
                  const Text('Dodaj urządzenie, żeby rozpocząć.'),
                  const SizedBox(height: 20),
                  _buildAddDeviceButton(),
                  const SizedBox(height: 12),
                  ElevatedButton(
                    onPressed: () {
                      setState(() {
                        _devicesFuture = _loadDevices();
                      });
                    },
                    child: const Text('Odśwież'),
                  ),
                ],
              );
            }

            return RefreshIndicator(
              onRefresh: () async {
                setState(() {
                  _devicesFuture = _loadDevices();
                });
                await _devicesFuture;
              },
              child: ListView.builder(
                itemCount: devices.length + 1,
                itemBuilder: (context, index) {
                  if (index == devices.length) {
                    return _buildAddDeviceButton();
                  }

                  final dev = devices[index];
                  final name = (dev['name'] as String?)?.trim();
                  final mac = (dev['mac'] as String?)?.trim();

                  return _buildDeviceCard(
                    name: (name == null || name.isEmpty) ? 'Bez nazwy' : name,
                    mac: mac ?? '-',
                  );
                },
              ),
            );
          },
        ),
      ),
    );
  }

  Widget _buildAddDeviceButton() {
    return Card(
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: const BorderSide(color: Colors.black12),
      ),
      margin: const EdgeInsets.only(bottom: 12),
      child: InkWell(
        onTap: () {
          print("Kliknięto Dodaj urządzenie");
        },
        borderRadius: BorderRadius.circular(12),
        child: const Padding(
          padding: EdgeInsets.all(20.0),
          child: Row(
            children: [
              Text(
                'Dodaj urządzenie +',
                style: TextStyle(fontSize: 18, fontWeight: FontWeight.bold),
              ),
            ],
          ),
        ),
      ),
    );
  }

  Widget _buildDeviceCard({required String name, required String mac}) {
    return Card(
      elevation: 2,
      shape: RoundedRectangleBorder(
        borderRadius: BorderRadius.circular(12),
        side: const BorderSide(color: Colors.black12),
      ),
      margin: const EdgeInsets.only(bottom: 12),
      child: InkWell(
        onTap: () {
          print("Kliknięto $name ($mac)");
        },
        borderRadius: BorderRadius.circular(12),
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Row(
            children: [
              Container(
                width: 50,
                height: 50,
                decoration: BoxDecoration(
                  color: Colors.green.shade50,
                  borderRadius: BorderRadius.circular(8),
                ),
                child: const Icon(
                  Icons.local_florist,
                  color: Colors.green,
                  size: 30,
                ),
              ),
              const SizedBox(width: 16),
              Expanded(
                child: Column(
                  crossAxisAlignment: CrossAxisAlignment.start,
                  children: [
                    Text(
                      name,
                      style: const TextStyle(
                        fontSize: 18,
                        fontWeight: FontWeight.w500,
                      ),
                    ),
                    const SizedBox(height: 4),
                    Text(
                      'MAC: $mac',
                      style: const TextStyle(color: Colors.black54),
                    ),
                  ],
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
