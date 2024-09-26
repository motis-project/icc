import type { Match } from './openapi';

export type Location = {
	match?: Match;
	level?: number;
	precision?: number;
};
