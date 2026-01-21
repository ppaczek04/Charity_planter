import 'dart:async';
import 'dart:convert';
import 'dart:io';

import 'package:flutter/material.dart';
import 'package:flutter_blue_plus/flutter_blue_plus.dart';
import 'package:permission_handler/permission_handler.dart';

import 'services/api_service.dart';

class AddDeviceScreen extends StatelessWidget {
	const AddDeviceScreen({super.key});

	@override
	Widget build(BuildContext context) {
		return const AddDeviceScanScreen();
	}
}

class AddDeviceScanScreen extends StatefulWidget {
	const AddDeviceScanScreen({super.key});

	@override
	State<AddDeviceScanScreen> createState() => _AddDeviceScanScreenState();
}

class _AddDeviceScanScreenState extends State<AddDeviceScanScreen> {
	StreamSubscription<List<ScanResult>>? _scanSub;
	StreamSubscription<bool>? _isScanningSub;

	bool _isScanning = false;
	bool _isConnecting = false;
	final List<ScanResult> _scanResults = [];

	String _status = '';

	@override
	void initState() {
		super.initState();

		_scanSub = FlutterBluePlus.scanResults.listen((results) {
			if (!mounted) return;
			setState(() {
				_scanResults
					..clear()
					..addAll(results);
			});
		});

		_isScanningSub = FlutterBluePlus.isScanning.listen((scanning) {
			if (!mounted) return;
			setState(() => _isScanning = scanning);
		});
	}

	@override
	void dispose() {
		_scanSub?.cancel();
		_isScanningSub?.cancel();
		FlutterBluePlus.stopScan();
		super.dispose();
	}

	Future<void> _ensurePermissions() async {
		if (!Platform.isAndroid) return;

		final requests = <Permission>[
			Permission.bluetoothScan,
			Permission.bluetoothConnect,
			Permission.bluetoothAdvertise,
			Permission.locationWhenInUse,
		];

		final result = await requests.request();
		final denied = result.entries.where((e) => !e.value.isGranted).toList();
		if (denied.isNotEmpty) {
			throw Exception('Brak uprawnień Bluetooth/Lokalizacja.');
		}
	}

	Future<void> _scanForDevices() async {
		try {
			setState(() {
				_status = 'Szukam urządzenia...';
				_scanResults.clear();
			});

			await _ensurePermissions();

			final state = await FlutterBluePlus.adapterState.first;
			if (state != BluetoothAdapterState.on) {
				setState(() => _status = 'Włącz Bluetooth i spróbuj ponownie.');
				return;
			}

			await FlutterBluePlus.stopScan();
			await FlutterBluePlus.startScan(
				timeout: const Duration(seconds: 10),
			);

			await Future.delayed(const Duration(seconds: 10));
			if (!mounted) return;
			if (_scanResults.isEmpty) {
				setState(() => _status = 'Nie znaleziono urządzeń. Upewnij się, że doniczka reklamuje BLE.');
			}
		} catch (e) {
			if (!mounted) return;
			setState(() => _status = 'Błąd skanowania: $e');
		}
	}

	Future<void> _connectAndOpenForm(ScanResult result) async {
		if (_isConnecting) return;

		await FlutterBluePlus.stopScan();

		setState(() {
			_isConnecting = true;
			_status = 'Łączenie z urządzeniem...';
		});

		try {
			await _ensurePermissions();

			final success = await Navigator.of(context).push<bool>(
				MaterialPageRoute(
					builder: (_) => AddDeviceFormScreen(device: result.device),
				),
			);

			if (success == true && mounted) {
				ScaffoldMessenger.of(context).showSnackBar(
					const SnackBar(content: Text('Urządzenie skonfigurowane.')),
				);
				Navigator.of(context).pop(true);
			}
		} catch (e) {
			if (mounted) {
				setState(() => _status = 'Błąd połączenia: $e');
			}
		} finally {
			if (mounted) setState(() => _isConnecting = false);
		}
	}

	@override
	Widget build(BuildContext context) {
		return Scaffold(
			appBar: AppBar(
				title: const Text('Dodaj urządzenie'),
			),
			body: SafeArea(
				child: SingleChildScrollView(
					padding: const EdgeInsets.all(16),
					child: Column(
						crossAxisAlignment: CrossAxisAlignment.stretch,
						children: [
							FilledButton.icon(
								onPressed: (_isScanning || _isConnecting) ? null : _scanForDevices,
								icon: const Icon(Icons.search),
								label: Text(_isScanning ? 'Skanowanie...' : 'Szukaj doniczki'),
							),
							const SizedBox(height: 12),
							if (_scanResults.isNotEmpty) ...[
								const Text(
									'Znalezione urządzenia',
									style: TextStyle(fontSize: 16, fontWeight: FontWeight.w700),
								),
								const SizedBox(height: 8),
								ListView.separated(
									shrinkWrap: true,
									physics: const NeverScrollableScrollPhysics(),
									itemCount: _scanResults.length,
									separatorBuilder: (_, __) => const Divider(height: 1),
									itemBuilder: (context, i) {
										final r = _scanResults[i];
										final name = r.device.platformName.trim();
										final title = name.isEmpty ? 'Nieznane urządzenie' : name;

										return ListTile(
											title: Text(title),
											subtitle: Text(r.device.remoteId.str),
											trailing: _isConnecting
													? const SizedBox(
															width: 20,
															height: 20,
															child: CircularProgressIndicator(strokeWidth: 2),
														)
													: const Icon(Icons.link),
											onTap: _isConnecting ? null : () => _connectAndOpenForm(r),
										);
									},
								),
							],
							if (_status.trim().isNotEmpty) ...[
								const SizedBox(height: 12),
								Text(
									'Status: $_status',
									style: const TextStyle(color: Colors.black54),
									textAlign: TextAlign.center,
								),
							],
						],
					),
				),
			),
		);
	}
}

class AddDeviceFormScreen extends StatefulWidget {
	const AddDeviceFormScreen({super.key, required this.device});

	final BluetoothDevice device;

	@override
	State<AddDeviceFormScreen> createState() => _AddDeviceFormScreenState();
}

class _AddDeviceFormScreenState extends State<AddDeviceFormScreen> {
	static const String _service16 = '00ff';
	static const String _charSsid16 = 'ff01';
	static const String _charPass16 = 'ff02';
	static const String _charMqtt16 = 'ff03';
	static const String _charMac16 = 'ff06';
	static const String _mqttBrokerUrl = 'mqtt://13.63.7.113';
	static const String _charOwnerId = 'ff07';

	static bool _guidMatches(Guid guid, String uuid16) {
		final a = guid.toString().toLowerCase().replaceAll('-', '');
		final short = uuid16.toLowerCase().replaceAll('0x', '').padLeft(4, '0');

		if (a == short) return true;
		if (a.startsWith('0000$short')) return true;
		if (a.endsWith(short)) return true;
		return false;
	}

	BluetoothService? _service;
	bool _isConnecting = false;
	bool _isSending = false;
	String _status = '';

	final _ssidController = TextEditingController();
	final _passwordController = TextEditingController();

	@override
	void initState() {
		super.initState();
		_connectAndDiscover();
	}

	@override
	void dispose() {
		_disconnect();
		_ssidController.dispose();
		_passwordController.dispose();
		super.dispose();
	}

	Future<void> _connectAndDiscover() async {
		setState(() {
			_isConnecting = true;
			_status = 'Łączenie z urządzeniem...';
		});

		try {
			await widget.device.connect(timeout: const Duration(seconds: 12));
			final services = await widget.device.discoverServices();
			final svc = services.firstWhere(
				(s) => _guidMatches(s.uuid, _service16),
				orElse: () => services.first,
			);

			if (!_guidMatches(svc.uuid, _service16)) {
				final list = services.map((s) => s.uuid.toString()).join(', ');
				throw Exception('Nie znaleziono serwisu BLE 0x00FF. Dostępne: $list');
			}

			setState(() {
				_service = svc;
				_status = 'Połączono! Wpisz dane WiFi.';
			});
		} catch (e) {
			setState(() => _status = 'Błąd połączenia: $e');
		} finally {
			if (mounted) setState(() => _isConnecting = false);
		}
	}

	Future<void> _disconnect() async {
		try {
			await widget.device.disconnect();
		} catch (_) {}
	}

	BluetoothCharacteristic? _findChar(BluetoothService s, String uuid16) {
		for (final c in s.characteristics) {
			if (_guidMatches(c.uuid, uuid16)) return c;
		}
		return null;
	}

	Future<String> _readMac(BluetoothService svc) async {
		final macChar = _findChar(svc, _charMac16);
		if (macChar == null) {
			throw Exception('Brak charakterystyki MAC (FF06).');
		}
		final value = await macChar.read();
		final decoded = utf8.decode(value, allowMalformed: true);
		return decoded.replaceAll('\u0000', '').trim();
	}

	Future<void> _sendConfig() async {
		if (_isSending || _isConnecting) return;
		final svc = _service;
		if (svc == null) {
			setState(() => _status = 'Najpierw połącz się z urządzeniem.');
			return;
		}

		final ssid = _ssidController.text.trim();
		final pass = _passwordController.text;

		if (ssid.isEmpty) {
			setState(() => _status = 'SSID jest wymagane.');
			return;
		}

		setState(() {
			_isSending = true;
			_status = 'Wysyłanie konfiguracji WiFi...';
		});

		try {
			final ssidChar = _findChar(svc, _charSsid16);
			final passChar = _findChar(svc, _charPass16);
			final mqttChar = _findChar(svc, _charMqtt16);
			final ownerChar = _findChar(svc, _charOwnerId);
			if (ssidChar == null || passChar == null || mqttChar == null) {
				throw Exception('Brak charakterystyk WiFi/MQTT (FF01/FF02/FF03).');
			}

			final user = await ApiService.getCurrentUser();
			final userId = user?['id']?.toString();

			if (userId == null || userId.isEmpty) {
				throw Exception('Brak zalogowanego użytkownika.');
			}

			await ssidChar.write(utf8.encode(ssid), withoutResponse: false);
			await passChar.write(utf8.encode(pass), withoutResponse: false);
			await mqttChar.write(utf8.encode(_mqttBrokerUrl), withoutResponse: false);

			if(ownerChar != null) {
				setState(() => _status = 'Przypisywanie właściciela do urządzenia...');
				await ownerChar.write(utf8.encode(userId), withoutResponse: false);
			}

			final mac = await _readMac(svc);

			await ApiService.claimDevice(deviceMac: mac, userId: userId);

			if (!mounted) return;
			setState(() => _status = 'SUKCES! Urządzenie skonfigurowane.');
			await Future.delayed(const Duration(seconds: 1));
			if (mounted) Navigator.of(context).pop(true);

		} catch (e) {
			setState(() => _status = 'Błąd procesu: $e');
		} finally {
			if (mounted) setState(() => _isSending = false);
		}
	}

	@override
	Widget build(BuildContext context) {
		final connected = _service != null;

		return Scaffold(
			appBar: AppBar(
				title: const Text('Konfiguracja WiFi'),
			),
			body: SafeArea(
				child: SingleChildScrollView(
					padding: const EdgeInsets.all(16),
					child: Column(
						crossAxisAlignment: CrossAxisAlignment.stretch,
						children: [
							Card(
								elevation: 1,
								shape: RoundedRectangleBorder(
									borderRadius: BorderRadius.circular(14),
									side: const BorderSide(color: Colors.black12),
								),
								child: Padding(
									padding: const EdgeInsets.all(16),
									child: Column(
										crossAxisAlignment: CrossAxisAlignment.stretch,
										children: [
											Row(
												children: [
													Icon(
														connected
																? Icons.bluetooth_connected
																: Icons.bluetooth,
														color: connected ? Colors.green : Colors.black45,
													),
													const SizedBox(width: 8),
													Expanded(
														child: Text(
															connected
																	? 'Połączono z ESP32'
																	: 'Łączenie...',
															style: const TextStyle(
																fontSize: 16,
																fontWeight: FontWeight.w700,
															),
														),
													),
												],
											),
											const SizedBox(height: 12),
											TextField(
												controller: _ssidController,
												enabled: connected && !_isSending,
												decoration: const InputDecoration(
													labelText: 'Nazwa sieci WiFi (SSID)',
													border: OutlineInputBorder(),
												),
											),
											const SizedBox(height: 12),
											TextField(
												controller: _passwordController,
												enabled: connected && !_isSending,
												obscureText: true,
												decoration: const InputDecoration(
													labelText: 'Hasło WiFi',
													border: OutlineInputBorder(),
												),
											),
											const SizedBox(height: 12),
											FilledButton(
												onPressed:
														(!connected || _isSending) ? null : _sendConfig,
												child: Padding(
													padding: const EdgeInsets.symmetric(vertical: 12),
													child: _isSending
															? const SizedBox(
																	width: 20,
																	height: 20,
																	child: CircularProgressIndicator(
																		strokeWidth: 2,
																	),
																)
															: const Text('Zapisz i dodaj'),
												),
											),
										],
									),
								),
							),
							if (_status.trim().isNotEmpty) ...[
								const SizedBox(height: 12),
								Text(
									'Status: $_status',
									style: const TextStyle(color: Colors.black54),
									textAlign: TextAlign.center,
								),
							],
						],
					),
				),
			),
		);
	}
}
