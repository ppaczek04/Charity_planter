import { useState } from 'react';
import api from '../api/axiosConfig';

interface WateringViewProps {
    deviceMac: string;
    deviceId: number; 
    isHolidayMode: boolean;
}

const WateringView = ({ deviceId, isHolidayMode }: WateringViewProps) => {
    const [duration, setDuration] = useState<number>(1.0);
    const [loading, setLoading] = useState<boolean>(false);

    const handleWaterClick = async () => {
        setLoading(true);
        try {
            await api.post(`/devices/${deviceId}/water`, { duration: duration });
            
            alert(`Sukces! Doniczka podlana.`);
            
        } catch (error: any) {
            console.error("Błąd:", error);
            
            if (error.response && error.response.status === 504) {
                 alert("Błąd: Urządzenie nie odpowiada! Sprawdź, czy doniczka jest podłączona do prądu.");
            } else {
                 alert("Nie udało się wysłać komendy.");
            }
        } finally {
            setLoading(false);
        }
    };

    return (
        <div className="row justify-content-center">
            <div className="col-md-8">
                <div className="card shadow-sm border-0">
                    <div className="card-body p-5 text-center">
                        
                        <div className="mb-4">
                            <i className={`bi bi-droplet-fill fs-1 ${isHolidayMode ? 'text-primary' : 'text-muted'}`}></i>
                            <h3 className="fw-bold mt-3">Podlewanie ręczne</h3>
                        </div>

                        {isHolidayMode ? (
                            <>
                                <p className="text-success mb-4">
                                    <i className="bi bi-check-circle-fill me-2"></i>
                                    Tryb wakacyjny jest aktywny. Możesz zdalnie podlać roślinę.
                                </p>

                                <div className="input-group mb-4 mx-auto" style={{ maxWidth: '300px' }}>
                                    <input 
                                        type="number" className="form-control form-control-lg"
                                        value={duration} onChange={(e) => setDuration(parseFloat(e.target.value))}
                                        min="0.5" step="0.5"
                                    />
                                    <span className="input-group-text">sek</span>
                                </div>

                                <button 
                                    className="btn btn-primary btn-lg w-100" 
                                    onClick={handleWaterClick} 
                                    disabled={loading}
                                >
                                    {loading ? 'Wysyłanie...' : 'Podlej teraz'}
                                </button>
                            </>
                        ) : (
                            <div className="alert alert-secondary mt-3">
                                <h5 className="alert-heading"><i className="bi bi-lock-fill"></i> Funkcja niedostępna</h5>
                                <p className="mb-0 mt-2">
                                    W trybie standardowym podlewanie odbywa się tylko po wrzuceniu monety.
                                </p>
                                <hr />
                                <p className="mb-0 small">
                                    Aby odblokować podlewanie ręczne, przejdź do zakładki <strong>Ustawienia</strong> i włącz <strong>Tryb Wakacyjny</strong>.
                                </p>
                            </div>
                        )}
                    </div>
                </div>
            </div>
        </div>
    );
};

export default WateringView;