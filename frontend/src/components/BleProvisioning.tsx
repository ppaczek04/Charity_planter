import { useState } from 'react';
import { useNavigate } from 'react-router-dom';
import api from '../api/axiosConfig';

const SERVICE_UUID = 0x00FF;
const CHAR_SSID_UUID = 0xFF01;
const CHAR_PASS_UUID = 0xFF02;
const CHAR_URL_UUID = 0xFF03;
const CHAR_MAC_UUID = 0xFF06;
const CHAR_OWNER_UUID = 0xFF07;

const BleProvisioning = () => {
    const navigate = useNavigate();
    
    const [device, setDevice] = useState<BluetoothDevice | null>(null);
    const [isConnected, setIsConnected] = useState(false);
    const [status, setStatus] = useState("");
    
    const [ssid, setSsid] = useState("");
    const [password, setPassword] = useState("");
    const [mqttUrl, setMqttUrl] = useState("mqtt://"); 

    const connectToDevice = async () => {
        try {
            setStatus("Szukam urządzenia...");
            
            const device = await navigator.bluetooth.requestDevice({
                acceptAllDevices: true,
                optionalServices: [SERVICE_UUID]
            });

            setDevice(device);
            setStatus("Łączenie z GATT...");

            const server = await device.gatt?.connect();
            
            if (server) {
                setIsConnected(true);
                setStatus("Połączono! Wpisz dane WiFi i MQTT.");
            }
        } catch (error: any) {
            console.error(error);
            setStatus("Błąd połączenia: " + error.message);
        }
    };

    const sendConfigAndRegister = async () => {
        if (!device || !device.gatt?.connected) {
            setStatus("Urządzenie rozłączone. Połącz ponownie.");
            return;
        }

        try {
            const server = await device.gatt.connect();
            const service = await server.getPrimaryService(SERVICE_UUID);

            setStatus("Pobieranie MAC adresu urządzenia...");
            const macChar = await service.getCharacteristic(CHAR_MAC_UUID);
            const macValue = await macChar.readValue();
            const decoder = new TextDecoder("utf-8");
            const deviceMac = decoder.decode(macValue).replace(/\0/g, '').trim();
            console.log("Odczytano MAC:", deviceMac);

            setStatus("Wysyłanie konfiguracji WiFi...");
            const encoder = new TextEncoder();

            const ssidChar = await service.getCharacteristic(CHAR_SSID_UUID);
            await ssidChar.writeValue(encoder.encode(ssid));

            const passChar = await service.getCharacteristic(CHAR_PASS_UUID);
            await passChar.writeValue(encoder.encode(password));

            const urlChar = await service.getCharacteristic(CHAR_URL_UUID);
            await urlChar.writeValue(encoder.encode(mqttUrl));

            setStatus("Rejestrowanie urządzenia w chmurze...");
            
            const userStr = localStorage.getItem('user');
            const user = userStr ? JSON.parse(userStr) : null;

            if (user && user.id) {
                const ownerIdStr = String(user.id);
                const ownerChar = await service.getCharacteristic(CHAR_OWNER_UUID);
                await ownerChar.writeValue(encoder.encode(ownerIdStr));
            } else {
                throw new Error("Nie znaleziono danych użytkownika. Zaloguj się ponownie.");
            }

            if (user) {
                await api.post('/devices/claim', {
                    deviceMac: deviceMac,
                    userId: user.id
                });
                
                setStatus("SUKCES! Urządzenie skonfigurowane i dodane.");
                
                setTimeout(() => {
                    if (device.gatt?.connected) device.gatt.disconnect();
                    navigate('/dashboard');
                }, 2000);
            } else {
                setStatus("Błąd: Nie jesteś zalogowany w aplikacji webowej.");
            }

        } catch (e: any) {
            console.error(e);
            setStatus("Błąd procesu: " + e.message);
        }
    };

    return (
        <div className="card shadow-sm border-0">
            <div className="card-body p-4">
                <div className="d-flex align-items-center gap-3 mb-4">
                    <div className="rounded-circle p-3 bg-light" style={{ color: '#6f42c1' }}>
                        <i className="bi bi-bluetooth fs-4"></i>
                    </div>
                    <h5 className="mb-0 fw-bold">Dodaj nowe urządzenie</h5>
                </div>

                {!isConnected ? (
                    <div className="text-center">
                        <p className="text-muted mb-4 small">
                            Upewnij się, że Bluetooth w komputerze jest włączony, a doniczka jest podłączona do prądu.
                        </p>
                        <button 
                            className="btn w-100 py-2 text-white"
                            style={{ backgroundColor: '#6f42c1' }}
                            onClick={connectToDevice}
                        >
                            <i className="bi bi-search me-2"></i> Szukaj doniczki
                        </button>
                    </div>
                ) : (
                    <div>
                        <div className="alert alert-success py-2 text-center small mb-3">
                            <i className="bi bi-check-circle me-2"></i>Połączono z ESP32
                        </div>

                        <div className="mb-3">
                            <label className="form-label small text-muted">Nazwa sieci WiFi (SSID)</label>
                            <input 
                                className="form-control" 
                                value={ssid} 
                                onChange={e => setSsid(e.target.value)} 
                                placeholder="np. Domowe_WiFi"
                            />
                        </div>
                        <div className="mb-3">
                            <label className="form-label small text-muted">Hasło WiFi</label>
                            <input 
                                type="password" 
                                className="form-control" 
                                value={password} 
                                onChange={e => setPassword(e.target.value)} 
                            />
                        </div>
                        <div className="mb-4">
                            <label className="form-label small text-muted">Adres Brokera MQTT</label>
                            <input 
                                className="form-control" 
                                placeholder="mqtt://..."
                                value={mqttUrl} 
                                onChange={e => setMqttUrl(e.target.value)} 
                            />
                        </div>

                        <button 
                            className="btn w-100 text-white" 
                            style={{ backgroundColor: '#6f42c1' }}
                            onClick={sendConfigAndRegister}
                        >
                            Zapisz i Dodaj
                        </button>
                    </div>
                )}

                {status && (
                    <div className="mt-3 text-center small text-muted border-top pt-2">
                        Status: {status}
                    </div>
                )}
            </div>
        </div>
    );
};

export default BleProvisioning;