import 'package:flutter/material.dart';
import 'services/api_service.dart';

class DeviceDashboardScreen extends StatefulWidget {
  final Map<String, dynamic> device;

  const DeviceDashboardScreen({super.key, required this.device});

  @override
  State<DeviceDashboardScreen> createState() => _DeviceDashboardScreenState();
}

class _DeviceDashboardScreenState extends State<DeviceDashboardScreen> {
  late String _deviceName;
  late String _deviceMac;
  late String _deviceOwnerId;
  late int _deviceId;

  int _selectedIndex = 0;

  // Stats tab state
  int _selectedStatsTab = 0; // 0=Temp, 1=Ciśnienie, 2=Gleba, 3=Monety
  List<Map<String, dynamic>> _statsData = [];
  bool _statsLoading = false;

  // Placeholder settings state
  bool _vacationMode = false; // saved/applied
  bool _vacationModeDraft = false; // edited in Settings, applied on Save
  bool _isSavingSettings = false;
  bool _isWatering = false;
  
  final TextEditingController _manualWaterSecondsController =
      TextEditingController(text: '1');
  final TextEditingController _measurementIntervalSecondsController =
      TextEditingController(text: '5');
  final TextEditingController _stopThresholdController =
      TextEditingController(text: '70');

  @override
  void initState() {
    super.initState();

    final name = (widget.device['name'] as String?)?.trim();
    final mac = (widget.device['mac'] as String?)?.trim();
    final ownerId = widget.device['ownerId'] as String?;
    final id = widget.device['id'] as int?;

    _deviceName = (name == null || name.isEmpty) ? 'Bez nazwy' : name;
    _deviceMac = mac ?? '-';
    _deviceOwnerId = ownerId ?? '';
    _deviceId = id ?? 0;

    // Initialize draft values from saved state.
    _vacationModeDraft = _vacationMode;
    _measurementIntervalSecondsController.text = '5';
    _stopThresholdController.text = '70';
    
    // Load measurements on init
    _loadMeasurements();
  }

  @override
  void dispose() {
    _manualWaterSecondsController.dispose();
    _measurementIntervalSecondsController.dispose();
    _stopThresholdController.dispose();
    super.dispose();
  }

  Future<void> _loadMeasurements() async {
    if (_statsLoading) return;
    
    setState(() => _statsLoading = true);
    try {
      List<Map<String, dynamic>> data = [];
      
      switch (_selectedStatsTab) {
        case 0: // Temp
          data = await ApiService.getTemperatures(
            deviceMac: _deviceMac,
            ownerId: _deviceOwnerId,
          );
          break;
        case 1: // Ciśnienie
          data = await ApiService.getPressures(
            deviceMac: _deviceMac,
            ownerId: _deviceOwnerId,
          );
          break;
        case 2: // Gleba
          data = await ApiService.getSoilMeasurements(
            deviceMac: _deviceMac,
            ownerId: _deviceOwnerId,
          );
          break;
        case 3: // Monety
          data = await ApiService.getCoinEvents(
            deviceMac: _deviceMac,
            ownerId: _deviceOwnerId,
          );
          break;
      }
      
      if (mounted) {
        setState(() => _statsData = data);
      }
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Błąd pobierania danych: $e')),
        );
      }
    } finally {
      if (mounted) setState(() => _statsLoading = false);
    }
  }

  Future<void> _saveSettings() async {
    final interval = int.tryParse(_measurementIntervalSecondsController.text) ?? 5;
    final soilMax = int.tryParse(_stopThresholdController.text) ?? 70;

    setState(() => _isSavingSettings = true);
    try {
      await ApiService.updateDeviceSettings(
        deviceId: _deviceId,
        interval: interval,
        holidayMode: _vacationModeDraft,
        soilMax: soilMax,
      );
      
      setState(() => _vacationMode = _vacationModeDraft);
      
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          const SnackBar(content: Text('Zapisano konfigurację.')),
        );
      }
    } catch (e) {
      if (mounted) {
        final message = e.toString();
        if (message.contains('504')) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Urządzenie nie potwierdziło zmiany interwału (Offline).'),
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Błąd zapisu: $e')),
          );
        }
      }
    } finally {
      if (mounted) setState(() => _isSavingSettings = false);
    }
  }
  
  Future<void> _waterNow() async {
    final duration = double.tryParse(_manualWaterSecondsController.text) ?? 1.0;
    
    setState(() => _isWatering = true);
    try {
      final responseMessage = await ApiService.waterDevice(
        deviceId: _deviceId,
        duration: duration,
      );
      
      if (mounted) {
        final normalized = responseMessage.toLowerCase();
        if (normalized.contains('potwierdzone')) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(content: Text('Podlano roślinę!')),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(
              content: Text(
                responseMessage.isEmpty
                    ? 'Komenda wysłana, brak potwierdzenia urządzenia.'
                    : responseMessage,
              ),
            ),
          );
        }
      }
    } catch (e) {
      if (mounted) {
        final message = e.toString();
        if (message.contains('504')) {
          ScaffoldMessenger.of(context).showSnackBar(
            const SnackBar(
              content: Text('Urządzenie nie odpowiada (Offline).'),
            ),
          );
        } else {
          ScaffoldMessenger.of(context).showSnackBar(
            SnackBar(content: Text('Błąd podlewania: $e')),
          );
        }
      }
    } finally {
      if (mounted) setState(() => _isWatering = false);
    }
  }

  Future<void> _editDeviceName() async {
    final controller = TextEditingController(text: _deviceName);

    final result = await showDialog<String>(
      context: context,
      builder: (context) {
        return AlertDialog(
          title: const Text('Zmień nazwę doniczki'),
          content: TextField(
            controller: controller,
            decoration: const InputDecoration(
              labelText: 'Nazwa',
              border: OutlineInputBorder(),
            ),
            autofocus: true,
            textInputAction: TextInputAction.done,
            onSubmitted: (_) => Navigator.of(context).pop(controller.text),
          ),
          actions: [
            TextButton(
              onPressed: () => Navigator.of(context).pop(),
              child: const Text('Anuluj'),
            ),
            FilledButton(
              onPressed: () => Navigator.of(context).pop(controller.text),
              child: const Text('Zapisz'),
            ),
          ],
        );
      },
    );

    if (!mounted) return;

    final trimmed = result?.trim();
    if (trimmed == null || trimmed.isEmpty) return;

    try {
      await ApiService.renameDevice(deviceId: _deviceId, newName: trimmed);
      if (!mounted) return;
      setState(() {
        _deviceName = trimmed;
      });

      ScaffoldMessenger.of(context).showSnackBar(
        const SnackBar(content: Text('Zmieniono nazwę doniczki.')),
      );
    } catch (e) {
      if (mounted) {
        ScaffoldMessenger.of(context).showSnackBar(
          SnackBar(content: Text('Nie udało się zmienić nazwy: $e')),
        );
      }
    }
  }

  Widget _statsTab() {
    final statsLabels = ['Temp', 'Ciśnienie', 'Gleba', 'Monety'];
    
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        Row(
          mainAxisAlignment: MainAxisAlignment.spaceBetween,
          children: [
            const Text(
              'Statystyki',
              style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
            ),
            IconButton(
              icon: const Icon(Icons.refresh),
              onPressed: _statsLoading ? null : _loadMeasurements,
            ),
          ],
        ),
        const SizedBox(height: 12),
        Card(
          elevation: 1,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(14),
            side: const BorderSide(color: Colors.black12),
          ),
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                const Text(
                  'Historia pomiarów (ostatnie 24h)',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
                ),
                const SizedBox(height: 12),
                // Zakładki
                SingleChildScrollView(
                  scrollDirection: Axis.horizontal,
                  child: Row(
                    children: List.generate(
                      statsLabels.length,
                      (i) => Padding(
                        padding: const EdgeInsets.only(right: 8),
                        child: FilterChip(
                          label: Text(statsLabels[i]),
                          selected: _selectedStatsTab == i,
                          onSelected: (selected) {
                            if (selected && !_statsLoading) {
                              setState(() => _selectedStatsTab = i);
                              _loadMeasurements();
                            }
                          },
                        ),
                      ),
                    ),
                  ),
                ),
                const SizedBox(height: 16),
                // Header
                const Row(
                  children: [
                    Expanded(
                      child: Text(
                        'DATA I CZAS',
                        style: TextStyle(
                          fontSize: 12,
                          fontWeight: FontWeight.w600,
                          color: Colors.black54,
                        ),
                      ),
                    ),
                    Expanded(
                      child: Align(
                        alignment: Alignment.centerRight,
                        child: Text(
                          'WARTOŚĆ',
                          style: TextStyle(
                            fontSize: 12,
                            fontWeight: FontWeight.w600,
                            color: Colors.black54,
                          ),
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 12),
                // Content
                if (_statsLoading)
                  const Padding(
                    padding: EdgeInsets.all(20),
                    child: CircularProgressIndicator(),
                  )
                else if (_statsData.isEmpty)
                  Text(
                    'Brak pomiarów w tym okresie.',
                    textAlign: TextAlign.center,
                    style: TextStyle(color: Colors.black45),
                  )
                else
                  ListView.separated(
                    shrinkWrap: true,
                    physics: const NeverScrollableScrollPhysics(),
                    itemCount: _statsData.length,
                    separatorBuilder: (_, __) => const Divider(height: 1),
                    itemBuilder: (_, i) {
                      final item = _statsData[i];
                      final timestamp = item['timestamp'] as String?;
                      final value = item['value']?.toString() ?? '-';
                      
                      String displayTime = '-';
                      if (timestamp != null) {
                        try {
                          final dt = DateTime.parse(timestamp);
                          displayTime = '${dt.day}.${dt.month} ${dt.hour}:${dt.minute.toString().padLeft(2, '0')}';
                        } catch (_) {}
                      }
                      
                      return Padding(
                        padding: const EdgeInsets.symmetric(vertical: 8),
                        child: Row(
                          children: [
                            Expanded(child: Text(displayTime)),
                            Expanded(
                              child: Align(
                                alignment: Alignment.centerRight,
                                child: Text(value),
                              ),
                            ),
                          ],
                        ),
                      );
                    },
                  ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _wateringTab() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        const Text(
          'Podlewanie',
          style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
        ),
        const SizedBox(height: 12),
        Card(
          elevation: 1,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(14),
            side: const BorderSide(color: Colors.black12),
          ),
          child: Padding(
            padding: const EdgeInsets.all(18.0),
            child: Column(
              children: [
                Icon(
                  Icons.water_drop,
                  size: 56,
                  color: _vacationMode ? Colors.blue : Colors.black38,
                ),
                const SizedBox(height: 8),
                const Text(
                  'Podlewanie ręczne',
                  style: TextStyle(fontSize: 20, fontWeight: FontWeight.w700),
                ),
                const SizedBox(height: 14),
                if (!_vacationMode)
                  Container(
                    width: double.infinity,
                    padding: const EdgeInsets.all(14),
                    decoration: BoxDecoration(
                      color: Colors.black12,
                      borderRadius: BorderRadius.circular(12),
                    ),
                    child: const Column(
                      children: [
                        Row(
                          mainAxisAlignment: MainAxisAlignment.center,
                          children: [
                            Icon(Icons.lock, size: 18),
                            SizedBox(width: 8),
                            Text(
                              'Funkcja niedostępna',
                              style: TextStyle(fontWeight: FontWeight.w700),
                            ),
                          ],
                        ),
                        SizedBox(height: 8),
                        Text(
                          'W trybie standardowym podlewanie odbywa się tylko po wrzuceniu monety.\n\nAby odblokować podlewanie ręczne, przejdź do zakładki Ustawienia i włącz Tryb wakacyjny.',
                          textAlign: TextAlign.center,
                        ),
                      ],
                    ),
                  )
                else
                  Column(
                    crossAxisAlignment: CrossAxisAlignment.stretch,
                    children: [
                      Row(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: const [
                          Icon(Icons.check_circle, color: Colors.green),
                          SizedBox(width: 8),
                          Expanded(
                            child: Text(
                              'Tryb wakacyjny jest aktywny. Możesz zdalnie podlać roślinę.',
                              softWrap: true,
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      Row(
                        children: [
                          Expanded(
                            child: TextField(
                              controller: _manualWaterSecondsController,
                              keyboardType: TextInputType.number,
                              decoration: const InputDecoration(
                                labelText: 'Czas podlewania',
                                suffixText: 'sek',
                                border: OutlineInputBorder(),
                              ),
                            ),
                          ),
                        ],
                      ),
                      const SizedBox(height: 12),
                      FilledButton(
                        onPressed: _isWatering ? null : _waterNow,
                        child: Padding(
                          padding: const EdgeInsets.symmetric(vertical: 12),
                          child: _isWatering
                              ? const SizedBox(
                                  width: 20,
                                  height: 20,
                                  child: CircularProgressIndicator(
                                    strokeWidth: 2,
                                  ),
                                )
                              : const Text('Podlej teraz'),
                        ),
                      ),
                    ],
                  ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _settingsTab() {
    return Column(
      crossAxisAlignment: CrossAxisAlignment.stretch,
      children: [
        const Text(
          'Ustawienia',
          style: TextStyle(fontSize: 18, fontWeight: FontWeight.w700),
        ),
        const SizedBox(height: 12),
        Card(
          elevation: 1,
          shape: RoundedRectangleBorder(
            borderRadius: BorderRadius.circular(14),
            side: const BorderSide(color: Colors.black12),
          ),
          child: Padding(
            padding: const EdgeInsets.all(16.0),
            child: Column(
              crossAxisAlignment: CrossAxisAlignment.stretch,
              children: [
                const Text(
                  'Interwał pomiarów',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
                ),
                const SizedBox(height: 8),
                TextField(
                  controller: _measurementIntervalSecondsController,
                  keyboardType: TextInputType.number,
                  decoration: const InputDecoration(
                    border: OutlineInputBorder(),
                    suffixText: 'sekund',
                  ),
                ),
                const SizedBox(height: 18),
                Row(
                  children: [
                    const Expanded(
                      child: Column(
                        crossAxisAlignment: CrossAxisAlignment.start,
                        children: [
                          Text(
                            'Tryb wakacyjny',
                            style: TextStyle(
                              fontSize: 16,
                              fontWeight: FontWeight.w600,
                            ),
                          ),
                          SizedBox(height: 4),
                          Text(
                            'Gdy włączony, umożliwia zdalne podlewanie bez monety.',
                            style: TextStyle(color: Colors.black54),
                          ),
                        ],
                      ),
                    ),
                    Switch(
                      value: _vacationModeDraft,
                      onChanged: (v) => setState(() => _vacationModeDraft = v),
                    ),
                  ],
                ),
                const SizedBox(height: 18),
                const Text(
                  'Progi wilgotności',
                  style: TextStyle(fontSize: 16, fontWeight: FontWeight.w600),
                ),
                const SizedBox(height: 6),
                const Text(
                  'Definiują strefę "za mokro".',
                  style: TextStyle(color: Colors.black54),
                ),
                const SizedBox(height: 12),
                Row(
                  children: [
                    Expanded(
                      child: TextField(
                        controller: _stopThresholdController,
                        keyboardType: TextInputType.number,
                        decoration: const InputDecoration(
                          labelText: 'Próg stop (max %)',
                          border: OutlineInputBorder(),
                          suffixText: '%',
                        ),
                      ),
                    ),
                  ],
                ),
                const SizedBox(height: 18),
                Align(
                  alignment: Alignment.centerRight,
                  child: FilledButton(
                    onPressed: _isSavingSettings ? null : _saveSettings,
                    child: Padding(
                      padding: const EdgeInsets.symmetric(vertical: 12),
                      child: _isSavingSettings
                          ? const SizedBox(
                              width: 20,
                              height: 20,
                              child: CircularProgressIndicator(
                                strokeWidth: 2,
                              ),
                            )
                          : const Text('Zapisz konfigurację'),
                    ),
                  ),
                ),
              ],
            ),
          ),
        ),
      ],
    );
  }

  Widget _tabBody() {
    switch (_selectedIndex) {
      case 0:
        return _statsTab();
      case 1:
        return _wateringTab();
      case 2:
        return _settingsTab();
      default:
        return const SizedBox.shrink();
    }
  }

  @override
  Widget build(BuildContext context) {
    return Scaffold(
      appBar: AppBar(
        title: Column(
          mainAxisSize: MainAxisSize.min,
          crossAxisAlignment: CrossAxisAlignment.start,
          children: [
            Text(
              _deviceName,
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
            ),
            Text(
              'MAC: $_deviceMac',
              maxLines: 1,
              overflow: TextOverflow.ellipsis,
              style: Theme.of(context).textTheme.bodySmall,
            ),
          ],
        ),
        actions: [
          IconButton(
            onPressed: _editDeviceName,
            icon: const Icon(Icons.edit),
            tooltip: 'Zmień nazwę',
          ),
        ],
        leading: IconButton(
          icon: const Icon(Icons.arrow_back),
          onPressed: () {
            Navigator.of(context).pop<String>(_deviceName);
          },
        ),
      ),
      bottomNavigationBar: BottomNavigationBar(
        currentIndex: _selectedIndex,
        onTap: (i) => setState(() => _selectedIndex = i),
        items: const [
          BottomNavigationBarItem(
            icon: Icon(Icons.query_stats),
            label: 'Statystyki',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.water_drop),
            label: 'Podlewanie',
          ),
          BottomNavigationBarItem(
            icon: Icon(Icons.settings),
            label: 'Ustawienia',
          ),
        ],
      ),
      body: SafeArea(
        child: Padding(
          padding: const EdgeInsets.all(16.0),
          child: Column(
            crossAxisAlignment: CrossAxisAlignment.stretch,
            children: [
              Expanded(
                child: SingleChildScrollView(
                  physics: const AlwaysScrollableScrollPhysics(),
                  child: _tabBody(),
                ),
              ),
            ],
          ),
        ),
      ),
    );
  }
}
