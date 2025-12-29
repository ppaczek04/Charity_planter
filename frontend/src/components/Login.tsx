import { useState, type ChangeEvent, type FormEvent } from 'react';
import api from '../api/axiosConfig';
import { useNavigate } from 'react-router-dom';
import type { User } from '../interfaces';

function Login() {
    const [formData, setFormData] = useState({ email: '', password: '' });
    const [error, setError] = useState<string>('');
    const navigate = useNavigate();

    const handleChange = (e: ChangeEvent<HTMLInputElement>) => {
        setFormData({ ...formData, [e.target.name]: e.target.value });
    };

    const handleSubmit = async (e: FormEvent) => {
        e.preventDefault();
        try {
            // Mówimy axiosowi, że spodziewamy się obiektu typu User w odpowiedzi
            const response = await api.post<User>('/auth/login', formData);
            
            // Zapisujemy w localStorage (TypeScript wymaga stringa, więc JSON.stringify)
            localStorage.setItem('user', JSON.stringify(response.data));
            
            navigate('/dashboard');
        } catch (err: any) {
            setError('Nieudane logowanie: ' + (err.response?.data || err.message));
        }
    };

    return (
        <div style={{ padding: '20px' }}>
            <h2>Logowanie</h2>
            <form onSubmit={handleSubmit} style={{ display: 'flex', flexDirection: 'column', gap: '10px', maxWidth: '300px' }}>
                <input name="email" placeholder="Email" onChange={handleChange} required />
                <input name="password" type="password" placeholder="Hasło" onChange={handleChange} required />
                <button type="submit">Zaloguj</button>
            </form>
            {error && <p style={{ color: 'red' }}>{error}</p>}
        </div>
    );
}

export default Login;