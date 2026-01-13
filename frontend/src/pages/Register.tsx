import { useState, type ChangeEvent, type FormEvent } from 'react';
import api from '../api/axiosConfig';
import { useNavigate, Link } from 'react-router-dom';

function Register() {
    const [formData, setFormData] = useState({
        username: '',
        email: '',
        password: '',
    });
    
    const [msg, setMsg] = useState<{ text: string, type: 'success' | 'danger' } | null>(null);
    const navigate = useNavigate();

    const handleChange = (e: ChangeEvent<HTMLInputElement>) => {
        setFormData({ ...formData, [e.target.name]: e.target.value });
    };

    const handleSubmit = async (e: FormEvent) => {
        e.preventDefault();
        setMsg(null);

        try {
            await api.post('/auth/register', formData);
            setMsg({ text: 'Rejestracja udana! Za chwilę nastąpi przekierowanie...', type: 'success' });
            
            setTimeout(() => navigate('/login'), 2000);
        } catch (error: any) {
            const errorMessage = error.response?.data || error.message || 'Wystąpił błąd';
            setMsg({ text: 'Błąd: ' + errorMessage, type: 'danger' });
        }
    };

    return (
        <div className="container d-flex justify-content-center align-items-center" style={{ minHeight: '80vh' }}>
            <div className="card shadow-lg p-4 border-0" style={{ width: '100%', maxWidth: '400px', borderRadius: '15px' }}>
                
                <div className="text-center mb-4">
                    <h2 className="fw-bold" style={{ color: '#6f42c1' }}>Zarejestruj się</h2>
                    <p className="text-muted">Utwórz konto, aby zarządzać swoimi roślinami</p>
                </div>

                <form onSubmit={handleSubmit}>
                    <div className="mb-3">
                        <label className="form-label text-muted small">Nazwa użytkownika</label>
                        <div className="input-group">
                            <span className="input-group-text bg-light border-end-0">
                                <i className="bi bi-person"></i>
                            </span>
                            <input 
                                name="username" 
                                type="text"
                                className="form-control form-control-lg border-start-0 ps-0" 
                                placeholder="Twój nick"
                                value={formData.username}
                                onChange={handleChange} 
                                required 
                            />
                        </div>
                    </div>

                    <div className="mb-3">
                        <label className="form-label text-muted small">Adres Email</label>
                        <div className="input-group">
                            <span className="input-group-text bg-light border-end-0">
                                <i className="bi bi-envelope"></i>
                            </span>
                            <input 
                                name="email" 
                                type="email" 
                                className="form-control form-control-lg border-start-0 ps-0" 
                                placeholder="jan@example.com"
                                value={formData.email}
                                onChange={handleChange} 
                                required 
                            />
                        </div>
                    </div>

                    <div className="mb-4">
                        <label className="form-label text-muted small">Hasło</label>
                        <div className="input-group">
                            <span className="input-group-text bg-light border-end-0">
                                <i className="bi bi-lock"></i>
                            </span>
                            <input 
                                name="password" 
                                type="password" 
                                className="form-control form-control-lg border-start-0 ps-0" 
                                placeholder="••••••••"
                                value={formData.password}
                                onChange={handleChange} 
                                required 
                            />
                        </div>
                    </div>

                    <button 
                        type="submit" 
                        className="btn w-100 btn-lg text-white mb-3 shadow-sm"
                        style={{ backgroundColor: '#6f42c1', border: 'none' }}
                    >
                        Zarejestruj się
                    </button>
                </form>

                {msg && (
                    <div className={`alert alert-${msg.type} text-center mt-3`} role="alert">
                        {msg.text}
                    </div>
                )}

                <div className="text-center mt-3">
                    <small className="text-muted">
                        Masz już konto?{' '}
                        <Link to="/login" style={{ color: '#6f42c1', textDecoration: 'none', fontWeight: 'bold' }}>
                            Zaloguj się
                        </Link>
                    </small>
                </div>
            </div>
        </div>
    );
}

export default Register;