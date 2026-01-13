import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import Navbar from '../components/Navbar';
import BleProvisioning from '../components/BleProvisioning';
import type { User } from '../interfaces';

function AddDevicePage() {
    const [user, setUser] = useState<User | null>(null);
    const navigate = useNavigate();

    useEffect(() => {
        const loggedInUser = localStorage.getItem('user');
        if (!loggedInUser) {
            navigate('/login');
        } else {
            setUser(JSON.parse(loggedInUser));
        }
    }, [navigate]);

    const handleLogout = () => {
        localStorage.removeItem('user');
        navigate('/login');
    };

    if (!user) return null;

    return (
        <div style={{ minHeight: '100vh', backgroundColor: '#f8f9fa' }}>
            <Navbar username={user.username} onLogout={handleLogout} />

            <div className="container py-5">
                <div className="row justify-content-center">
                    <div className="col-md-8 col-lg-6">
                        <div className="mb-4">
                            <button 
                                onClick={() => navigate('/dashboard')} 
                                className="btn btn-link text-decoration-none ps-0"
                                style={{ color: '#6f42c1' }}
                            >
                                <i className="bi bi-arrow-left me-2"></i>Wróć do Panelu
                            </button>
                            <h2 className="fw-bold mt-2">Rejestracja urządzenia</h2>
                            <p className="text-muted">Postępuj zgodnie z instrukcjami, aby połączyć doniczkę z WiFi.</p>
                        </div>
                        
                        <BleProvisioning />
                        
                    </div>
                </div>
            </div>
        </div>
    );
}

export default AddDevicePage;