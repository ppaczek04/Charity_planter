import { useEffect, useState } from 'react';
import { useNavigate } from 'react-router-dom';
import type { User } from '../interfaces';

function Dashboard() {
    // Stan może być typu User LUB null (jeśli nie załadowano)
    const [user, setUser] = useState<User | null>(null);
    const navigate = useNavigate();

    useEffect(() => {
        const loggedInUser = localStorage.getItem('user');
        if (!loggedInUser) {
            navigate('/login');
        } else {
            // Parsujemy stringa z powrotem na obiekt
            setUser(JSON.parse(loggedInUser));
        }
    }, [navigate]);

    const handleLogout = () => {
        localStorage.removeItem('user');
        navigate('/login');
    };

    if (!user) return <div>Ładowanie...</div>;

    return (
        <div style={{ padding: '20px' }}>
            <h1>Witaj, {user.username}!</h1>
            <div style={{ background: '#f0f0f0', padding: '15px', borderRadius: '8px' }}>
                <p><strong>ID:</strong> {user.id}</p>
                <p><strong>Email:</strong> {user.email}</p>
                <p><strong>Adres MAC urządzenia:</strong> {user.mobileMacAddress || 'Brak'}</p>
            </div>
            <br />
            <button onClick={handleLogout} style={{ padding: '10px 20px', cursor: 'pointer' }}>Wyloguj</button>
        </div>
    );
}

export default Dashboard;