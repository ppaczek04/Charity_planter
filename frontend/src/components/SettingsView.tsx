import { useEffect, useState } from 'react';
import api from '../api/axiosConfig';

interface SettingsViewProps {
    deviceMac: string;
    deviceId: number;
    initialSettings: {
        interval: number;
        holidayMode: boolean;
        soilMin: number;
        soilMax: number;
    };
    onSettingsSaved: (updatedDevice: any) => void;
}

const SettingsView = ({ deviceId, initialSettings, onSettingsSaved }: SettingsViewProps) => {
    const [interval, setInterval] = useState(initialSettings.interval);
    const [isHolidayMode, setIsHolidayMode] = useState(initialSettings.holidayMode);
    const [soilMin, setSoilMin] = useState(initialSettings.soilMin);
    const [soilMax, setSoilMax] = useState(initialSettings.soilMax);
    const [loading, setLoading] = useState(false);

    useEffect(() => {
        setInterval(initialSettings.interval);
        setIsHolidayMode(initialSettings.holidayMode);
        setSoilMin(initialSettings.soilMin);
        setSoilMax(initialSettings.soilMax);
    }, [initialSettings]);
    
    const handleSave = async () => {
        setLoading(true);
        if (soilMin >= soilMax) {
            alert("Próg minimalny musi być mniejszy od maksymalnego!");
            setLoading(false);
            return;
        }

        try {
            const response = await api.put(`/devices/${deviceId}/settings`, {
                interval: interval,
                holidayMode: isHolidayMode,
                soilMin: soilMin,
                soilMax: soilMax
            });

            alert(`Konfiguracja zapisana!`);
            onSettingsSaved(response.data); 
        } catch (error) {
            console.error(error);
            alert("Błąd zapisu ustawień.");
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="row g-4">
            <div className="col-12">
                <div className="card shadow-sm border-0">
                    <div className="card-body px-4 py-4">
                        <div className="d-flex align-items-center gap-3 mb-3">
                            <i className="bi bi-clock-history fs-4 text-primary"></i>
                            <h5 className="mb-0 fw-bold">Interwał pomiarów</h5>
                        </div>
                        <div className="input-group" style={{ maxWidth: '300px' }}>
                            <input 
                                type="number" className="form-control" value={interval}
                                onChange={(e) => setInterval(parseInt(e.target.value) || 5)} min="5"
                            />
                            <span className="input-group-text">sekund</span>
                        </div>
                    </div>
                </div>
            </div>

            <div className="col-12">
                <div className="card shadow-sm border-0">
                    <div className="card-body px-4 py-4">
                        <div className="d-flex justify-content-between align-items-center">
                            <div className="d-flex align-items-center gap-3">
                                <i className="bi bi-airplane-fill fs-4 text-success"></i>
                                <div>
                                    <h5 className="mb-0 fw-bold">Tryb wakacyjny</h5>
                                    <small className="text-muted">Gdy włączony, umożliwia zdalne podlewanie bez monety.</small>
                                </div>
                            </div>
                            <div className="form-check form-switch fs-3">
                                <input 
                                    className="form-check-input" type="checkbox" role="switch"
                                    checked={isHolidayMode}
                                    onChange={() => setIsHolidayMode(!isHolidayMode)}
                                    style={{ cursor: 'pointer' }}
                                />
                            </div>
                        </div>

                        <hr className="my-4 opacity-10" />

                        <div className="d-flex align-items-center gap-3 mb-3">
                             <i className="bi bi-moisture fs-4" style={{ color: '#fd7e14' }}></i>
                             <div>
                                <h5 className="mb-0 fw-bold">Progi wilgotności</h5>
                                <small className="text-muted">Definiują strefy "za sucho" i "za mokro".</small>
                             </div>
                        </div>

                        <div className="mt-3">
                            <div className="row g-3">
                                <div className="col-md-6">
                                    <label className="form-label small text-muted fw-bold">Próg Suszy (Min %)</label>
                                    <div className="input-group">
                                        <input 
                                            type="number" className="form-control" value={soilMin}
                                            onChange={(e) => setSoilMin(parseInt(e.target.value))} 
                                        />
                                        <span className="input-group-text">%</span>
                                    </div>
                                    <div className="form-text small">Poniżej tej wartości system pozwoli podlać.</div>
                                </div>
                                <div className="col-md-6">
                                    <label className="form-label small text-muted fw-bold">Próg Stop (Max %)</label>
                                    <div className="input-group">
                                        <input 
                                            type="number" className="form-control" value={soilMax}
                                            onChange={(e) => setSoilMax(parseInt(e.target.value))} 
                                        />
                                        <span className="input-group-text">%</span>
                                    </div>
                                    <div className="form-text small">Powyżej tej wartości system zablokuje podlewanie.</div>
                                </div>
                            </div>
                        </div>
                    </div>
                </div>
            </div>

            <div className="col-12 text-end">
                <button className="btn btn-primary btn-lg px-5 shadow-sm" onClick={handleSave} disabled={loading}>
                    {loading ? (
                        <span><span className="spinner-border spinner-border-sm me-2"></span>Zapisywanie...</span>
                    ) : (
                        'Zapisz konfigurację'
                    )}
                </button>
            </div>
        </div>
    );
};

export default SettingsView;