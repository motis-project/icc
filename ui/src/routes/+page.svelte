<script lang="ts">
	import Control from '$lib/map/Control.svelte';

	let zoom = $state(18);
	let bounds = $state<undefined | maplibregl.LngLatBounds>(undefined);
	let map = $state<null | maplibregl.Map>(null);
</script>

<Map
	bind:map
	bind:bounds
	{zoom}
	transformRequest={(url: string, _: any) => {
		if (url.startsWith('/')) {
			return { url: `https://europe.motis-project.de/tiles${url}` };
		}
	}}
	center={[8.563351200419433, 50]}
	class="h-screen"
	style={getStyle(level)}
>
	<Control>
		<SearchMask />
	</Control>
</Map>
