<script lang="ts">
	import X from 'lucide-svelte/icons/x';
	import { getStyle } from '$lib/map/style';
	import Map from '$lib/map/Map.svelte';
	import Control from '$lib/map/Control.svelte';
	import SearchMask from './SearchMask.svelte';
	import { type Location } from '$lib/Location';
	import { Card } from '$lib/components/ui/card';
	import type { Selected } from 'bits-ui';
	import { type Itinerary, plan, type PlanResponse } from '$lib/openapi';
	import ItineraryList from './ItineraryList.svelte';
	import ConnectionDetail from './ConnectionDetail.svelte';
	import { Button } from '$lib/components/ui/button';
	import ItineraryGeoJson from './ItineraryGeoJSON.svelte';
	import maplibregl from 'maplibre-gl';

	let level = $state(0);
	let zoom = $state(18);
	let bounds = $state<maplibregl.LngLatBoundsLike>();
	let map = $state<maplibregl.Map>();

	let from = $state<Location>();
	let to = $state<Location>();
	let dateTime = $state<Date>(new Date());
	let timeType = $state<string>('departure');
	let profile = $state<Selected<string>>({ value: 'foot', label: 'Foot' });

	const pad = (x: number) => ('0' + x).slice(-2);
	let baseQuery = $derived(
		from && to
			? {
					query: {
						date: `${pad(dateTime.getUTCMonth() + 1)}-${pad(dateTime.getUTCDate())}-${dateTime.getUTCFullYear()}`,
						time: `${pad(dateTime.getUTCHours())}:${pad(dateTime.getUTCMinutes())}`,
						fromPlace: `${from?.match?.lat},${from?.match?.lon},${from?.level}`,
						toPlace: `${to?.match?.lat},${to?.match?.lon},${to?.level}`,
						wheelchair: profile.value == 'wheelchair',
						timetableView: true,
						arriveBy: timeType === 'arrival'
					}
				}
			: undefined
	);
	let routingResponses = $state<Array<Promise<PlanResponse>>>([]);
	$effect(() => {
		if (baseQuery) {
			routingResponses = [plan<true>(baseQuery).then((response) => response.data)];
			selectedItinerary = undefined;
		}
	});

	let selectedItinerary = $state<Itinerary>();
</script>

<Map
	bind:map
	bind:bounds
	bind:zoom
	transformRequest={(url: string) => {
		if (url.startsWith('/')) {
			return { url: `https://europe.motis-project.de/tiles${url}` };
		}
	}}
	center={[8.563351200419433, 50]}
	class="h-screen"
	style={getStyle(level)}
>
	<Control position="top-left">
		<Card class="w-[500px] overflow-y-auto overflow-x-hidden bg-white rounded-lg">
			<SearchMask bind:from bind:to bind:dateTime bind:timeType bind:profile />
		</Card>
	</Control>

	{#if !selectedItinerary && baseQuery && routingResponses.length !== 0}
		<Control position="top-left">
			<Card class="w-[500px] max-h-[70vh] overflow-y-auto overflow-x-hidden bg-white rounded-lg">
				<ItineraryList {routingResponses} {baseQuery} bind:selectedItinerary />
			</Card>
		</Control>
	{/if}

	{#if selectedItinerary}
		<Control position="top-left">
			<Card class="w-[500px] bg-white rounded-lg">
				<div class="w-full flex justify-between items-center shadow-md pl-1 mb-1">
					<h2 class="ml-2 text-base font-semibold tracking-tight">Journey Details</h2>
					<Button
						variant="ghost"
						on:click={() => {
							selectedItinerary = undefined;
						}}
					>
						<X />
					</Button>
				</div>
				<div class="overflow-y-auto overflow-x-hidden max-h-[70vh]">
					<ConnectionDetail itinerary={selectedItinerary} />
				</div>
			</Card>
		</Control>
		<ItineraryGeoJson itinerary={selectedItinerary} {level} />
	{/if}
</Map>
