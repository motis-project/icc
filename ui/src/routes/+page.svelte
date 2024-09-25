<script lang="ts">
	import { getStyle } from '$lib/map/style';
	import Map from '$lib/map/Map.svelte';
	import Control from '$lib/map/Control.svelte';
	import SearchMask from './SearchMask.svelte';
	import { Card } from '$lib/components/ui/card';

	let level = $state(0);
	let zoom = $state(18);
	let bounds = $state<maplibregl.LngLatBoundsLike | undefined>(undefined);
	let map = $state<maplibregl.Map | undefined>(undefined);

	let from = $state<Location>();
	let to = $state<Location>();
	let dateTime = $state<Date>(new Date());
	let timeType = $state<string>('departure');
</script>

<Map
	bind:map
	bind:bounds
	bind:zoom
	transformRequest={(url: string, _: any) => {
		if (url.startsWith('/')) {
			return { url: `https://europe.motis-project.de/tiles${url}` };
		}
	}}
	center={[8.563351200419433, 50]}
	class="h-screen"
	style={getStyle(level)}
>
	<Control position="top-left">
		<Card class="w-[500px] max-h-[90vh] overflow-y-auto overflow-x-hidden bg-white rounded-lg">
			<SearchMask {from} {to} {dateTime} {timeType} />
		</Card>
	</Control>
</Map>
