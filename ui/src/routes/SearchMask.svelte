<script lang="ts">
	import AddressTypeahead from '$lib/AddressTypeahead.svelte';
	import { Label } from '$lib/components/ui/label';
	import * as RadioGroup from '$lib/components/ui/radio-group';
	import {
		Select,
		SelectTrigger,
		SelectItem,
		SelectContent,
		SelectValue
	} from '$lib/components/ui/select';
	import DateInput from '$lib/DateInput.svelte';
	import type { Match } from '$lib/openapi';
	import type { Selected } from 'bits-ui';
</script>

<AddressTypeahead
	name="from"
	inputClass="w-full bg-white"
	placeholder="From"
	onSelectedChange={(match: Selected<Match> | undefined) => {
		if (match) {
			start.lng = match.value.lon;
			start.lat = match.value.lat;
			startMarker.setLngLat([start.lng, start.lat]);
		}
	}}
/>
<AddressTypeahead
	name="to"
	inputClass="w-full bg-white"
	placeholder="To"
	onSelectedChange={(match: Selected<Match> | undefined) => {
		if (match) {
			destination.lng = match.value.lon;
			destination.lat = match.value.lat;
			destinationMarker.setLngLat([destination.lng, destination.lat]);
		}
	}}
/>
<div class="flex flex-row space-x-4 justify-between">
	<div class="flex">
		<DateInput class="bg-white" bind:value={dateTime} />
		<RadioGroup.Root class="flex space-x-1 ml-1" bind:value={timeType}>
			<Label
				for="departure"
				class="flex items-center rounded-md border-2 border-muted bg-popover p-1 px-2 hover:bg-accent hover:text-accent-foreground [&:has([data-state=checked])]:border-blue-600 hover:cursor-pointer"
			>
				<RadioGroup.Item value="departure" id="departure" class="sr-only" aria-label="Abfahrt" />
				<span>Abfahrt</span>
			</Label>
			<Label
				for="arrival"
				class="flex items-center rounded-md border-2 border-muted bg-popover p-1 px-2 hover:bg-accent hover:text-accent-foreground [&:has([data-state=checked])]:border-blue-600 hover:cursor-pointer"
			>
				<RadioGroup.Item value="arrival" id="arrival" class="sr-only" aria-label="Ankunft" />
				<span>Ankunft</span>
			</Label>
		</RadioGroup.Root>
	</div>
	<div class="min-w-24">
		<Select bind:selected={profile}>
			<SelectTrigger class="bg-white">
				<SelectValue placeholder="Profile" />
			</SelectTrigger>
			<SelectContent>
				<SelectItem value="wheelchair">Wheelchair</SelectItem>
				<SelectItem value="foot">Foot</SelectItem>
				<SelectItem value="bike">Bike</SelectItem>
				<SelectItem value="car">Car</SelectItem>
			</SelectContent>
		</Select>
	</div>
</div>
