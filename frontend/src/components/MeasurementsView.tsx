import { useEffect, useState } from 'react';
import api from '../api/axiosConfig';

interface Measurement {
    id: number;
    value: number;
    timestamp: string;
    deviceMac: string;
    ownerId: string;
}

type SensorType = 'temperature' | 'pressure' | 'soil' | 'coin';

interface MeasurementsViewProps {
    deviceMac: string;
    ownerId: string;
}

const MeasurementsView = ({ deviceMac, ownerId }: MeasurementsViewProps) => {
    const [activeSensor, setActiveSensor] = useState<SensorType>('temperature');
    const [data, setData] = useState<Measurement[]>([]);
    const [loading, setLoading] = useState<boolean>(false);
    const [error, setError] = useState<string>('');

    const fetchData = async () => {
        if (!deviceMac || !ownerId) return;

        setLoading(true);
        setError('');
        try {
            let endpoint = '';
            switch (activeSensor) {
                case 'temperature': endpoint = '/temperatures'; break;
                case 'pressure': endpoint = '/pressures'; break;
                case 'soil': endpoint = '/soil-measurements'; break;
                case 'coin': endpoint = '/coin-events'; break;
            }

            const date = new Date();
            date.setDate(date.getDate() - 1);
            const fromDate = date.toISOString();

            const response = await api.get<Measurement[]>(endpoint, {
                params: {
                    deviceMac: deviceMac,
                    ownerId: ownerId,
                    from: fromDate
                }
            });
            setData(response.data);
        } catch (err: any) {
            console.error(err);
            setError('Błąd pobierania danych.');
        } finally {
            setLoading(false);
        }
    };

    useEffect(() => {
        fetchData();
    }, [activeSensor, deviceMac]);

    const getSensorMeta = () => {
        switch (activeSensor) {
            case 'temperature': return { label: 'Temperatura', unit: '°C', icon: 'bi-thermometer-half' };
            case 'pressure': return { label: 'Ciśnienie', unit: 'hPa', icon: 'bi-speedometer' };
            case 'soil': return { label: 'Wilgotność gleby', unit: '%', icon: 'bi-moisture' };
            case 'coin': return { label: 'Wrzucone monety', unit: 'PLN', icon: 'bi-coin' };
        }
    };

    const meta = getSensorMeta();

    return (
        <div className="card shadow-sm border-0">
            <div className="card-header bg-white border-bottom-0 pt-4 px-4">
                <div className="d-flex align-items-center gap-3 mb-3">
                    <div className="rounded-circle p-2 bg-light" style={{ color: '#6f42c1' }}>
                        <i className="bi bi-graph-up fs-4"></i>
                    </div>
                    <h5 className="mb-0 fw-bold">Historia pomiarów (Ostatnie 24h)</h5>
                </div>

                <div className="d-flex gap-2 flex-wrap">
                    {[
                        { id: 'temperature', label: 'Temp' },
                        { id: 'pressure', label: 'Ciśnienie' },
                        { id: 'soil', label: 'Gleba' },
                        { id: 'coin', label: 'Monety' }
                    ].map((sensor) => (
                        <button
                            key={sensor.id}
                            onClick={() => setActiveSensor(sensor.id as SensorType)}
                            className={`btn btn-sm rounded-pill px-3 ${activeSensor === sensor.id ? 'text-white' : 'btn-light text-muted'}`}
                            style={{ 
                                backgroundColor: activeSensor === sensor.id ? '#6f42c1' : undefined,
                                borderColor: activeSensor === sensor.id ? '#6f42c1' : undefined
                            }}
                        >
                            {sensor.label}
                        </button>
                    ))}
                    
                    <button onClick={fetchData} className="btn btn-sm btn-outline-secondary ms-auto" title="Odśwież">
                        <i className="bi bi-arrow-clockwise"></i>
                    </button>
                </div>
            </div>

            <div className="card-body p-0">
                {loading ? (
                    <div className="text-center py-5">
                        <div className="spinner-border text-primary" role="status" style={{ color: '#6f42c1' }}></div>
                        <p className="mt-2 text-muted small">Ładowanie danych...</p>
                    </div>
                ) : error ? (
                    <div className="alert alert-danger m-4">{error}</div>
                ) : (
                    <div className="table-responsive" style={{ maxHeight: '400px', overflowY: 'auto' }}>
                        <table className="table table-hover align-middle mb-0">
                            <thead className="bg-light sticky-top" style={{ zIndex: 1 }}>
                                <tr>
                                    <th scope="col" className="ps-4 py-3 text-muted small text-uppercase">Data i czas</th>
                                    <th scope="col" className="py-3 text-muted small text-uppercase text-end pe-4">Wartość</th>
                                </tr>
                            </thead>
                            <tbody>
                                {data.length === 0 ? (
                                    <tr>
                                        <td colSpan={2} className="text-center py-5 text-muted">
                                            Brak pomiarów w tym okresie.
                                        </td>
                                    </tr>
                                ) : (
                                    data.map((item) => (
                                        <tr key={item.id}>
                                            <td className="ps-4">
                                                <div className="fw-bold text-dark">
                                                    {new Date(item.timestamp).toLocaleTimeString([], {hour: '2-digit', minute:'2-digit', second:'2-digit'})}
                                                </div>
                                                <div className="text-muted small">
                                                    {new Date(item.timestamp).toLocaleDateString()}
                                                </div>
                                            </td>
                                            <td className="text-end pe-4">
                                                <span className="badge bg-light text-dark border px-3 py-2 rounded-pill fs-6">
                                                    <i className={`bi ${meta.icon} me-2`} style={{ color: '#6f42c1' }}></i>
                                                    {item.value.toFixed(2)} {meta.unit}
                                                </span>
                                            </td>
                                        </tr>
                                    ))
                                )}
                            </tbody>
                        </table>
                    </div>
                )}
            </div>
        </div>
    );
};

export default MeasurementsView;