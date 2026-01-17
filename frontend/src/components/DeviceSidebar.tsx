interface DeviceSidebarProps {
    deviceMac: string;
    activeTab: string;
    setActiveTab: (tab: string) => void;
    isArchived?: boolean;
    deviceName?: string;
}

const DeviceSidebar = ({ deviceMac, activeTab, setActiveTab, isArchived, deviceName }: DeviceSidebarProps) => {
    const menuItems = [
        { id: 'info', label: 'Urządzenie: ESP32', icon: '' },
        { id: 'measurements', label: 'Statystyki', icon: 'bi-graph-up' }, 
        ...(isArchived ? [] : [
            { id: 'watering', label: 'Podlewanie', icon: 'bi-droplet' },
            { id: 'settings', label: 'Ustawienia', icon: 'bi-gear' }
        ])
    ];

    return (
        <div className="border-end h-100 p-3">
            <div className="mb-4">
                <strong>Nazwa urządzenia: {deviceName ?? `Doniczka ${deviceMac}`}</strong>
                <div className="text-muted small mt-1">Adres MAC: {deviceMac}</div>
            </div>
            
            <ul className="nav flex-column gap-2">
                {menuItems.slice(1).map((item) => (
                    <li key={item.id} className="nav-item">
                        <button 
                            className={`nav-link d-flex align-items-center gap-2 w-100 text-start ${activeTab === item.id ? 'active fw-bold' : 'text-dark'}`}
                            onClick={() => setActiveTab(item.id)}
                            style={{ 
                                background: 'none', 
                                border: 'none',
                                color: activeTab === item.id ? '#6f42c1' : 'inherit'
                            }}
                        >
                            <i className={`bi ${item.icon}`}></i>
                            {item.label}
                        </button>
                    </li>
                ))}
            </ul>
        </div>
    );
};

export default DeviceSidebar;