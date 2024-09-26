<script lang="ts">
	import AddressTypeahead from '$lib/AddressTypeahead.svelte';
	import { Label } from '$lib/components/ui/label';
	import * as RadioGroup from '$lib/components/ui/radio-group';
	import * as Select from '$lib/components/ui/select';
	import DateInput from '$lib/DateInput.svelte';
	import { type Location } from '$lib/Location';
	import type { Selected } from 'bits-ui';

	let {
		from = $bindable(),
		to = $bindable(),
		dateTime = $bindable(),
		timeType = $bindable(),
		profile = $bindable()
	}: {
		from: Location | undefined;
		to: Location | undefined;
		dateTime: Date;
		timeType: string;
		profile: Selected<string>;
	} = $props();
</script>

<div class="flex flex-col space-y-4 p-4 shadow-md rounded">
	<AddressTypeahead name="from" class="w-full" placeholder="From" bind:value={from} />
	<AddressTypeahead name="to" class="w-full" placeholder="To" bind:value={to} />
	<div class="flex flex-row space-x-2 justify-between">
		<DateInput bind:value={dateTime} />
		<RadioGroup.Root class="flex space-x-1" bind:value={timeType}>
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
		<div class="min-w-24">
			<Select.Root bind:selected={profile}>
				<Select.SelectTrigger>
					<Select.SelectValue placeholder="Profile" />
				</Select.SelectTrigger>
				<Select.SelectContent>
					<Select.SelectItem value="wheelchair">Wheelchair</Select.SelectItem>
					<Select.SelectItem value="foot">Foot</Select.SelectItem>
					<Select.SelectItem value="bike">Bike</Select.SelectItem>
					<Select.SelectItem value="car">Car</Select.SelectItem>
				</Select.SelectContent>
			</Select.Root>
		</div>
	</div>
</div>
