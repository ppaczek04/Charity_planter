import { useState } from "react";

interface DeviceHeaderProps {
    id: number;
    name: string;
    mac: string;
    onRename: (id: number, newName: string) => void;
    isArchived?: boolean;
}

const DeviceHeader = ({ id, name, mac, onRename, isArchived }: DeviceHeaderProps) => {
    const [isEditing, setIsEditing] = useState(false);
    const [tempName, setTempName] = useState(name);

    const handleSave = (e: React.MouseEvent) => {
        e.stopPropagation();
        if (tempName.trim() !== "") {
            onRename(id, tempName);
            setIsEditing(false);
        }
    };

    const handleCancel = (e: React.MouseEvent) => {
        e.stopPropagation();
        setTempName(name);
        setIsEditing(false);
    };

    const handleInputClick = (e: React.MouseEvent) => {
        e.stopPropagation();
    }

    return (
        <div className="card mb-3 shadow-sm" style={{ border: 'none', transition: '0.3s' }}>
            <div className="card-body d-flex align-items-center gap-4">
                <div className="fs-1" style={{ color: '#6f42c1' }}>
                    <i className="bi bi-flower1"></i>
                </div>

                <div className="flex-grow-1">
                    {isEditing ? (
                        <div className="d-flex align-items-center gap-2" onClick={handleInputClick}>
                            <input 
                                autoFocus
                                type="text" 
                                className="form-control form-control-sm"
                                value={tempName}
                                onChange={(e) => setTempName(e.target.value)}
                                style={{ maxWidth: '200px', borderColor: '#6f42c1' }}
                                onKeyDown={(e) => {
                                    if(e.key === 'Enter') handleSave(e as any);
                                    if(e.key === 'Escape') handleCancel(e as any);
                                }}
                            />
                            <button className="btn btn-sm btn-outline-success" onClick={handleSave} title="Zapisz">
                                <i className="bi bi-check-lg"></i>
                            </button>
                            <button className="btn btn-sm btn-outline-secondary" onClick={handleCancel} title="Anuluj">
                                <i className="bi bi-x-lg"></i>
                            </button>
                        </div>
                    ) : (
                        <div className="d-flex align-items-center gap-2 group-hover-container">
                            <h5 className="mb-0 fw-bold">{name}</h5>
                            
                            { !isArchived && 
                            <button 
                                className="btn btn-sm btn-link text-muted p-0 ms-2"
                                onClick={(e) => {
                                    e.stopPropagation();
                                    setIsEditing(true);
                                }}
                                title="Edytuj nazwę"
                            >
                                <i className="bi bi-pencil-square"></i>
                            </button> }
                        </div>
                    )}
                    
                    <div className="text-muted small mt-1">{mac}</div>
                </div>
            </div>
        </div>
    );
};

export default DeviceHeader;