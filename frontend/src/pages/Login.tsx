import { useState, type ChangeEvent, type FormEvent } from 'react';
import api from '../api/axiosConfig';
import { useNavigate, Link } from 'react-router-dom';
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
            const response = await api.post<User>('/auth/login', formData);
            localStorage.setItem('user', JSON.stringify(response.data));
            navigate('/dashboard');
        } catch (err: any) {
            setError('Nieudane logowanie: ' + (err.response?.data || err.message));
        }
    };

    return (
        <div className="container d-flex justify-content-center align-items-center" style={{ minHeight: '80vh' }}>
            <div className="card shadow-lg p-4 border-0" style={{ width: '100%', maxWidth: '400px', borderRadius: '15px' }}>
                <div className="text-center mb-4">
                    <h2 className="fw-bold" style={{ color: '#6f42c1' }}>Witaj ponownie</h2>
                    <p className="text-muted">Zaloguj się do swojego inteligentnego ogrodu</p>
                </div>
                
                <form onSubmit={handleSubmit}>
                    <div className="mb-3">
                        <label className="form-label text-muted small">Adres Email</label>
                        <input 
                            name="email" 
                            type="email" 
                            className="form-control form-control-lg" 
                            placeholder="np. jan@example.com"
                            onChange={handleChange} 
                            required 
                        />
                    </div>
                    
                    <div className="mb-4">
                        <label className="form-label text-muted small">Hasło</label>
                        <input 
                            name="password" 
                            type="password" 
                            className="form-control form-control-lg" 
                            placeholder="••••••••"
                            onChange={handleChange} 
                            required 
                        />
                    </div>

                    <button 
                        type="submit" 
                        className="btn w-100 btn-lg text-white mb-3"
                        style={{ backgroundColor: '#6f42c1', border: 'none' }}
                    >
                        Zaloguj się
                    </button>
                </form>

                {error && <div className="alert alert-danger text-center">{error}</div>}

                <div className="text-center mt-3">
                    <small className="text-muted">Nie masz konta? <Link to="/register" style={{ color: '#6f42c1', textDecoration: 'none', fontWeight: 'bold' }}>Zarejestruj się</Link></small>
                </div>
            </div>
        </div>
    );
}

export default Login;