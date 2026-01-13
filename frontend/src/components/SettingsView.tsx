import { useState } from 'react';
import api from '../api/axiosConfig';

interface SettingsViewProps {
    deviceMac: string;
    deviceId: number;
}

const SettingsView = ({ deviceId }: SettingsViewProps) => {
    const [interval, setInterval] = useState<number>(5);

    const [soilMin, setSoilMin] = useState<number>(30);
    const [soilMax, setSoilMax] = useState<number>(70);

    const [loading, setLoading] = useState<boolean>(false);

    const handleSave = async () => {
        setLoading(true);
        
        try {
            await api.put(`/devices/${deviceId}/settings`, {
                interval: interval
            });

            alert(`Ustawienia zostały wysłane do urządzenia!`);
        } catch (error) {
            console.error(error);
            alert("Błąd podczas zapisywania ustawień.");
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="row g-4">
            
            <div className="col-12">
                <div className="card shadow-sm border-0">
                    <div className="card-header bg-white border-bottom-0 pt-4 px-4 pb-0">
                        <div className="d-flex align-items-center gap-3">
                            <div className="rounded-circle p-2 bg-light" style={{ color: '#6f42c1' }}>
                                <i className="bi bi-clock-history fs-4"></i>
                            </div>
                            <h5 className="mb-0 fw-bold">Częstotliwość pomiarów</h5>
                        </div>
                    </div>
                    <div className="card-body px-4 pb-4">
                        <p className="text-muted small mb-3">
                            Określ, co ile sekund urządzenie ma wybudzać się i wysyłać aktualne dane do serwera.
                        </p>
                        <div className="input-group" style={{ maxWidth: '300px' }}>
                            <span className="input-group-text bg-light border-end-0">
                                <i className="bi bi-stopwatch"></i>
                            </span>
                            <input 
                                type="number" 
                                className="form-control border-start-0 ps-0"
                                value={interval}
                                onChange={(e) => setInterval(parseInt(e.target.value) || 0)}
                                min="5"
                            />
                            <span className="input-group-text bg-light">sekund</span>
                        </div>
                    </div>
                </div>
            </div>

            <div className="col-12">
                <div className="card shadow-sm border-0">
                    <div className="card-header bg-white border-bottom-0 pt-4 px-4 pb-0">
                        <div className="d-flex align-items-center gap-3">
                            <div className="rounded-circle p-2 bg-light" style={{ color: '#6f42c1' }}>
                                <i className="bi bi-moisture fs-4"></i>
                            </div>
                            <h5 className="mb-0 fw-bold">Wilgotność gleby i automatyzacja</h5>
                        </div>
                    </div>
                    <div className="card-body px-4 pb-4">
                        <p className="text-muted small mb-4">
                            Skonfiguruj progi wilgotności, aby system wiedział, kiedy Twoja roślina potrzebuje wody.
                        </p>

                        <div className="row g-4">
                            <div className="col-md-6">
                                <div className="p-3 border rounded bg-light bg-opacity-25 h-100 position-relative">
                                    <div className="d-flex align-items-center gap-2 mb-3" style={{ color: '#dc3545' }}>
                                        <i className="bi bi-droplet-half fs-5"></i>
                                        <span className="fw-bold text-dark">Próg minimalny (Susza)</span>
                                    </div>
                                    
                                    <label className="form-label text-muted small mb-1">
                                        Jeśli wilgotność spadnie <strong>poniżej</strong>:
                                    </label>
                                    <div className="input-group">
                                        <input 
                                            type="number" 
                                            className="form-control"
                                            value={soilMin}
                                            onChange={(e) => setSoilMin(parseFloat(e.target.value) || 0)}
                                            min="0" max="100"
                                        />
                                        <span className="input-group-text">%</span>
                                    </div>
                                    <div className="form-text text-danger small mt-2">
                                        <i className="bi bi-arrow-return-right me-1"></i>
                                        System uruchomi podlewanie.
                                    </div>
                                </div>
                            </div>

                            <div className="col-md-6">
                                <div className="p-3 border rounded bg-light bg-opacity-25 h-100">
                                    <div className="d-flex align-items-center gap-2 mb-3" style={{ color: '#198754' }}>
                                        <span className="fw-bold text-dark">Próg maksymalny</span>
                                    </div>

                                    <label className="form-label text-muted small mb-1">
                                        Jeśli wilgotność jest <strong>powyżej</strong>:
                                    </label>
                                    <div className="input-group">
                                        <input 
                                            type="number" 
                                            className="form-control"
                                            value={soilMax}
                                            onChange={(e) => setSoilMax(parseFloat(e.target.value) || 0)}
                                            min="0" max="100"
                                        />
                                        <span className="input-group-text">%</span>
                                    </div>
                                    <div className="form-text text-success small mt-2">
                                        <i className="bi bi-check-circle me-1"></i>
                                        Gleba jest wilgotna, system nie będzie podlewać.
                                    </div>
                                </div>
                            </div>
                        </div>

                        <div className="mt-4">
                            <label className="text-muted small mb-2 d-block">Wizualizacja zakresu idealnego:</label>
                            <div className="progress" style={{ height: '10px' }}>
                                <div 
                                    className="progress-bar bg-danger bg-opacity-50" 
                                    role="progressbar" 
                                    style={{ width: `${soilMin}%` }}
                                ></div>
                                <div 
                                    className="progress-bar bg-success" 
                                    role="progressbar" 
                                    style={{ width: `${soilMax - soilMin}%` }}
                                ></div>
                                <div 
                                    className="progress-bar bg-info bg-opacity-25" 
                                    role="progressbar" 
                                    style={{ width: `${100 - soilMax}%` }}
                                ></div>
                            </div>
                            <div className="d-flex justify-content-between text-muted" style={{ fontSize: '0.75rem', marginTop: '4px' }}>
                                <span>0% (Za sucho)</span>
                                <span>100% (Za mokro)</span>
                            </div>
                        </div>

                    </div>
                </div>
            </div>

            <div className="col-12 text-end">
                <button 
                    className="btn btn-lg text-white px-5 shadow-sm"
                    style={{ backgroundColor: '#6f42c1' }}
                    onClick={handleSave}
                    disabled={loading}
                >
                    {loading ? (
                        <span><span className="spinner-border spinner-border-sm me-2"></span>Zapisywanie...</span>
                    ) : (
                        <span>Zapisz konfigurację</span>
                    )}
                </button>
            </div>
        </div>
    );
};

export default SettingsView;