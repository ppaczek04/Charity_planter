interface SensorItemProps {
    label: string;
    value: number | string;
    unit: string;
    minThreshold: number;
    maxThreshold: number;
    icon: string;
}

const SensorItem = ({ label, value, unit, minThreshold, maxThreshold, icon }: SensorItemProps) => {
    return (
        <div className="col-md-6 mb-4">
            <div className="d-flex align-items-center gap-2 mb-2">
                <i className={`bi ${icon} text-success fs-4`}></i>
                <h6 className="mb-0 text-secondary">{label}</h6>
            </div>
            
            <div className="d-flex align-items-center gap-2 mb-1">
                <i className="bi bi-graph-down text-success"></i>
                <span>{minThreshold} {unit}</span>
            </div>
            
            <div className="d-flex align-items-center gap-2">
                <i className="bi bi-graph-up text-success"></i>
                <span>{maxThreshold} {unit}</span>
            </div>

            <div className="mt-2 fw-bold">
                Aktualnie: {value} {unit}
            </div>
        </div>
    );
};

export default SensorItem;