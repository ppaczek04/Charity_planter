import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import { tabTranslations, type Device, type User } from '../interfaces';
import Navbar from '../components/Navbar';
import DeviceHeader from '../components/DeviceHeader';
import DeviceSidebar from '../components/DeviceSidebar';
import WateringView from '../components/WateringView';
import SettingsView from '../components/SettingsView';
import MeasurementsView from '../components/MeasurementsView';
import api from '../api/axiosConfig';

function Dashboard() {
    const [user, setUser] = useState<User | null>(null);
    const [activeTab, setActiveTab] = useState('measurements');
    
    const [activeDevices, setActiveDevices] = useState<Device[]>([]);
    const [archivedDevices, setArchivedDevices] = useState<Device[]>([]);
    
    const [selectedDevice, setSelectedDevice] = useState<Device | null>(null);
    const navigate = useNavigate();

    useEffect(() => {
        const loggedInUser = localStorage.getItem('user');
        if (!loggedInUser) {
            navigate('/login');
        } else {
            setUser(JSON.parse(loggedInUser));
        }
    }, [navigate]);

    useEffect(() => {
        if (user?.id) {
            api.get<Device[]>(`/devices/user/${user.id}`)
                .then(res => {
                    setActiveDevices(res.data);
                    if (res.data.length > 0 && !selectedDevice) {
                        setSelectedDevice(res.data[0]);
                    }
                })
                .catch(err => console.error("Błąd pobierania aktywnych:", err));

            api.get<Device[]>(`/devices/user/${user.id}/archived`)
                .then(res => setArchivedDevices(res.data))
                .catch(err => console.error("Błąd pobierania archiwalnych:", err));
        }
    }, [user]);

    const handleLogout = () => {
        localStorage.removeItem('user');
        navigate('/login');
    };

    const handleRename = async (deviceId: number, newName: string) => {
        try {
            await api.put(`/devices/${deviceId}`, { newName });

            setActiveDevices(prevDevices => 
                prevDevices.map(d => 
                    d.id === deviceId ? { ...d, name: newName } : d
                )
            );

            if (selectedDevice && selectedDevice.id === deviceId) {
                setSelectedDevice(prev => prev ? { ...prev, name: newName } : null);
            }

        } catch (error) {
            console.error("Błąd zmiany nazwy:", error);
            alert("Nie udało się zmienić nazwy urządzenia.");
        }
    };

    if (!user) return <div className="text-center mt-5">Ładowanie...</div>;

    return (
        <div style={{ minHeight: '100vh', backgroundColor: '#f8f9fa' }}>
            <Navbar username={user.username} onLogout={handleLogout} />

            <div className="container py-4">
                <h2 className="mb-4" style={{ color: '#333' }}>Twoje Urządzenia</h2>

                {activeDevices.length === 0 && archivedDevices.length === 0 ? (
                    <div className="alert alert-info">
                        Nie masz jeszcze żadnych doniczek. <br/>
                        Kliknij "Dodaj doniczkę" w menu, aby skonfigurować nową.
                    </div>
                ) : (
                    <div className="row mb-4">
                        {activeDevices.map((dev) => (
                            <div key={dev.id} className="col-12 mb-3">
                                <div 
                                    onClick={() => {
                                        setSelectedDevice(dev);
                                    }}
                                    style={{ 
                                        cursor: 'pointer',
                                        transform: selectedDevice?.mac === dev.mac ? 'scale(1.01)' : 'scale(1)',
                                        transition: 'all 0.2s',
                                        border: selectedDevice?.mac === dev.mac ? '2px solid #6f42c1' : '2px solid transparent',
                                        borderRadius: '8px'
                                    }}
                                >
                                    <DeviceHeader 
                                        id={dev.id}
                                        name={dev.name || "Bez nazwy"} 
                                        mac={dev.mac}
                                        onRename={handleRename}
                                        isArchived={dev.isArchived}
                                    />
                                </div>
                            </div>
                        ))}
                    </div>
                )}

                {archivedDevices.length > 0 && (
                    <div className="mb-4">
                        <h5 className="text-muted border-bottom pb-2 mb-3">Historia (Urządzenia sprzedane/przekazane)</h5>
                        <div className="row opacity-75">
                            {archivedDevices.map((dev) => (
                                <div key={dev.mac} className="col-12 mb-3">
                                    <div 
                                        onClick={() => {
                                            setSelectedDevice({ ...dev, isArchived: true });
                                            setActiveTab('measurements');
                                        }}
                                        style={{ 
                                            cursor: 'pointer',
                                            transform: selectedDevice?.mac === dev.mac ? 'scale(1.01)' : 'scale(1)',
                                            transition: 'all 0.2s',
                                            border: selectedDevice?.mac === dev.mac ? '2px solid #6c757d' : '2px solid transparent',
                                            borderRadius: '8px',
                                            filter: 'grayscale(100%)'
                                        }}
                                    >
                                        <DeviceHeader 
                                            id={0} 
                                            name={dev.name || "Urządzenie archiwalne"} 
                                            mac={dev.mac}
                                            onRename={() => {}} 
                                            isArchived={dev.isArchived}
                                        />
                                    </div>
                                </div>
                            ))}
                        </div>
                    </div>
                )}

                {selectedDevice && (
                    <div className="card shadow-sm border-0 fade-in">
                        <div className="row g-0">
                            <div className="col-md-3">
                                <DeviceSidebar 
                                    deviceMac={selectedDevice.mac} 
                                    activeTab={activeTab} 
                                    setActiveTab={setActiveTab}
                                    isArchived={selectedDevice.isArchived}
                                    deviceName={selectedDevice.name}
                                />
                            </div>

                            <div className="col-md-9 p-4 bg-white" style={{ borderTopRightRadius: '0.375rem', borderBottomRightRadius: '0.375rem' }}>
                                <div className="d-flex justify-content-between border-bottom pb-2 mb-4">
                                    <h4 className="mb-0 fw-bold" style={{ color: selectedDevice.isArchived ? '#6c757d' : '#6f42c1' }}>
                                        {tabTranslations[activeTab] || activeTab}
                                    </h4>
                                    <span className={`badge ${selectedDevice.isArchived ? 'bg-secondary' : 'bg-light text-dark'} align-self-center`}>
                                        MAC: {selectedDevice.mac}
                                        {selectedDevice.isArchived && " (Archiwum)"}
                                    </span>
                                </div>

                                {selectedDevice.isArchived && (
                                    <div className="alert alert-secondary mb-4">
                                        <i className="bi bi-archive-fill me-2"></i>
                                        To urządzenie nie jest już przypisane do Twojego konta. Przeglądasz historię pomiarów z okresu, gdy byłeś właścicielem.
                                    </div>
                                )}

                                {activeTab === 'measurements' && (
                                    <MeasurementsView 
                                        deviceMac={selectedDevice.mac} 
                                        ownerId={String(user.id)}
                                    />
                                )}

                                {!selectedDevice.isArchived && activeTab === 'watering' && (
                                    <WateringView 
                                        deviceMac={selectedDevice.mac}
                                        deviceId={selectedDevice.id}
                                    />
                                )}

                                {!selectedDevice.isArchived && activeTab === 'settings' && (
                                    <SettingsView deviceMac={selectedDevice.mac} deviceId={selectedDevice.id}/>
                                )}
                            </div>
                        </div>
                    </div>
                )}
            </div>
            
            <style>{`
                .fade-in { animation: fadeIn 0.5s; }
                @keyframes fadeIn { from { opacity: 0; } to { opacity: 1; } }
            `}</style>
        </div>
    );
}

export default Dashboard;