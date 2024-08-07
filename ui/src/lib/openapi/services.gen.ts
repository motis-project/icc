// This file is auto-generated by @hey-api/openapi-ts

import type { CancelablePromise } from './core/CancelablePromise';
import { OpenAPI } from './core/OpenAPI';
import { request as __request } from './core/request';
import type { PlanData, PlanResponse } from './types.gen';

/**
 * Computes optimal connections from one place to another.
 * @param data The data for the request.
 * @param data.fromPlace latitude, longitude, level tuple in degrees or stop id
 * @param data.toPlace latitude, longitude, level tuple in degrees or stop id
 * @param data.date Optional. Defaults to the current date.
 *
 * Departure date ($arriveBy=false) / arrival date ($arriveBy=true), format: 06-28-2024
 *
 * @param data.time Optional. Defaults to the current time.
 *
 * Meaning depending on `arriveBy`
 * - Departure time for `arriveBy=false`
 * - Arrival time for `arriveBy=true`
 *
 * Format:
 * - 12h format: 7:06pm
 * - 24h format: 19:06
 *
 * @param data.maxTransfers The maximum number of allowed transfers.
 * If not provided, the routing uses the server-side default value
 * which is hardcoded and very high to cover all use cases.
 *
 * *Warning*: Use with care. Setting this too low can lead to
 * optimal (e.g. the fastest) journeys not being found.
 * If this value is too low to reach the destination at all,
 * it can lead to slow routing performance.
 *
 * @param data.maxHours The maximum travel time in hours.
 * If not provided, the routing to uses the value
 * hardcoded in the server which is usually quite high.
 *
 * *Warning*: Use with care. Setting this too low can lead to
 * optimal (e.g. the least transfers) journeys not being found.
 * If this value is too low to reach the destination at all,
 * it can lead to slow routing performance.
 *
 * @param data.minTransferTime Minimum transfer time for each transfer.
 * @param data.transferTimeFactor Factor to multiply transfer times with.
 * @param data.wheelchair Whether the trip must be wheelchair accessible.
 * @param data.mode A comma separated list of allowed modes.
 *
 * Default if not provided: `WALK,TRANSIT`
 *
 * @param data.numItineraries The minimum number of itineraries to compute.
 * This is only relevant if `timetableView=true`.
 * The default value is 5.
 *
 * @param data.pageCursor Use the cursor to go to the next "page" of itineraries.
 * Copy the cursor from the last response and keep the original request as is.
 * This will enable you to search for itineraries in the next or previous time-window.
 *
 * @param data.timetableView Optional. Default is `true`.
 *
 * Search for the best trip options within a time window.
 * If true two itineraries are considered optimal
 * if one is better on arrival time (earliest wins)
 * and the other is better on departure time (latest wins).
 * In combination with arriveBy this parameter cover the following use cases:
 *
 * `timetable=false` = waiting for the first transit departure/arrival is considered travel time:
 * - `arriveBy=true`: event (e.g. a meeting) starts at 10:00 am,
 * compute the best journeys that arrive by that time (maximizes departure time)
 * - `arriveBy=false`: event (e.g. a meeting) ends at 11:00 am,
 * compute the best journeys that depart after that time
 *
 * `timetable=true` = optimize "later departure" + "earlier arrival" and give all options over a time window:
 * - `arriveBy=true`: the time window around `date` and `time` refers to the arrival time window
 * - `arriveBy=false`: the time window around `date` and `time` refers to the departure time window
 *
 * @param data.arriveBy Optional. Default is `false`.
 *
 * - `arriveBy=true`: the parameters `date` and `time` refer to the arrival time
 * - `arriveBy=false`: the parameters `date` and `time` refer to the departure time
 *
 * @param data.searchWindow Optional. Default is 2 hours which is `7200`.
 *
 * The length of the search-window in seconds. Default value two hours.
 *
 * - `arriveBy=true`: number of seconds between the earliest departure time and latest departure time
 * - `arriveBy=false`: number of seconds between the earliest arrival time and the latest arrival time
 *
 * @param data.maxPreTransitTime Optional. Default is 15min which is `900`.
 * Maximum time in seconds for the first street leg.
 *
 * @param data.maxPostTransitTime Optional. Default is 15min which is `900`.
 * Maximum time in seconds for the last street leg.
 *
 * @returns unknown routing result
 * @throws ApiError
 */
export const plan = (data: PlanData): CancelablePromise<PlanResponse> => { return __request(OpenAPI, {
    method: 'GET',
    url: '/api/v1/plan',
    query: {
        fromPlace: data.fromPlace,
        toPlace: data.toPlace,
        date: data.date,
        time: data.time,
        maxTransfers: data.maxTransfers,
        maxHours: data.maxHours,
        minTransferTime: data.minTransferTime,
        transferTimeFactor: data.transferTimeFactor,
        wheelchair: data.wheelchair,
        mode: data.mode,
        numItineraries: data.numItineraries,
        pageCursor: data.pageCursor,
        timetableView: data.timetableView,
        arriveBy: data.arriveBy,
        searchWindow: data.searchWindow,
        maxPreTransitTime: data.maxPreTransitTime,
        maxPostTransitTime: data.maxPostTransitTime
    }
}); };