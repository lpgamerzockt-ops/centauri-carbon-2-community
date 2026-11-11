import type { TimelapseSettings } from '@/lib/timelapse-settings'

export type TimelapseQualityPreset = 'low' | 'medium' | 'high' | 'custom'

export const TIMELAPSE_QUALITY_PRESETS: Record<
  Exclude<TimelapseQualityPreset, 'custom'>,
  Pick<TimelapseSettings, 'bit_rate' | 'crf' | 'preset'>
> = {
  low: { bit_rate: 2_000_000, crf: 23, preset: 'fast' },
  medium: { bit_rate: 4_000_000, crf: 20, preset: 'medium' },
  high: { bit_rate: 8_000_000, crf: 18, preset: 'slow' },
}

export function timelapsePresetFromValues(
  form: Pick<TimelapseSettings, 'bit_rate' | 'crf' | 'preset'>
): TimelapseQualityPreset {
  for (const [id, values] of Object.entries(TIMELAPSE_QUALITY_PRESETS) as [
    Exclude<TimelapseQualityPreset, 'custom'>,
    (typeof TIMELAPSE_QUALITY_PRESETS)[keyof typeof TIMELAPSE_QUALITY_PRESETS],
  ][]) {
    if (form.bit_rate === values.bit_rate && form.crf === values.crf && form.preset === values.preset) return id
  }
  return 'custom'
}

/** Minimum free bytes before CC2 warns (picture + video targets). */
export const TIMELAPSE_MIN_FREE_BYTES = 500 * 1024 * 1024

export function timelapseLowSpace(
  picture?: { available_bytes?: number } | null,
  video?: { available_bytes?: number } | null
): boolean {
  const check = (s?: { available_bytes?: number } | null) =>
    s && typeof s.available_bytes === 'number' && s.available_bytes < TIMELAPSE_MIN_FREE_BYTES
  return Boolean(check(picture) || check(video))
}
