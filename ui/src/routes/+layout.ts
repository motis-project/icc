import {client} from '$lib/openapi';
import {browser} from '$app/environment';

export const prerender = true;

const baseUrl = 'http://localhost:8080';

if (browser) {
    client.setConfig({baseUrl}); //`${window.location}`
}
