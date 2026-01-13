import { useState } from 'react';
import api from '../api/axiosConfig';

interface WateringViewProps {
    deviceMac: string;
    deviceId: number; 
}

const WateringView = ({ deviceMac, deviceId }: WateringViewProps) => {
    const [duration, setDuration] = useState<number>(1.0);
    const [isHolidayMode, setIsHolidayMode] = useState<boolean>(false);
    const [loading, setLoading] = useState<boolean>(false);

    const handleWaterClick = async () => {
        setLoading(true);
        try {
            await api.post(`/devices/${deviceId}/water`, {
                duration: duration
            });
            
            alert(`Wysłano komendę podlania na ${duration}s!`);
        } catch (error) {
            console.error("Błąd podlewania:", error);
            alert("Nie udało się wysłać komendy.");
        } finally {
            setLoading(false);
        }
    };

    const handleHolidayToggle = () => {
        const newState = !isHolidayMode;
        setIsHolidayMode(newState);
        console.log(`[MOCK] Tryb wakacyjny dla ${deviceMac}: ${newState ? 'WŁĄCZONY' : 'WYŁĄCZONY'}`);
    };

    return (
        <div className="row g-4">
            <div className="col-md-6">
                <div className="card shadow-sm border-0 h-100">
                    <div className="card-body p-4">
                        <div className="d-flex align-items-center gap-3 mb-4">
                            <div className="rounded-circle p-3 bg-light" style={{ color: '#6f42c1' }}>
                                <i className="bi bi-droplet-fill fs-4"></i>
                            </div>
                            <h5 className="mb-0 fw-bold">Podlewanie na żądanie</h5>
                        </div>

                        <p className="text-muted small">
                            Uruchom pompkę ręcznie. Pamiętaj, aby nie przelać rośliny!
                        </p>

                        <label className="form-label fw-bold text-muted small">Czas podlewania (sekundy)</label>
                        <div className="input-group mb-4">
                            <input 
                                type="number" 
                                className="form-control form-control-lg"
                                value={duration}
                                onChange={(e) => setDuration(parseFloat(e.target.value))}
                                min="0.5"
                                step="0.5"
                            />
                            <span className="input-group-text bg-light">sek</span>
                        </div>

                        <button 
                            className="btn w-100 btn-lg text-white"
                            style={{ backgroundColor: '#6f42c1' }}
                            onClick={handleWaterClick}
                            disabled={loading}
                        >
                            {loading ? (
                                <span><span className="spinner-border spinner-border-sm me-2"></span>Wysyłanie...</span>
                            ) : (
                                <span><i className="bi bi-water me-2"></i>Podlej teraz</span>
                            )}
                        </button>
                    </div>
                </div>
            </div>

            <div className="col-md-6">
                <div className="card shadow-sm border-0 h-100">
                    <div className="card-body p-4">
                        <div className="d-flex align-items-center gap-3 mb-4">
                            <div className="rounded-circle p-3 bg-light" style={{ color: '#6f42c1' }}>
                                <i className="bi bi-airplane-fill fs-4"></i>
                            </div>
                            <h5 className="mb-0 fw-bold">Tryb wakacyjny</h5>
                        </div>

                        <p className="text-muted small">
                            W tym trybie doniczka będzie automatycznie dbać o minimalną wilgotność, ignorując standardowy harmonogram.
                        </p>

                        <hr className="my-4 opacity-10" />

                        <div className="d-flex justify-content-between align-items-center">
                            <div>
                                <h6 className="mb-1">Aktywuj tryb</h6>
                                <small className={isHolidayMode ? "text-success fw-bold" : "text-muted"}>
                                    {isHolidayMode ? "Włączony" : "Wyłączony"}
                                </small>
                            </div>

                            <div className="form-check form-switch fs-3">
                                <input 
                                    className="form-check-input" 
                                    type="checkbox" 
                                    role="switch" 
                                    checked={isHolidayMode}
                                    onChange={handleHolidayToggle}
                                    style={{ 
                                        cursor: 'pointer', 
                                        backgroundColor: isHolidayMode ? '#6f42c1' : undefined,
                                        borderColor: isHolidayMode ? '#6f42c1' : undefined
                                    }}
                                />
                            </div>
                        </div>
                    </div>
                </div>
            </div>
        </div>
    );
};

export default WateringView;