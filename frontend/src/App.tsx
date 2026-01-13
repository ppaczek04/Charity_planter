import { BrowserRouter as Router, Routes, Route, Link, useLocation } from 'react-router-dom';
import Login from './pages/Login';
import Register from './pages/Register';
import Dashboard from './pages/Dashboard';
import AddDevicePage from './pages/AddDevicePage';

function AppContent() {
    const location = useLocation();

    const isLogin = location.pathname.startsWith('/login') ||  location.pathname.startsWith('/register');

    return (
        <div style={{ backgroundColor: '#f3f0ff', minHeight: '100vh' }}> 
            
            {isLogin && (
                <nav className="navbar navbar-expand-lg navbar-light bg-white shadow-sm px-4">
                    <Link className="navbar-brand fw-bold text-primary" to="/" style={{ color: '#6f42c1 !important' }}>
                        <i className="bi bi-flower1 me-2"></i>Charity Planter
                    </Link>
                    <div className="ms-auto">
                        <Link to="/login" className="btn btn-outline-primary me-2" style={{ borderColor: '#6f42c1', color: '#6f42c1' }}>Logowanie</Link>
                        <Link to="/register" className="btn btn-primary" style={{ backgroundColor: '#6f42c1', borderColor: '#6f42c1' }}>Rejestracja</Link>
                    </div>
                </nav>
            )}

            <Routes>
                <Route path="/login" element={<Login />} />
                <Route path="/register" element={<Register />} />
                <Route path="/dashboard" element={<Dashboard />} />
                <Route path="/add-device" element={<AddDevicePage />} />
                <Route path="/" element={<Login />} />
            </Routes>
        </div>
    );
}

function App() {
    return (
        <Router>
            <AppContent />
        </Router>
    );
}

export default App;
