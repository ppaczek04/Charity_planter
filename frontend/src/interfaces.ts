export interface User {
    id: number;
    username: string;
    email: string;
    mobileMacAddress?: string;
}

export interface AuthResponse {
    token?: string;
    id: number;
    username: string;
    email: string;
    mobileMacAddress: string;
}

export interface Device {
    id: number;
    mac: string;
    ownerId: string;
    name?: string;
    isArchived?: boolean;
    holidayMode?: boolean;
    soilMin?: number;
    soilMax?: number;
    measurementInterval?: number;
}

export const tabTranslations: Record<string, string> = {
    measurements: 'Statystyki',
    watering: 'Podlewanie',
    settings: 'Ustawienia'
};