import { Link } from 'react-router-dom';

interface NavbarProps {
    username?: string;
    onLogout: () => void;
}

const Navbar = ({ username, onLogout }: NavbarProps) => {
    return (
        <nav className="navbar navbar-expand-lg navbar-dark px-4" style={{ backgroundColor: '#6f42c1' }}>
            <Link className="navbar-brand d-flex align-items-center gap-2" to="/dashboard">
                <i className="bi bi-person-circle"></i>
            </Link>
            <div className="ms-auto d-flex align-items-center gap-3 text-white">
                <Link to="/add-device" className="btn btn-light btn-sm text-primary fw-bold d-flex align-items-center gap-2" style={{ color: '#6f42c1 !important' }}>
                    <i className="bi bi-plus-circle-fill" style={{ color: '#6f42c1' }}></i>
                    Dodaj doniczkę
                </Link>

                <div className="vr bg-white opacity-50 mx-2"></div>
                {username && <span>Witaj, {username}</span>}
                <button onClick={onLogout} className="btn btn-outline-light btn-sm">
                    Wyloguj się
                </button>
            </div>
        </nav>
    );
};

export default Navbar;