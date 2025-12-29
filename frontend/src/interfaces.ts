export interface User {
    id: number;
    username: string;
    email: string;
    mobileMacAddress?: string;
}

export interface AuthResponse {
    token?: string; // Jeśli kiedyś dodasz JWT
    id: number;
    username: string;
    email: string;
    mobileMacAddress: string;
}