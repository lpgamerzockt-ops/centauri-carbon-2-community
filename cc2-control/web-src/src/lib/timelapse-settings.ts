import { errText, notify, request } from '@/lib/api'
import { t } from '@/lib/i18n'

export type TimelapseSettings = {
  loaded: boolean
  firmware_support: boolean
  path: string
  usb_storage_root?: string
  picture_space?: { available_bytes: number } | null
  video_space?: { available_bytes: number } | null
  recording_mode: string
  width: number
  height: number
  camera_fps: number
  output_fps: number
  bit_rate: number
  crf: number
  preset: string
  tune: string
  profile: string
  level: string
  pic_max_size: number
  keep_source_jpegs: boolean
  picture_dir: string
  video_dir: string
  temp_link: string
}

export const DEFAULT_TIMELAPSE: TimelapseSettings = {
  loaded: true,
  firmware_support: false,
  path: '/opt/usr/cfg/timelapse.cfg',
  recording_mode: 'layer_timelapse',
  width: 640,
  height: 360,
  camera_fps: 20,
  output_fps: 30,
  bit_rate: 4_000_000,
  crf: 20,
  preset: 'medium',
  tune: 'stillimage',
  profile: 'main',
  level: '3.1',
  pic_max_size: 307_200,
  keep_source_jpegs: false,
  picture_dir: '/opt/usr/picture/',
  video_dir: '/opt/usr/video/',
  temp_link: '/tmp/pic_link',
}

/** Matches backend TIMELAPSE_USB_MOUNT_PREFIX / GCODE_USB_ROOT in main.c */
export const TIMELAPSE_USB_ROOT = '/mnt/exUDISK/'

export const TIMELAPSE_USB_PICTURE = `${TIMELAPSE_USB_ROOT}timelapse/picture/`
export const TIMELAPSE_USB_VIDEO = `${TIMELAPSE_USB_ROOT}timelapse/video/`

export function timelapseStoragePathOk(path: string, tempLink: boolean): boolean {
  if (!path || path.includes('..') || path.includes('//')) return false
  if (tempLink) return path.startsWith('/tmp/') || path.startsWith('/opt/usr/')
  return (
    path.startsWith('/opt/usr/') || (path.startsWith(TIMELAPSE_USB_ROOT) && path.length > TIMELAPSE_USB_ROOT.length)
  )
}

export function validateTimelapseForm(
  form: Pick<TimelapseSettings, 'picture_dir' | 'video_dir' | 'temp_link'>
): boolean {
  return (
    timelapseStoragePathOk(form.picture_dir, false) &&
    timelapseStoragePathOk(form.video_dir, false) &&
    timelapseStoragePathOk(form.temp_link, true)
  )
}

export async function fetchTimelapseSettings(): Promise<TimelapseSettings> {
  const data = await request('/api/timelapse/settings')
  return { ...DEFAULT_TIMELAPSE, ...data, keep_source_jpegs: data.keep_source_jpegs === true }
}

export async function saveTimelapseSettings(body: Partial<TimelapseSettings>) {
  return request('/api/timelapse/settings', {
    method: 'POST',
    headers: { 'Content-Type': 'application/json', 'X-CC2-Request': '1' },
    body: JSON.stringify(body),
  })
}

export async function loadTimelapseSettings(): Promise<TimelapseSettings | null> {
  try {
    return await fetchTimelapseSettings()
  } catch (e) {
    notify(`${t('timelapse.load_failed')} ${errText(e)}`, 'error')
    return null
  }
}
