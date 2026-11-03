import type { TimelapseSettings } from '@/lib/timelapse-settings'

export type ResolutionPresetId = '640x360' | '854x480' | '1280x720' | 'custom'

export const TIMELAPSE_RESOLUTION_PRESETS: Record<
  Exclude<ResolutionPresetId, 'custom'>,
  Pick<TimelapseSettings, 'width' | 'height' | 'pic_max_size'>
> = {
  '640x360': { width: 640, height: 360, pic_max_size: 307_200 },
  '854x480': { width: 854, height: 480, pic_max_size: 409_600 },
  '1280x720': { width: 1280, height: 720, pic_max_size: 614_400 },
}

export function resolutionPresetFromValues(form: Pick<TimelapseSettings, 'width' | 'height' | 'pic_max_size'>) {
  for (const [id, v] of Object.entries(TIMELAPSE_RESOLUTION_PRESETS) as [
    Exclude<ResolutionPresetId, 'custom'>,
    (typeof TIMELAPSE_RESOLUTION_PRESETS)[keyof typeof TIMELAPSE_RESOLUTION_PRESETS],
  ][]) {
    if (form.width === v.width && form.height === v.height && form.pic_max_size === v.pic_max_size) return id
  }
  return 'custom' as ResolutionPresetId
}

export const TIMELAPSE_TUNES = ['stillimage', 'film', 'animation', 'grain'] as const

export function pathsOnUsb(picture: string, video: string, usbRoot = '/mnt/exUDISK/') {
  return picture.startsWith(usbRoot) && video.startsWith(usbRoot)
}

export const INTERNAL_PICTURE = '/opt/usr/picture/'
export const INTERNAL_VIDEO = '/opt/usr/video/'
