const DEFAULT_LIVE_TOLERANCE_MS = 2_000;
const LABEL_HALF_WIDTH_PERCENT = 12;
const LABEL_SPACING_PERCENT = 26;

export function classifyPlayback(
  positionMs,
  oldestMs,
  newestMs,
  liveToleranceMs = DEFAULT_LIVE_TOLERANCE_MS,
) {
  if (
    !Number.isFinite(positionMs)
    || !Number.isFinite(oldestMs)
    || !Number.isFinite(newestMs)
    || newestMs <= oldestMs
    || positionMs > newestMs + Math.max(0, liveToleranceMs)
  ) {
    return { state: 'invalid', delayMs: null, positionMs };
  }

  const delayMs = Math.max(0, newestMs - positionMs);
  if (positionMs < oldestMs) {
    return { state: 'expired', delayMs, positionMs };
  }
  if (positionMs >= newestMs - Math.max(0, liveToleranceMs)) {
    return { state: 'live', delayMs, positionMs };
  }
  return { state: 'available', delayMs, positionMs };
}

export function formatDelay(delayMs) {
  if (!Number.isFinite(delayMs) || delayMs <= 0) return 'Live';

  const totalSeconds = Math.floor(delayMs / 1_000);
  if (totalSeconds < 60) return `${totalSeconds} s`;

  const totalMinutes = Math.floor(totalSeconds / 60);
  if (totalMinutes < 60) return `${totalMinutes} m`;

  const totalHours = Math.floor(totalMinutes / 60);
  const minutes = totalMinutes % 60;
  if (totalHours < 24) return minutes ? `${totalHours} h ${minutes} m` : `${totalHours} h`;

  const days = Math.floor(totalHours / 24);
  const hours = totalHours % 24;
  return hours ? `${days} d ${hours} h` : `${days} d`;
}

export function buildMarkerLayout(outputs, buffer) {
  const { oldestMs, newestMs, liveToleranceMs = DEFAULT_LIVE_TOLERANCE_MS } = buffer;
  const markers = outputs.map((output) => {
    const classification = classifyPlayback(
      output.positionMs,
      oldestMs,
      newestMs,
      liveToleranceMs,
    );
    return {
      ...output,
      ...classification,
      delayLabel: formatDelay(classification.delayMs),
      percent: null,
      lane: null,
    };
  });

  const playable = markers
    .filter(({ state }) => state === 'available' || state === 'live')
    .sort((a, b) => a.positionMs - b.positionMs);
  const laneCenters = [];
  for (const marker of playable) {
    marker.percent = Math.min(100, (marker.positionMs - oldestMs) / (newestMs - oldestMs) * 100);
    // Labels shift inward at the edges, while stems stay at their true playback positions.
    const center = Math.max(LABEL_HALF_WIDTH_PERCENT, Math.min(100 - LABEL_HALF_WIDTH_PERCENT, marker.percent));
    const freeLane = laneCenters.findIndex((lastCenter) => center - lastCenter >= LABEL_SPACING_PERCENT);
    marker.lane = freeLane === -1 ? laneCenters.length : freeLane;
    laneCenters[marker.lane] = center;
  }
  return markers;
}
