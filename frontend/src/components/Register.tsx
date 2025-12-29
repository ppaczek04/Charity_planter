import { useState, type ChangeEvent, type FormEvent } from 'react';
import api from '../api/axiosConfig';
import { useNavigate } from 'react-router-dom';

function Register() {
    const [formData, setFormData] = useState({
        username: '',
        email: '',
        password: '',
        mobileMacAddress: ''
    });
    const [msg, setMsg] = useState<string>('');
    const navigate = useNavigate();

    // Typujemy zdarzenie zmiany w polu input
    const handleChange = (e: ChangeEvent<HTMLInputElement>) => {
        setFormData({ ...formData, [e.target.name]: e.target.value });
    };

    // Typujemy zdarzenie wysłania formularza
    const handleSubmit = async (e: FormEvent) => {
        e.preventDefault();
        try {
            await api.post('/auth/register', formData);
            setMsg('Rejestracja udana! Przekierowanie...');
            setTimeout(() => navigate('/login'), 2000);
        } catch (error: any) {
            // Error handling w axiosie bywa tricky w TS, używamy any dla uproszczenia w projekcie studenckim
            const errorMessage = error.response?.data || error.message || 'Wystąpił błąd';
            setMsg('Błąd: ' + errorMessage);
        }
    };

    return (
        <div style={{ padding: '20px' }}>
            <h2>Rejestracja</h2>
            <form onSubmit={handleSubmit} style={{ display: 'flex', flexDirection: 'column', gap: '10px', maxWidth: '300px' }}>
                <input name="username" placeholder="Nazwa użytkownika" onChange={handleChange} required />
                <input name="email" type="email" placeholder="Email" onChange={handleChange} required />
                <input name="password" type="password" placeholder="Hasło" onChange={handleChange} required />
                <input name="mobileMacAddress" placeholder="MAC Telefonu (np. AA:BB:CC...)" onChange={handleChange} />
                <button type="submit">Zarejestruj się</button>
            </form>
            {msg && <p>{msg}</p>}
        </div>
    );
}

export default Register;